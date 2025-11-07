#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <sodium.h>

namespace colony::programs::archive::crypto
{

/**
 * @brief Parameters for deriving keys with Argon2id.
 *
 * The defaults follow libsodium's "moderate" profile and return a
 * 256-bit key suitable for symmetric encryption. The salt must be
 * crypto_pwhash_SALTBYTES bytes long.
 */
struct Argon2idParams
{
    std::size_t output_length{32};
    std::uint32_t opslimit;
    std::size_t memlimit;

    Argon2idParams();
};

inline constexpr std::size_t kArgon2idSaltSize = crypto_pwhash_SALTBYTES;

/**
 * @brief Derive a deterministic key from a password using Argon2id.
 *
 * @param password     UTF-8 master password provided by the user.
 * @param salt         Random salt; must be exactly kArgon2idSaltSize bytes.
 * @param params       Tuning parameters controlling cost and output length.
 * @return std::vector<std::uint8_t>  Derived key material.
 *
 * @throws std::invalid_argument if the salt has an unexpected size.
 * @throws std::runtime_error if libsodium fails to derive the key.
 */
[[nodiscard]] std::vector<std::uint8_t> derive_argon2id_key(
    std::string_view password,
    std::span<const std::uint8_t> salt,
    const Argon2idParams& params = {});

/**
 * @brief Ensure libsodium is initialised for cryptographic operations.
 *
 * The function may be called multiple times and is thread-safe. A
 * std::runtime_error is raised if initialisation fails.
 */
void ensure_sodium_ready();

/**
 * @brief Fill the destination buffer with cryptographically secure random bytes.
 */
void fill_random_bytes(std::span<std::uint8_t> destination);

} // namespace colony::programs::archive::crypto
