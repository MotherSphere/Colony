#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "programs/archive/crypto/aes_gcm.hpp"
#include "programs/archive/crypto/argon2id.hpp"
#include "programs/archive/crypto/hkdf.hpp"

#include <array>
#include <span>
#include <string>
#include <vector>

using namespace colony::programs::archive::crypto;

namespace
{
Argon2idParams fast_params()
{
    Argon2idParams params;
    params.opslimit = 3;
    params.memlimit = 1u << 15; // 32 KiB for tests
    params.output_length = 32;
    return params;
}
}

TEST_CASE("Argon2id derives deterministic key material")
{
    const std::string password{"correct horse battery staple"};
    std::array<std::uint8_t, kArgon2idSaltSize> salt{};
    for (std::size_t i = 0; i < salt.size(); ++i)
    {
        salt[i] = static_cast<std::uint8_t>(i);
    }

    const auto key1 = derive_argon2id_key(password, salt, fast_params());
    const auto key2 = derive_argon2id_key(password, salt, fast_params());
    CHECK(key1 == key2);
    CHECK(key1.size() == 32);
}

TEST_CASE("HKDF derives unique keys")
{
    std::array<std::uint8_t, 32> ikm{};
    for (std::size_t i = 0; i < ikm.size(); ++i)
    {
        ikm[i] = static_cast<std::uint8_t>(i * 3);
    }
    std::array<std::uint8_t, kArgon2idSaltSize> salt{};
    salt.fill(42);
    const std::string info_str{"unit-test-info"};
    const auto info = std::span<const std::uint8_t>{reinterpret_cast<const std::uint8_t*>(info_str.data()), info_str.size()};

    const auto key_a = hkdf_sha256(ikm, salt, info, 32);
    salt[0] ^= 0xFF;
    const auto key_b = hkdf_sha256(ikm, salt, info, 32);
    CHECK(key_a != key_b);
}

TEST_CASE("AES-256-GCM round trips and authenticates")
{
    std::array<std::uint8_t, 32> key{};
    std::array<std::uint8_t, 12> nonce{};
    for (std::size_t i = 0; i < key.size(); ++i)
    {
        key[i] = static_cast<std::uint8_t>(i * 7);
        if (i < nonce.size())
        {
            nonce[i] = static_cast<std::uint8_t>(0xA5 ^ i);
        }
    }

    const std::string plaintext_str{"Sensitive payload"};
    const std::string aad_str{"metadata"};
    const auto aad = std::span<const std::uint8_t>{reinterpret_cast<const std::uint8_t*>(aad_str.data()), aad_str.size()};

    const auto cipher = aes256gcm_encrypt(key, nonce, std::span<const std::uint8_t>{
        reinterpret_cast<const std::uint8_t*>(plaintext_str.data()), plaintext_str.size()}, aad);

    const auto decrypted = aes256gcm_decrypt(key, nonce, cipher.ciphertext, cipher.tag, aad);
    const std::string decrypted_str{reinterpret_cast<const char*>(decrypted.data()), decrypted.size()};
    CHECK(decrypted_str == plaintext_str);

    auto corrupted_cipher = cipher;
    corrupted_cipher.ciphertext[0] ^= 0xFF;
    CHECK_THROWS([&]() {
        (void)aes256gcm_decrypt(key, nonce, corrupted_cipher.ciphertext, corrupted_cipher.tag, aad);
    }());
}
