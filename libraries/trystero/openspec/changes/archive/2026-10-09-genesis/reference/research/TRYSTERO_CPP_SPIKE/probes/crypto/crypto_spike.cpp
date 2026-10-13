// Spike: BIP-340 via libsecp256k1 and SHA-1/SHA-256 via Boost.Hash2 under Emscripten.
#include <boost/hash2/sha1.hpp>
#include <boost/hash2/sha2.hpp>
#include <secp256k1.h>
#include <secp256k1_extrakeys.h>
#include <secp256k1_schnorrsig.h>
#include <sys/random.h>
#include <cstdio>
#include <cstring>

int main() {
    boost::hash2::sha2_256 h256;
    h256.update("abc", 3);
    auto d256 = h256.result();
    boost::hash2::sha1_160 h1;
    h1.update("abc", 3);
    auto d1 = h1.result();
    std::printf("sha256(abc)[0]=%02x sha1(abc)[0]=%02x\n", d256[0], d1[0]);

    unsigned char seckey[32];
    unsigned char aux[32];
    if (getentropy(seckey, 32) != 0 || getentropy(aux, 32) != 0) {
        std::puts("getentropy failed");
        return 1;
    }
    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    secp256k1_keypair kp;
    if (!secp256k1_keypair_create(ctx, &kp, seckey)) {
        std::puts("bad key");
        return 1;
    }
    secp256k1_xonly_pubkey pub;
    secp256k1_keypair_xonly_pub(ctx, &pub, nullptr, &kp);
    unsigned char sig[64];
    secp256k1_schnorrsig_sign32(ctx, sig, d256.data(), &kp, aux);
    int ok = secp256k1_schnorrsig_verify(ctx, sig, d256.data(), 32, &pub);
    std::printf("bip340 verify=%d\n", ok);
    secp256k1_context_destroy(ctx);
    return ok ? 0 : 1;
}
