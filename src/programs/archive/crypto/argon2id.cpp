#include "programs/archive/crypto/argon2id.hpp"

#include <stdexcept>

namespace colony::programs::archive::crypto
{
namespace
{
void throw_if_error(int status)
{
    if (status != 0)
    {
        throw std::runtime_error{"libsodium failed to derive key material"};
    }
}
}

Argon2idParams::Argon2idParams()
    : opslimit{crypto_pwhash_OPSLIMIT_MODERATE}
    , memlimit{crypto_pwhash_MEMLIMIT_MODERATE}
{
}

void ensure_sodium_ready()
{
    static const int rc = sodium_init();
    if (rc < 0)
    {
        throw std::runtime_error{"libsodium initialisation failed"};
    }
}

std::vector<std::uint8_t> derive_argon2id_key(
    std::string_view password,
    std::span<const std::uint8_t> salt,
    const Argon2idParams& params)
{
    ensure_sodium_ready();

    if (salt.size() != kArgon2idSaltSize)
    {
        throw std::invalid_argument{"Argon2id salt must be crypto_pwhash_SALTBYTES bytes"};
    }

    std::vector<std::uint8_t> key(params.output_length);
    const int status = crypto_pwhash(
        key.data(),
        static_cast<unsigned long long>(key.size()),
        password.data(),
        password.size(),
        salt.data(),
        params.opslimit,
        params.memlimit,
        crypto_pwhash_ALG_ARGON2ID13);

    throw_if_error(status);
    return key;
}

void fill_random_bytes(std::span<std::uint8_t> destination)
{
    ensure_sodium_ready();
    randombytes_buf(destination.data(), destination.size());
}

} // namespace colony::programs::archive::crypto
