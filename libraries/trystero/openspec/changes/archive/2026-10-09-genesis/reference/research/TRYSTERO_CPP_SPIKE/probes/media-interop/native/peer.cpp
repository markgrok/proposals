// Throwaway feasibility probe: a native libdatachannel peer that answers a browser offer received over a
// local WebSocket, echoes the data channel, counts the RTP packets of every incoming media track and
// reflects them back to the browser under its own SSRC, so media flows both ways.
#include <rtc/rtc.hpp>
#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace {

struct track_stats {
    std::shared_ptr<rtc::Track> track;  // libdatachannel holds only a weak_ptr; a track nobody holds is rejected in the answer
    std::string kind;
    std::atomic<int> payload_type{-1};  // of the last RTP packet: the codec the browser actually chose
    std::atomic<int> inner_type{-1};    // when that codec is RED (RFC 2198), the payload type it carries
    std::atomic<std::uint64_t> rtp{0};
    std::atomic<std::uint64_t> bytes{0};
    std::uint32_t local_ssrc = 0;  // the SSRC this peer sends the reflected stream under, announced in the answer
    std::atomic<std::uint64_t> reflected{0};
};

std::string codec_of(const track_stats& entry) {
    const int type = entry.payload_type.load();
    if (type < 0) return "none yet";
    const auto media = entry.track->description();
    const auto name = [&media](int pt) {
        const auto* map = media.rtpMap(pt);
        return (map ? map->format : std::string("?")) + " pt " + std::to_string(pt);
    };
    const int inner = entry.inner_type.load();
    return name(type) + (inner >= 0 ? " carrying " + name(inner) : std::string());
}

}  // namespace

int main() {
    rtc::InitLogger(rtc::LogLevel::Warning);
    rtc::WebSocketServer::Configuration config;
    config.port = 8900;
    config.bindAddress = "127.0.0.1";
    rtc::WebSocketServer server(config);

    std::mutex mutex;
    std::vector<std::shared_ptr<rtc::PeerConnection>> peers;
    std::vector<std::shared_ptr<rtc::WebSocket>> sockets;  // libdatachannel closes a socket nobody holds

    server.onClient([&](std::shared_ptr<rtc::WebSocket> ws) {
        std::cout << "native: signaling client connected" << std::endl;
        auto pc = std::make_shared<rtc::PeerConnection>(rtc::Configuration{});
        auto stats = std::make_shared<std::map<std::string, std::shared_ptr<track_stats>>>();
        auto channel = std::make_shared<std::shared_ptr<rtc::DataChannel>>();
        {
            std::lock_guard lock(mutex);
            peers.push_back(pc);
            sockets.push_back(ws);
        }
        std::weak_ptr<rtc::WebSocket> weak_ws = ws;
        pc->onGatheringStateChange([pc_weak = std::weak_ptr(pc), weak_ws](rtc::PeerConnection::GatheringState state) {
            if (state != rtc::PeerConnection::GatheringState::Complete) return;
            auto pc = pc_weak.lock();
            auto ws = weak_ws.lock();
            if (!pc || !ws) return;
            const auto description = pc->localDescription();
            ws->send(nlohmann::json{{"type", description->typeString()}, {"sdp", std::string(*description)}}.dump());
            std::cout << "native: answer sent (" << std::string(*description).size() << " bytes of SDP)" << std::endl;
            const std::string sdp(*description);
            for (std::size_t at = 0; (at = sdp.find("\na=", at)) != std::string::npos; ++at) {
                const auto line = sdp.substr(at + 1, sdp.find('\r', at) - at - 1);
                if (line.starts_with("a=mid") || line == "a=sendrecv" || line == "a=recvonly" || line == "a=sendonly" || line == "a=inactive") std::cout << "native:   answer " << line << std::endl;
            }
        });
        pc->onStateChange([](rtc::PeerConnection::State state) { std::cout << "native: peer connection " << state << std::endl; });
        pc->onDataChannel([channel](std::shared_ptr<rtc::DataChannel> dc) {
            std::cout << "native: data channel \"" << dc->label() << "\" opened by the browser" << std::endl;
            *channel = dc;
            dc->onMessage([dc](rtc::message_variant message) {
                if (const auto* text = std::get_if<std::string>(&message)) {
                    std::cout << "native: received \"" << *text << "\"" << std::endl;
                    dc->send("echo from native libdatachannel: " + *text);
                }
            });
        });
        pc->onTrack([stats](std::shared_ptr<rtc::Track> track) {
            auto entry = std::make_shared<track_stats>();
            entry->track = track;
            entry->kind = track->description().type();
            // Keep only the codecs this peer could depacketize (opus, VP8); otherwise the answer accepts every codec
            // offered and Chrome wraps VP8 in RED/ULPFEC and probes with RTX padding.
            auto media = track->description();
            for (const int pt : media.payloadTypes()) {
                if (!media.hasPayloadType(pt)) continue;  // removing a codec also removes its RTX, and rtpMap() throws
                const auto* map = media.rtpMap(pt);
                if (map && map->format != "opus" && map->format != "VP8") media.removeRtpMap(pt);
            }
            for (const auto& attribute : media.attributes()) {
                if (attribute.starts_with("msid:")) media.removeAttribute(attribute);  // reciprocate() echoes the offerer's msid
            }
            entry->local_ssrc = entry->kind == "audio" ? 0x0A0D10 : 0x0B1DE0;
            media.addSSRC(entry->local_ssrc, "native-cname", "native-stream", "native-" + entry->kind);
            track->setDescription(std::move(media));
            (*stats)[track->mid()] = entry;
            std::cout << "native: track mid=" << track->mid() << " kind=" << entry->kind << std::endl;
            track->onMessage([raw = entry.get()](rtc::message_variant message) {
                const auto* packet = std::get_if<rtc::binary>(&message);
                if (!packet || packet->size() < 12) return;
                const auto second = std::to_integer<int>((*packet)[1]);
                if (second >= 192 && second <= 223) return;  // RTCP packet types 192-223 (RFC 5761 demultiplexing)
                raw->payload_type = second & 0x7f;
                const auto first = std::to_integer<int>((*packet)[0]);
                std::size_t payload = 12 + 4 * (first & 0x0f);
                if ((first & 0x10) && packet->size() >= payload + 4) {
                    payload += 4 + 4 * ((std::to_integer<std::size_t>((*packet)[payload + 2]) << 8) | std::to_integer<std::size_t>((*packet)[payload + 3]));
                }
                if (raw->rtp == 0) {
                    std::cout << "native: first " << raw->kind << " packet, " << packet->size() << " bytes, header ends at " << payload << ":";
                    for (std::size_t i = 0; i < std::min<std::size_t>(packet->size(), 40); ++i) std::cout << ' ' << std::hex << std::to_integer<int>((*packet)[i]) << std::dec;
                    std::cout << std::endl;
                }
                const auto media = raw->track->description();  // probe only: copies the description per packet
                const auto* map = media.rtpMap(second & 0x7f);
                if (map && map->format == "red" && packet->size() > payload) {
                    raw->inner_type = std::to_integer<int>((*packet)[payload]) & 0x7f;
                }
                raw->rtp += 1;
                raw->bytes += packet->size();
                // Reflect the packet to the browser under our own SSRC: native media sending, encrypted by libdatachannel.
                auto copy = *packet;
                for (int i = 0; i < 4; ++i) copy[8 + i] = std::byte((raw->local_ssrc >> (24 - 8 * i)) & 0xff);
                if (raw->track->isOpen() && raw->track->send(copy)) raw->reflected += 1;
            });
        });
        ws->onMessage([pc](rtc::message_variant message) {
            const auto* text = std::get_if<std::string>(&message);
            if (!text) return;
            const auto json = nlohmann::json::parse(*text);
            std::cout << "native: received " << json["type"].get<std::string>() << " (" << json["sdp"].get<std::string>().size() << " bytes of SDP)" << std::endl;
            const auto sdp = json["sdp"].get<std::string>();
            for (std::size_t at = 0; (at = sdp.find("\na=", at)) != std::string::npos; ++at) {
                const auto line = sdp.substr(at + 1, sdp.find('\r', at) - at - 1);
                if (line.starts_with("a=mid") || line == "a=sendrecv" || line == "a=recvonly" || line == "a=sendonly" || line == "a=inactive" || line.starts_with("a=ssrc-group") ) std::cout << "native:   offer " << line << std::endl;
            }
            pc->setRemoteDescription(rtc::Description(json["sdp"].get<std::string>(), json["type"].get<std::string>()));
        });
        std::thread([stats, channel, pc]() {
            for (int i = 0; i < 120; ++i) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                auto dc = *channel;
                if (!dc || !dc->isOpen()) continue;
                std::string line = "native counts:";
                for (const auto& [mid, entry] : *stats) {
                    line += " " + entry->kind + "/" + codec_of(*entry) + " " + std::to_string(entry->rtp.load()) + " RTP packets in, " + std::to_string(entry->reflected.load()) + " reflected (" + std::to_string(entry->bytes.load() / 1024) + " KiB);";
                }
                dc->send(line);
            }
        }).detach();
    });

    std::cout << "native: signaling on ws://127.0.0.1:" << server.port() << std::endl;
    std::this_thread::sleep_for(std::chrono::minutes(3));
    return 0;
}
