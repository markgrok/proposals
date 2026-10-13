// Spike: AES-GCM (16-byte IV, as Trystero uses) and SHA-256 through WebCrypto from C++ with val co_await.
#include <emscripten/val.h>
#include <cstdio>
#include <string>

using emscripten::val;

val run() {
    val subtle = val::global("crypto")["subtle"];
    val u8 = val::global("Uint8Array");
    val pw = val::global("TextEncoder").new_().call<val>("encode", std::string("password:app:room"));
    val digest = co_await subtle.call<val>("digest", std::string("SHA-256"), pw);
    val usages = val::array();
    usages.call<void>("push", std::string("encrypt"));
    usages.call<void>("push", std::string("decrypt"));
    val key = co_await subtle.call<val>("importKey", std::string("raw"), digest, std::string("AES-GCM"), false, usages);
    val iv = u8.new_(16);
    val::global("crypto").call<val>("getRandomValues", iv);
    val alg = val::object();
    alg.set("name", std::string("AES-GCM"));
    alg.set("iv", iv);
    val plain = val::global("TextEncoder").new_().call<val>("encode", std::string("sdp offer"));
    val ct = co_await subtle.call<val>("encrypt", alg, key, plain);
    val pt = co_await subtle.call<val>("decrypt", alg, key, ct);
    std::string back = val::global("TextDecoder").new_().call<std::string>("decode", pt);
    std::printf("ciphertext bytes=%d roundtrip=%s\n", ct["byteLength"].as<int>(), back.c_str());
    co_return val::undefined();
}

int main() {
    run();
    std::puts("main returned; promise chain continues on the event loop");
    return 0;
}
