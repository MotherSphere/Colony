#pragma once

#include "programs/archive/crypto/argon2id.hpp"
#include "programs/archive/vault/Entry.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace colony::programs::archive::vault
{

struct RepositoryMetadata
{
    std::uint16_t version{1};
    std::string repository_id;
    std::chrono::system_clock::time_point created_at{std::chrono::system_clock::now()};
    std::chrono::system_clock::time_point updated_at{created_at};
    std::vector<std::string> tags;
};

/**
 * @brief Provides loading and storing encrypted vault files (.vault).
 *
 * The file layout is:
 *  - Magic header (8 bytes) and version (uint16_t, little endian)
 *  - Metadata length (uint32_t) followed by metadata CBOR payload
 *  - Salt (crypto_pwhash_SALTBYTES bytes)
 *  - Nonce (12 bytes)
 *  - Ciphertext length (uint32_t), ciphertext bytes, and 16-byte tag
 */
class Repository
{
public:
    using Buffer = std::vector<std::uint8_t>;

    Repository();
    explicit Repository(RepositoryMetadata metadata, std::vector<Entry> entries = {});

    [[nodiscard]] const RepositoryMetadata& metadata() const noexcept;
    [[nodiscard]] RepositoryMetadata& metadata() noexcept;

    [[nodiscard]] const std::vector<Entry>& entries() const noexcept;
    [[nodiscard]] std::vector<Entry>& entries() noexcept;

    void touch();

    [[nodiscard]] Buffer seal(std::string_view password) const;
    [[nodiscard]] static Repository unseal(const Buffer& blob, std::string_view password);

    [[nodiscard]] static Buffer reencrypt(
        const Buffer& blob,
        std::string_view current_password,
        std::string_view new_password);

    void save(const std::filesystem::path& path, std::string_view password) const;
    static Repository load(const std::filesystem::path& path, std::string_view password);

private:
    RepositoryMetadata metadata_{};
    std::vector<Entry> entries_{};
};

} // namespace colony::programs::archive::vault
