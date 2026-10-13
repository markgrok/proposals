// Throwaway feasibility probe: C++20 compiled with Emscripten drives the browser's own WebRTC stack
// through emscripten::val: a data channel, camera and microphone tracks, and an offer sent to a
// native libdatachannel peer over a local WebSocket.
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <string>

using emscripten::val;

namespace {

val peer = val::undefined();
val channel = val::undefined();
val socket = val::undefined();

void log(const std::string& text) {
    val::global("console").call<void>("log", val(text));
    val::global("document").call<val>("getElementById", val("log")).call<void>("append", val(text + "\n"));
}

void on_channel_open(val) {
    log("browser: data channel \"data\" open, sending a message");
    channel.call<void>("send", val(std::string("hello from C++ compiled to WebAssembly")));
}

void on_channel_message(val event) {
    log("browser: received \"" + event["data"].as<std::string>() + "\"");
}

void on_socket_message(val event) {
    val answer = val::global("JSON").call<val>("parse", event["data"]);
    log("browser: received " + answer["type"].as<std::string>() + ", applying it");
    val::module_property("logFailure")(peer.call<val>("setRemoteDescription", answer), val("setRemoteDescription"));
}

void on_state_change(val) {
    log("browser: connection " + peer["connectionState"].as<std::string>());
}

val run() {
    val config = val::object();
    config.set("iceServers", val::array());
    peer = val::global("RTCPeerConnection").new_(config);
    peer.set("onconnectionstatechange", val::module_property("on_state_change"));
    val::global("window").set("probePeer", peer);  // probe only: lets the test driver read getStats()

    channel = peer.call<val>("createDataChannel", val("data"));
    channel.set("binaryType", val("arraybuffer"));
    channel.set("onopen", val::module_property("on_channel_open"));
    channel.set("onmessage", val::module_property("on_channel_message"));

    val constraints = val::object();
    constraints.set("audio", true);
    constraints.set("video", true);
    val stream = co_await val::global("navigator")["mediaDevices"].call<val>("getUserMedia", constraints);
    val tracks = stream.call<val>("getTracks");
    const int count = tracks["length"].as<int>();
    for (int i = 0; i < count; ++i) {
        val track = tracks[i];
        peer.call<val>("addTrack", track, stream);
        log("browser: added " + track["kind"].as<std::string>() + " track \"" + track["label"].as<std::string>() + "\"");
    }

    val offer = co_await peer.call<val>("createOffer");
    co_await peer.call<val>("setLocalDescription", offer);
    co_await val::module_property("waitIceComplete")(peer);
    log("browser: offer ready with its ICE candidates");

    socket = val::global("WebSocket").new_(val("ws://127.0.0.1:8900"));
    socket.set("onmessage", val::module_property("on_socket_message"));
    co_await val::module_property("waitOpen")(socket);
    socket.call<void>("send", val::global("JSON").call<val>("stringify", peer["localDescription"]));
    log("browser: offer sent over the signaling socket");
    co_return val::undefined();
}

}  // namespace

EMSCRIPTEN_BINDINGS(probe) {
    emscripten::function("on_channel_open", &on_channel_open);
    emscripten::function("on_channel_message", &on_channel_message);
    emscripten::function("on_socket_message", &on_socket_message);
    emscripten::function("on_state_change", &on_state_change);
}

int main() {
    run();
    return 0;
}
