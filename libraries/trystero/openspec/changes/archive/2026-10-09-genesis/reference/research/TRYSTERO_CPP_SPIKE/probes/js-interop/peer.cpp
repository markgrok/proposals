// Throwaway feasibility probe: a C++ peer compiled with Emscripten that joins a Trystero 0.26 room over public
// Nostr relays and talks to Trystero JS peers. It implements only what the question needs: discovery, the
// offer/answer exchange with trickled candidates, the room envelope, the handshake and inline actions.
#include <boost/hash2/sha1.hpp>
#include <boost/hash2/sha2.hpp>
#include <boost/json.hpp>
#include <boost/json/src.hpp>
#include <emscripten/bind.h>
#include <emscripten/eventloop.h>
#include <emscripten/val.h>
#include <secp256k1.h>
#include <secp256k1_extrakeys.h>
#include <secp256k1_schnorrsig.h>
#include <sys/random.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

using emscripten::val;
namespace json = boost::json;
using bytes = std::vector<std::uint8_t>;

namespace {

// ---- Trystero's primitives, byte for byte -------------------------------------------------------------

constexpr std::string_view id_charset = "0123456789AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz";

bytes random_bytes(std::size_t n) {
    bytes out(n);
    getentropy(out.data(), n);
    return out;
}

std::string gen_id(std::size_t n) {
    std::string out;
    for (auto b : random_bytes(n)) out += id_charset[b % id_charset.size()];
    return out;
}

std::string to_hex(const std::uint8_t* data, std::size_t n) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string out;
    for (std::size_t i = 0; i < n; ++i) {
        out += digits[data[i] >> 4];
        out += digits[data[i] & 15];
    }
    return out;
}

std::array<std::uint8_t, 32> sha256(std::string_view text) {
    boost::hash2::sha2_256 h;
    h.update(text.data(), text.size());
    auto d = h.result();
    std::array<std::uint8_t, 32> out{};
    std::copy(d.begin(), d.end(), out.begin());
    return out;
}

// Trystero's sha1(): every digest byte written in base 36 without padding, concatenated.
std::string sha1_topic(std::string_view text) {
    boost::hash2::sha1_160 h;
    h.update(text.data(), text.size());
    std::string out;
    for (std::uint8_t b : h.result()) {
        std::string digits;
        do {
            digits.insert(digits.begin(), "0123456789abcdefghijklmnopqrstuvwxyz"[b % 36]);
            b /= 36;
        } while (b);
        out += digits;
    }
    return out;
}

double str_to_num(std::string_view text, double limit) {
    double sum = 0;
    for (unsigned char c : text) sum += c;  // charCodeAt over ASCII input
    return std::fmod(sum, limit);
}

// Trystero's deterministic shuffle of the default relays, seeded from the app id.
std::vector<std::string> shuffle(std::vector<std::string> xs, double seed) {
    auto rand = [&seed] {
        const double x = std::sin(seed++) * 10000;
        return x - std::floor(x);
    };
    for (std::size_t i = xs.size(); i;) {
        const auto j = static_cast<std::size_t>(std::floor(rand() * static_cast<double>(i)));
        --i;
        std::swap(xs[i], xs[j]);
    }
    return xs;
}

std::string base64_encode(const bytes& in) {
    static constexpr char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    std::size_t i = 0;
    for (; i + 2 < in.size(); i += 3) {
        const unsigned v = (in[i] << 16) | (in[i + 1] << 8) | in[i + 2];
        for (int s : {18, 12, 6, 0}) out += table[(v >> s) & 63];
    }
    if (i + 1 == in.size()) {
        const unsigned v = in[i] << 16;
        out += table[(v >> 18) & 63];
        out += table[(v >> 12) & 63];
        out += "==";
    } else if (i + 2 == in.size()) {
        const unsigned v = (in[i] << 16) | (in[i + 1] << 8);
        for (int s : {18, 12, 6}) out += table[(v >> s) & 63];
        out += '=';
    }
    return out;
}

bytes base64_decode(std::string_view in) {
    auto value = [](char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    };
    bytes out;
    unsigned buffer = 0;
    int bits = 0;
    for (char c : in) {
        const int v = value(c);
        if (v < 0) continue;
        buffer = (buffer << 6) | static_cast<unsigned>(v);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<std::uint8_t>((buffer >> bits) & 0xff));
        }
    }
    return out;
}

val to_js(const bytes& data) {
    val array = val::global("Uint8Array").new_(data.size());
    array.call<void>("set", val(emscripten::typed_memory_view(data.size(), data.data())));
    return array;
}

bytes from_js(const val& buffer) {
    val view = val::global("Uint8Array").new_(buffer);
    bytes out(view["length"].as<std::size_t>());
    val(emscripten::typed_memory_view(out.size(), out.data())).call<void>("set", view);
    return out;
}

// ---- The peer's state ---------------------------------------------------------------------------------

void log(const std::string& category, const std::string& text) {
    val::module_property("log")(val(category), val(text));
}

struct config {
    std::string app_id;
    std::string room_id;
    std::string password;
    std::vector<std::string> relays;
};

struct relay {
    std::string url;
    val socket = val::undefined();
    std::string sub_id = gen_id(64);
    int announce_attempt = 0;
    bool open = false;
};

struct peer_state {
    val pc = val::undefined();
    val channel = val::undefined();
    std::string offer_id;
    bool initiator = false;
    bool answered = false;
    bool signal_sent = false;  // offer or answer published: candidates may follow
    bool channel_open = false;
    bool remote_ready = false;
    bool active = false;
    double last_counter_announce = 0;
    std::vector<std::string> outgoing_candidates;
    std::vector<val> incoming_candidates;
};

struct state {
    config cfg;
    std::string self_id;
    std::string root_plain;
    std::string root_topic;
    std::string self_topic;
    std::string room_token;
    val key = val::undefined();
    secp256k1_context* secp = nullptr;
    secp256k1_keypair keypair{};
    std::string pubkey;
    std::vector<relay> relays;
    std::map<std::string, peer_state> peers;
    std::set<std::string> seen_events;
};

state* app = nullptr;

double now_ms() { return val::global("Date").call<double>("now"); }

int topic_kind(std::string_view topic) { return static_cast<int>(str_to_num(topic, 10000)) + 20000; }

std::string peer_topic(const std::string& peer_id) { return sha1_topic(app->root_plain + "@" + peer_id); }

// ---- Nostr --------------------------------------------------------------------------------------------

std::string make_event(const std::string& topic, const std::string& content) {
    const auto created_at = static_cast<std::int64_t>(std::time(nullptr));
    const int kind = topic_kind(topic);
    json::array tags{json::array{"x", topic}};
    const std::string serialized = json::serialize(json::array{0, app->pubkey, created_at, kind, tags, content});
    const auto id = sha256(serialized);
    std::array<std::uint8_t, 64> sig{};
    const auto aux = random_bytes(32);
    secp256k1_schnorrsig_sign32(app->secp, sig.data(), id.data(), &app->keypair, aux.data());
    json::object event{
        {"kind", kind},
        {"tags", tags},
        {"created_at", created_at},
        {"content", content},
        {"pubkey", app->pubkey},
        {"id", to_hex(id.data(), id.size())},
        {"sig", to_hex(sig.data(), sig.size())},
    };
    return json::serialize(json::array{"EVENT", event});
}

void publish(const std::string& topic, const std::string& content, const std::string& what) {
    const std::string event = make_event(topic, content);
    int sent = 0;
    for (auto& r : app->relays) {
        if (!r.open) continue;
        r.socket.call<void>("send", val(event));
        ++sent;
    }
    log("nostr", "published " + what + " on topic " + topic.substr(0, 12) + "... to " + std::to_string(sent) + " relays");
}

void announce_tick(void* user) {
    auto& r = app->relays[reinterpret_cast<std::intptr_t>(user)];
    if (!r.open) return;
    const std::string content = json::serialize(json::object{{"peerId", app->self_id}, {"nonce", gen_id(8)}});
    r.socket.call<void>("send", val(make_event(app->root_topic, content)));
    if (r.announce_attempt == 0) log("nostr", "announced on " + r.url);
    // Trystero's cadence: warmup 233, 533, 1333 ms, one recovery pulse at 5333 ms, then every 60 s.
    static constexpr std::array<int, 4> delays{233, 533, 1333, 5333};
    const int attempt = r.announce_attempt++;
    emscripten_set_timeout(announce_tick, attempt < 4 ? delays[attempt] : 60000, user);
}

// ---- WebCrypto: AES-GCM with Trystero's 16-byte IV and "iv,iv,...$base64" text form --------------------

val encrypt(std::string plain) {
    const auto iv = random_bytes(16);
    val algorithm = val::object();
    algorithm.set("name", val("AES-GCM"));
    algorithm.set("iv", to_js(iv));
    val text = val::global("TextEncoder").new_().call<val>("encode", val(plain));
    val cipher = co_await val::global("crypto")["subtle"].call<val>("encrypt", algorithm, app->key, text);
    std::string out;
    for (std::size_t i = 0; i < iv.size(); ++i) out += (i ? "," : "") + std::to_string(iv[i]);
    co_return val(out + "$" + base64_encode(from_js(cipher)));
}

val decrypt(std::string raw) {
    const auto dollar = raw.find('$');
    bytes iv;
    for (std::size_t at = 0; at < dollar;) {
        const auto comma = std::min(raw.find(',', at), dollar);
        iv.push_back(static_cast<std::uint8_t>(std::stoi(raw.substr(at, comma - at))));
        at = comma + 1;
    }
    val algorithm = val::object();
    algorithm.set("name", val("AES-GCM"));
    algorithm.set("iv", to_js(iv));
    val result = co_await val::module_property("settle")(
        val::global("crypto")["subtle"].call<val>("decrypt", algorithm, app->key, to_js(base64_decode(raw.substr(dollar + 1)))));
    if (!result["ok"].as<bool>()) co_return val::null();
    co_return val::global("TextDecoder").new_().call<val>("decode", result["value"]);
}

// ---- The room's data plane: envelope, handshake, actions ----------------------------------------------

bytes room_frame(const bytes& payload) {
    bytes out{1, static_cast<std::uint8_t>(app->room_token.size() >> 8), static_cast<std::uint8_t>(app->room_token.size() & 0xff)};
    out.insert(out.end(), app->room_token.begin(), app->room_token.end());
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

bytes presence_frame(bool present) {
    bytes out{2, static_cast<std::uint8_t>(present), static_cast<std::uint8_t>(app->room_token.size() >> 8),
              static_cast<std::uint8_t>(app->room_token.size() & 0xff)};
    out.insert(out.end(), app->room_token.begin(), app->room_token.end());
    return out;
}

constexpr int text_format = 0;
constexpr int json_format = 1;

// Action wire v2, inline kind: version, kind | format << 4, 32-byte name, uint16 metadata length, payload.
bytes inline_action(std::string_view type, int format, std::string_view payload) {
    bytes out(36, 0);
    out[0] = 2;
    out[1] = static_cast<std::uint8_t>(0 | (format << 4));
    std::copy(type.begin(), type.end(), out.begin() + 2);
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

void send_action(const std::string& peer_id, std::string_view type, int format, std::string_view payload) {
    auto& p = app->peers[peer_id];
    if (!p.channel_open) return;
    p.channel.call<void>("send", to_js(room_frame(inline_action(type, format, payload))));
    log("action", "sent " + std::string(type) + " to " + peer_id + ": " + std::string(payload));
}

void activate(const std::string& peer_id) {
    auto& p = app->peers[peer_id];
    if (p.active || !p.remote_ready || !p.channel_open) return;
    p.active = true;
    log("room", "peer joined: " + peer_id + " (handshake complete)");
    send_action(peer_id, "chat", text_format, "hello from C++ compiled to WebAssembly");
    send_action(peer_id, "drink", json_format, json::serialize(json::object{{"drink", "cafezinho"}, {"withIce", false}, {"from", "c++"}}));
}

void on_channel_open(std::string peer_id, val) {
    auto& p = app->peers[peer_id];
    p.channel_open = true;
    log("webrtc", "data channel open with " + peer_id);
    p.channel.call<void>("send", to_js(presence_frame(true)));
    // No password and no user handshake: the local side passes at once and says so.
    p.channel.call<void>("send", to_js(room_frame(inline_action("@_hsready", text_format, ""))));
    log("room", "sent presence and @_hsready to " + peer_id);
    activate(peer_id);
}

void on_channel_message(std::string peer_id, val event) {
    const bytes frame = from_js(event["data"]);
    if (frame.size() < 3) return;
    if (frame[0] == 2) {
        log("room", std::string("peer ") + peer_id + " presence " + (frame[1] ? "on" : "off") + " for a room token");
        return;
    }
    const std::size_t token_size = (frame[1] << 8) | frame[2];
    if (frame[0] != 1 || frame.size() < 3 + token_size) return;
    if (std::string(frame.begin() + 3, frame.begin() + 3 + token_size) != app->room_token) {
        log("room", "frame for another room, ignored");
        return;
    }
    const bytes action(frame.begin() + 3 + token_size, frame.end());
    if (action.size() < 36 || action[0] != 2 || (action[1] & 15) != 0) {
        log("action", "non-inline or invalid action frame, ignored in this probe");
        return;
    }
    std::string type(action.begin() + 2, action.begin() + 34);
    type.erase(std::find(type.begin(), type.end(), '\0'), type.end());
    const int format = action[1] >> 4;
    const std::size_t meta = (action[34] << 8) | action[35];
    const std::string payload(action.begin() + 36 + meta, action.end());
    auto& p = app->peers[peer_id];
    if (type == "@_hsready") {
        p.remote_ready = true;
        log("room", "got @_hsready from " + peer_id);
        activate(peer_id);
    } else if (type == "@_ping") {
        send_action(peer_id, "@_pong", text_format, "");
    } else if (type == "@_leave") {
        log("room", "peer left: " + peer_id);
    } else {
        log("action", "received " + type + " (" + (format == json_format ? "json" : format == text_format ? "text" : "binary") +
                          ") from " + peer_id + ": " + payload);
        if (type == "chat" && payload.rfind("echo:", 0) != 0) send_action(peer_id, "chat", text_format, "echo: " + payload);
    }
}

// ---- WebRTC through the browser's RTCPeerConnection ---------------------------------------------------

val guard(val promise, const std::string& what) { return val::module_property("guard")(promise, val(what)); }

void send_candidate(std::string peer_id, std::string candidate);

val publish_candidate(std::string peer_id, std::string candidate) {
    const std::string sealed = (co_await encrypt(candidate)).as<std::string>();
    const auto& p = app->peers[peer_id];
    publish(peer_topic(peer_id), json::serialize(json::object{{"peerId", app->self_id}, {"offerId", p.offer_id}, {"candidate", sealed}}),
            "candidate");
    co_return val::undefined();
}

void send_candidate(std::string peer_id, std::string candidate) {
    auto& p = app->peers[peer_id];
    if (!p.signal_sent) {
        p.outgoing_candidates.push_back(candidate);
        return;
    }
    guard(publish_candidate(peer_id, candidate), "publish candidate");
}

void on_ice_candidate(std::string peer_id, val event) {
    if (event["candidate"].isNull()) return;
    send_candidate(peer_id, val::global("JSON").call<std::string>("stringify", event["candidate"]));
}

void on_connection_state(std::string peer_id, val) {
    log("webrtc", "connection with " + peer_id + ": " + app->peers[peer_id].pc["connectionState"].as<std::string>());
}

void setup_channel(const std::string& peer_id, val channel) {
    auto& p = app->peers[peer_id];
    p.channel = channel;
    channel.set("binaryType", val("arraybuffer"));
    channel.set("onopen", val::module_property("bind")(val("on_channel_open"), val(peer_id)));
    channel.set("onmessage", val::module_property("bind")(val("on_channel_message"), val(peer_id)));
}

void on_data_channel(std::string peer_id, val event) { setup_channel(peer_id, event["channel"]); }

val make_peer_connection(const std::string& peer_id) {
    val servers = val::array();
    for (const char* url : {"stun:stun.l.google.com:19302", "stun:stun1.l.google.com:19302", "stun:stun2.l.google.com:19302",
                            "stun:stun.cloudflare.com:3478"}) {
        val server = val::object();
        server.set("urls", val(url));
        servers.call<void>("push", server);
    }
    val config = val::object();
    config.set("iceServers", servers);
    val pc = val::global("RTCPeerConnection").new_(config);
    pc.set("onicecandidate", val::module_property("bind")(val("on_ice_candidate"), val(peer_id)));
    pc.set("onconnectionstatechange", val::module_property("bind")(val("on_connection_state"), val(peer_id)));
    pc.set("ondatachannel", val::module_property("bind")(val("on_data_channel"), val(peer_id)));
    return pc;
}

void flush_signals(const std::string& peer_id) {
    auto& p = app->peers[peer_id];
    p.signal_sent = true;
    for (auto& c : std::exchange(p.outgoing_candidates, {})) guard(publish_candidate(peer_id, c), "publish candidate");
}

val apply_incoming_candidates(std::string peer_id) {
    auto& p = app->peers[peer_id];
    for (auto& c : std::exchange(p.incoming_candidates, {})) co_await p.pc.call<val>("addIceCandidate", c);
    co_return val::undefined();
}

val start_offer(std::string peer_id) {
    auto& p = app->peers[peer_id];
    if (!p.pc.isUndefined()) co_return val::undefined();
    p.initiator = true;
    p.offer_id = gen_id(12);
    p.pc = make_peer_connection(peer_id);
    setup_channel(peer_id, p.pc.call<val>("createDataChannel", val("data")));
    co_await p.pc.call<val>("setLocalDescription");
    const std::string sealed = (co_await encrypt(p.pc["localDescription"]["sdp"].as<std::string>())).as<std::string>();
    publish(peer_topic(peer_id), json::serialize(json::object{{"peerId", app->self_id}, {"offerId", p.offer_id}, {"offer", sealed}}),
            "offer for " + peer_id);
    flush_signals(peer_id);
    co_return val::undefined();
}

val handle_offer(std::string peer_id, std::string offer_id, std::string sealed) {
    auto& p = app->peers[peer_id];
    if (!p.pc.isUndefined()) co_return val::undefined();  // already answering or connected
    p.offer_id = offer_id;
    p.pc = make_peer_connection(peer_id);
    val sdp = co_await decrypt(sealed);
    if (sdp.isNull()) {
        log("signal", "could not decrypt the offer from " + peer_id + " (wrong password?)");
        co_return val::undefined();
    }
    val description = val::object();
    description.set("type", val("offer"));
    description.set("sdp", sdp);
    co_await p.pc.call<val>("setRemoteDescription", description);
    co_await apply_incoming_candidates(peer_id);
    co_await p.pc.call<val>("setLocalDescription");
    const std::string answer = (co_await encrypt(p.pc["localDescription"]["sdp"].as<std::string>())).as<std::string>();
    publish(peer_topic(peer_id), json::serialize(json::object{{"peerId", app->self_id}, {"answer", answer}, {"offerId", offer_id}}),
            "answer for " + peer_id);
    flush_signals(peer_id);
    co_return val::undefined();
}

val handle_answer(std::string peer_id, std::string sealed) {
    auto& p = app->peers[peer_id];
    if (!p.initiator || p.answered) co_return val::undefined();
    p.answered = true;
    val sdp = co_await decrypt(sealed);
    if (sdp.isNull()) co_return val::undefined();
    val description = val::object();
    description.set("type", val("answer"));
    description.set("sdp", sdp);
    co_await p.pc.call<val>("setRemoteDescription", description);
    co_await apply_incoming_candidates(peer_id);
    log("signal", "applied the answer from " + peer_id);
    co_return val::undefined();
}

val handle_candidate(std::string peer_id, std::string sealed) {
    val text = co_await decrypt(sealed);
    if (text.isNull()) co_return val::undefined();
    val candidate = val::global("JSON").call<val>("parse", text);
    auto& p = app->peers[peer_id];
    if (p.pc.isUndefined() || p.pc["remoteDescription"].isNull()) {
        p.incoming_candidates.push_back(candidate);
    } else {
        co_await p.pc.call<val>("addIceCandidate", candidate);
    }
    co_return val::undefined();
}

// ---- Signaling: what arrives on the root and self topics ----------------------------------------------

void handle_signal(const std::string& topic, const std::string& content) {
    if (topic != app->root_topic && topic != app->self_topic) return;
    boost::system::error_code ec;
    const json::value parsed = json::parse(content, ec);
    const auto* payload = ec ? nullptr : parsed.if_object();
    if (!payload) return;
    auto get = [payload](const char* key) -> std::string {
        const auto* v = payload->if_contains(key);
        const auto* s = v ? v->if_string() : nullptr;
        return s ? std::string(*s) : std::string();
    };
    const std::string peer_id = get("peerId");
    if (peer_id.empty() || peer_id == app->self_id) return;
    const std::string offer = get("offer"), answer = get("answer"), candidate = get("candidate"), offer_id = get("offerId");
    auto& p = app->peers[peer_id];
    if (p.active) return;
    if (offer.empty() && answer.empty() && candidate.empty()) {
        if (!p.pc.isUndefined()) return;
        if (app->self_id < peer_id) {
            log("signal", "announcement from " + peer_id + ": our id is lower, so we offer");
            guard(start_offer(peer_id), "offer");
        } else if (now_ms() - p.last_counter_announce >= 1000) {
            p.last_counter_announce = now_ms();
            log("signal", "announcement from " + peer_id + ": their id is lower, asking them to offer");
            publish(peer_topic(peer_id), json::serialize(json::object{{"peerId", app->self_id}}), "counter-announcement");
        }
        return;
    }
    if (!offer.empty()) {
        log("signal", "offer from " + peer_id);
        guard(handle_offer(peer_id, offer_id, offer), "answer");
    } else if (!answer.empty()) {
        guard(handle_answer(peer_id, answer), "apply answer");
    } else if (!candidate.empty()) {
        guard(handle_candidate(peer_id, candidate), "apply candidate");
    }
}

void on_relay_open(int index, val) {
    auto& r = app->relays[index];
    r.open = true;
    std::set<int> kinds{topic_kind(app->root_topic), topic_kind(app->self_topic)};
    json::array kind_list(kinds.begin(), kinds.end());
    const auto since = static_cast<std::int64_t>(std::time(nullptr));
    r.socket.call<void>("send", val(json::serialize(json::array{
                                    "REQ", r.sub_id, json::object{{"kinds", kind_list}, {"since", since}, {"#x", json::array{app->root_topic, app->self_topic}}}})));
    log("nostr", "connected to " + r.url + ", subscribed to the root and self topics");
    announce_tick(reinterpret_cast<void*>(static_cast<std::intptr_t>(index)));
}

void on_relay_close(int index, val) {
    app->relays[index].open = false;
    log("nostr", "relay closed: " + app->relays[index].url);
}

void on_relay_message(int index, val event) {
    const std::string data = event["data"].as<std::string>();
    boost::system::error_code ec;
    const json::value parsed = json::parse(data, ec);
    const auto* message = ec ? nullptr : parsed.if_array();
    if (!message || message->empty() || !message->at(0).is_string()) return;
    const std::string type(message->at(0).get_string());
    if (type != "EVENT") {
        if (type == "NOTICE" || type == "CLOSED" || (type == "OK" && message->size() > 2 && message->at(2) == false))
            log("nostr", app->relays[index].url + " says " + data.substr(0, 200));
        return;
    }
    if (message->size() < 3 || !message->at(2).is_object()) return;
    const auto& ev = message->at(2).get_object();
    const auto* id = ev.if_contains("id");
    if (!id || !id->is_string() || !app->seen_events.insert(std::string(id->get_string())).second) return;
    const auto* tags = ev.if_contains("tags");
    const auto* content = ev.if_contains("content");
    if (!tags || !tags->is_array() || !content || !content->is_string()) return;
    for (const auto& tag : tags->get_array()) {
        const auto* pair = tag.if_array();
        if (pair && pair->size() >= 2 && pair->at(0) == "x" && pair->at(1).is_string()) {
            handle_signal(std::string(pair->at(1).get_string()), std::string(content->get_string()));
            return;
        }
    }
}

const std::vector<std::string> default_relays = {
    "0x-nostr-relay.fly.dev", "bendernostur.duckdns.org:8443", "cdn.satellite.earth", "nos.lol", "nostr.chaima.info",
    "nostr.christiansass.de", "nostr.rblb.it", "nostr.red5d.dev", "nostr.robosats.org", "nostr.stakey.net",
    "nostrcity-club.fly.dev", "relay-dev.gulugulu.moe", "relay.aarpia.com", "relay.agentry.com", "relay.bitmacro.cloud",
    "relay.bullishbounty.com", "relay.degmods.com", "relay.flashapp.me", "relay.grigic.org", "relay.hackshed.dev",
    "relay.kaleidoswap.com", "relay.layer.systems", "relay.nostr.blockhenge.com", "relay.nostr.dev.br", "relay.nostrmap.net",
    "relay.novospes.com", "relay.piazza.today", "relay.routstr.com", "testr.nymble.world"};

val join(std::string app_id, std::string room_id, std::string password, std::string self_id) {
    app = new state();  // lives as long as the page
    app->cfg = {app_id, room_id, password, {}};
    app->self_id = self_id.empty() ? gen_id(20) : self_id;
    app->root_plain = "Trystero@" + app_id + "@" + room_id;
    app->root_topic = sha1_topic(app->root_plain);
    app->self_topic = sha1_topic(app->root_plain + "@" + app->self_id);
    const auto token = sha256("Trystero:" + app_id + ":" + room_id);
    app->room_token = to_hex(token.data(), token.size());
    log("room", "self id " + app->self_id + ", root topic " + app->root_topic + ", self topic " + app->self_topic);

    app->secp = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    auto secret = random_bytes(32);
    while (!secp256k1_keypair_create(app->secp, &app->keypair, secret.data())) secret = random_bytes(32);
    secp256k1_xonly_pubkey xonly;
    secp256k1_keypair_xonly_pub(app->secp, &xonly, nullptr, &app->keypair);
    std::array<std::uint8_t, 32> pub{};
    secp256k1_xonly_pubkey_serialize(app->secp, pub.data(), &xonly);
    app->pubkey = to_hex(pub.data(), pub.size());

    const auto key_bytes = sha256(password + ":" + app_id + ":" + room_id);
    val usages = val::array();
    usages.call<void>("push", val("encrypt"));
    usages.call<void>("push", val("decrypt"));
    app->key = co_await val::global("crypto")["subtle"].call<val>(
        "importKey", val("raw"), to_js(bytes(key_bytes.begin(), key_bytes.end())), val("AES-GCM"), val(false), usages);

    auto urls = shuffle(default_relays, str_to_num(app_id, 9007199254740991.0));
    urls.resize(5);
    for (const auto& url : urls) app->relays.push_back(relay{"wss://" + url});
    std::string list;
    for (const auto& r : app->relays) list += " " + r.url;
    log("nostr", "relays derived from the app id:" + list);
    for (std::size_t i = 0; i < app->relays.size(); ++i) {
        auto& r = app->relays[i];
        r.socket = val::global("WebSocket").new_(val(r.url));
        r.socket.set("onopen", val::module_property("bind")(val("on_relay_open"), val(static_cast<int>(i))));
        r.socket.set("onmessage", val::module_property("bind")(val("on_relay_message"), val(static_cast<int>(i))));
        r.socket.set("onclose", val::module_property("bind")(val("on_relay_close"), val(static_cast<int>(i))));
    }
    co_return val::undefined();
}

void start(std::string app_id, std::string room_id, std::string password, std::string self_id) {
    guard(join(app_id, room_id, password, self_id), "join");
}

}  // namespace

EMSCRIPTEN_BINDINGS(trystero_probe) {
    emscripten::function("start", &start);
    emscripten::function("on_relay_open", &on_relay_open);
    emscripten::function("on_relay_close", &on_relay_close);
    emscripten::function("on_relay_message", &on_relay_message);
    emscripten::function("on_ice_candidate", &on_ice_candidate);
    emscripten::function("on_connection_state", &on_connection_state);
    emscripten::function("on_data_channel", &on_data_channel);
    emscripten::function("on_channel_open", &on_channel_open);
    emscripten::function("on_channel_message", &on_channel_message);
}
