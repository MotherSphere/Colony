#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace colony::programs::archive::crypto
{

/**
 * @brief Result of AES-256-GCM encryption, containing ciphertext and tag.
 */
struct GcmCiphertext
{
    std::vector<std::uint8_t> ciphertext;
    std::array<std::uint8_t, 16> tag{};
};

/**
 * @brief Encrypt plaintext with AES-256-GCM.
 *
 * @param key          256-bit key material.
 * @param nonce        Unique nonce (recommended 12 bytes).
 * @param plaintext    Data to encrypt.
 * @param associated   Optional additional authenticated data.
 * @return GcmCiphertext  Ciphertext and authentication tag.
 */
[[nodiscard]] GcmCiphertext aes256gcm_encrypt(
    std::span<const std::uint8_t> key,
    std::span<const std::uint8_t> nonce,
    std::span<const std::uint8_t> plaintext,
    std::span<const std::uint8_t> associated = {});

/**
 * @brief Decrypt ciphertext with AES-256-GCM.
 *
 * @param key          256-bit key material.
 * @param nonce        Nonce used for encryption.
 * @param ciphertext   Ciphertext buffer.
 * @param tag          Authentication tag.
 * @param associated   Additional authenticated data supplied during encryption.
 * @return std::vector<std::uint8_t>  Decrypted plaintext.
 *
 * @throws std::runtime_error if authentication fails or parameters are invalid.
 */
[[nodiscard]] std::vector<std::uint8_t> aes256gcm_decrypt(
    std::span<const std::uint8_t> key,
    std::span<const std::uint8_t> nonce,
    std::span<const std::uint8_t> ciphertext,
    std::span<const std::uint8_t> tag,
    std::span<const std::uint8_t> associated = {});

} // namespace colony::programs::archive::crypto
