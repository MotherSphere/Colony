#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace colony::programs::archive::crypto
{

/**
 * @brief Derive key material using HKDF-SHA256.
 *
 * @param input_key_material  Base key material (IKM).
 * @param salt                Optional salt; may be empty for default behaviour.
 * @param info                Context/application specific info string.
 * @param output_length       Number of bytes of output key material to generate.
 */
[[nodiscard]] std::vector<std::uint8_t> hkdf_sha256(
    std::span<const std::uint8_t> input_key_material,
    std::span<const std::uint8_t> salt,
    std::span<const std::uint8_t> info,
    std::size_t output_length);

} // namespace colony::programs::archive::crypto
