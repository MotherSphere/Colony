#include "programs/archive/vault/Repository.hpp"

#include "programs/archive/crypto/aes_gcm.hpp"
#include "programs/archive/crypto/hkdf.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iterator>
#include "json.hpp"
#include <span>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace colony::programs::archive::vault
{
namespace
{
using Buffer = Repository::Buffer;
using Clock = std::chrono::system_clock;

constexpr std::array<char, 8> kMagic{ 'C', 'O', 'L', 'V', 'A', 'U', 'L', 'T' };
constexpr std::uint16_t kFormatVersion = 1;
constexpr std::size_t kNonceSize = 12;
constexpr std::size_t kTagSize = 16;

std::int64_t to_epoch_millis(Clock::time_point time)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()).count();
}

Clock::time_point from_epoch_millis(std::int64_t millis)
{
    return Clock::time_point{std::chrono::milliseconds{millis}};
}

std::string generate_identifier()
{
    std::array<std::uint8_t, 16> random{};
    crypto::fill_random_bytes(random);
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (auto byte : random)
    {
        oss << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

void append_bytes(Buffer& buffer, std::span<const std::uint8_t> bytes)
{
    buffer.insert(buffer.end(), bytes.begin(), bytes.end());
}

void append_le16(Buffer& buffer, std::uint16_t value)
{
    buffer.push_back(static_cast<std::uint8_t>(value & 0xFF));
    buffer.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
}

void append_le32(Buffer& buffer, std::uint32_t value)
{
    buffer.push_back(static_cast<std::uint8_t>(value & 0xFF));
    buffer.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFF));
    buffer.push_back(static_cast<std::uint8_t>((value >> 24) & 0xFF));
}

std::uint16_t read_le16(std::span<const std::uint8_t>& data)
{
    if (data.size() < 2)
    {
        throw std::runtime_error{"Vault payload truncated (u16)"};
    }
    std::uint16_t value = static_cast<std::uint16_t>(data[0]) |
        (static_cast<std::uint16_t>(data[1]) << 8);
    data = data.subspan(2);
    return value;
}

std::uint32_t read_le32(std::span<const std::uint8_t>& data)
{
    if (data.size() < 4)
    {
        throw std::runtime_error{"Vault payload truncated (u32)"};
    }
    std::uint32_t value = static_cast<std::uint32_t>(data[0]) |
        (static_cast<std::uint32_t>(data[1]) << 8) |
        (static_cast<std::uint32_t>(data[2]) << 16) |
        (static_cast<std::uint32_t>(data[3]) << 24);
    data = data.subspan(4);
    return value;
}

std::vector<std::uint8_t> encode_metadata(const RepositoryMetadata& metadata)
{
    nlohmann::json json{
        {"version", metadata.version},
        {"repository_id", metadata.repository_id},
        {"created_at", to_epoch_millis(metadata.created_at)},
        {"updated_at", to_epoch_millis(metadata.updated_at)},
        {"tags", metadata.tags}
    };
    return nlohmann::json::to_cbor(json);
}

RepositoryMetadata decode_metadata(std::span<const std::uint8_t> data)
{
    const auto json = nlohmann::json::from_cbor(data);
    RepositoryMetadata metadata;
    json.at("version").get_to(metadata.version);
    json.at("repository_id").get_to(metadata.repository_id);
    metadata.created_at = from_epoch_millis(json.at("created_at").get<std::int64_t>());
    metadata.updated_at = from_epoch_millis(json.at("updated_at").get<std::int64_t>());
    json.at("tags").get_to(metadata.tags);
    return metadata;
}

std::vector<std::uint8_t> encode_entries(const RepositoryMetadata& metadata, const std::vector<Entry>& entries)
{
    nlohmann::json json{
        {"metadata_version", metadata.version},
        {"entries", entries}
    };
    return nlohmann::json::to_cbor(json);
}

std::vector<Entry> decode_entries(std::span<const std::uint8_t> data, std::uint16_t expected_version)
{
    const auto json = nlohmann::json::from_cbor(data);
    const auto version = json.at("metadata_version").get<std::uint16_t>();
    if (version != expected_version)
    {
        throw std::runtime_error{"Unsupported repository metadata version"};
    }
    return json.at("entries").get<std::vector<Entry>>();
}

std::vector<std::uint8_t> derive_encryption_key(
    std::span<const std::uint8_t> password_key,
    std::span<const std::uint8_t> salt)
{
    static constexpr std::string_view kInfo{"colony.archive.vault"};
    const auto info = std::span<const std::uint8_t>{
        reinterpret_cast<const std::uint8_t*>(kInfo.data()), kInfo.size()};
    return crypto::hkdf_sha256(password_key, salt, info, 32);
}

} // namespace

Repository::Repository()
{
    metadata_.repository_id = generate_identifier();
}

Repository::Repository(RepositoryMetadata metadata, std::vector<Entry> entries)
    : metadata_{std::move(metadata)}
    , entries_{std::move(entries)}
{
    if (metadata_.repository_id.empty())
    {
        metadata_.repository_id = generate_identifier();
    }
    if (metadata_.updated_at < metadata_.created_at)
    {
        metadata_.updated_at = metadata_.created_at;
    }
}

const RepositoryMetadata& Repository::metadata() const noexcept
{
    return metadata_;
}

RepositoryMetadata& Repository::metadata() noexcept
{
    return metadata_;
}

const std::vector<Entry>& Repository::entries() const noexcept
{
    return entries_;
}

std::vector<Entry>& Repository::entries() noexcept
{
    return entries_;
}

void Repository::touch()
{
    metadata_.updated_at = Clock::now();
}

Repository::Buffer Repository::seal(std::string_view password) const
{
    if (password.empty())
    {
        throw std::invalid_argument{"Master password must not be empty"};
    }

    Buffer blob;
    blob.reserve(256);
    blob.insert(blob.end(), kMagic.begin(), kMagic.end());
    append_le16(blob, kFormatVersion);

    const auto metadata_bytes = encode_metadata(metadata_);
    append_le32(blob, static_cast<std::uint32_t>(metadata_bytes.size()));
    append_bytes(blob, metadata_bytes);

    std::array<std::uint8_t, crypto::kArgon2idSaltSize> salt{};
    crypto::fill_random_bytes(salt);
    append_bytes(blob, salt);

    std::array<std::uint8_t, kNonceSize> nonce{};
    crypto::fill_random_bytes(nonce);
    append_bytes(blob, nonce);

    const auto password_key = crypto::derive_argon2id_key(password, salt);
    const auto encryption_key = derive_encryption_key(password_key, salt);
    const auto plaintext = encode_entries(metadata_, entries_);
    const auto cipher = crypto::aes256gcm_encrypt(
        encryption_key,
        nonce,
        plaintext,
        metadata_bytes);

    append_le32(blob, static_cast<std::uint32_t>(cipher.ciphertext.size()));
    append_bytes(blob, cipher.ciphertext);
    append_bytes(blob, cipher.tag);

    return blob;
}

Repository Repository::unseal(const Buffer& blob, std::string_view password)
{
    if (password.empty())
    {
        throw std::invalid_argument{"Master password must not be empty"};
    }

    std::span<const std::uint8_t> cursor{blob};
    if (cursor.size() < kMagic.size())
    {
        throw std::runtime_error{"Vault payload too small"};
    }

    if (!std::equal(kMagic.begin(), kMagic.end(), cursor.begin()))
    {
        throw std::runtime_error{"Vault payload missing magic header"};
    }
    cursor = cursor.subspan(kMagic.size());

    const auto version = read_le16(cursor);
    if (version != kFormatVersion)
    {
        throw std::runtime_error{"Unsupported vault file version"};
    }

    const auto metadata_length = read_le32(cursor);
    if (cursor.size() < metadata_length)
    {
        throw std::runtime_error{"Vault payload truncated (metadata)"};
    }

    const std::span<const std::uint8_t> metadata_bytes{cursor.data(), metadata_length};
    cursor = cursor.subspan(metadata_length);
    auto metadata = decode_metadata(metadata_bytes);

    if (cursor.size() < crypto::kArgon2idSaltSize + kNonceSize + sizeof(std::uint32_t) + kTagSize)
    {
        throw std::runtime_error{"Vault payload truncated"};
    }

    const std::span<const std::uint8_t> salt{cursor.data(), crypto::kArgon2idSaltSize};
    cursor = cursor.subspan(crypto::kArgon2idSaltSize);

    const std::span<const std::uint8_t> nonce{cursor.data(), kNonceSize};
    cursor = cursor.subspan(kNonceSize);

    const auto ciphertext_length = read_le32(cursor);
    if (cursor.size() < ciphertext_length + kTagSize)
    {
        throw std::runtime_error{"Vault payload truncated (ciphertext)"};
    }

    const std::span<const std::uint8_t> ciphertext{cursor.data(), ciphertext_length};
    cursor = cursor.subspan(ciphertext_length);

    const std::span<const std::uint8_t> tag{cursor.data(), kTagSize};
    cursor = cursor.subspan(kTagSize);

    const auto password_key = crypto::derive_argon2id_key(password, salt);
    const auto encryption_key = derive_encryption_key(password_key, salt);
    const auto plaintext = crypto::aes256gcm_decrypt(
        encryption_key,
        nonce,
        ciphertext,
        tag,
        metadata_bytes);

    auto entries = decode_entries(plaintext, metadata.version);

    return Repository{std::move(metadata), std::move(entries)};
}

Repository::Buffer Repository::reencrypt(
    const Buffer& blob,
    std::string_view current_password,
    std::string_view new_password)
{
    auto repository = Repository::unseal(blob, current_password);
    repository.touch();
    repository.metadata().updated_at = Clock::now();
    return repository.seal(new_password);
}

void Repository::save(const std::filesystem::path& path, std::string_view password) const
{
    auto blob = seal(password);
    std::ofstream output{path, std::ios::binary};
    if (!output.is_open())
    {
        throw std::runtime_error{"Failed to open vault for writing"};
    }
    output.write(reinterpret_cast<const char*>(blob.data()), static_cast<std::streamsize>(blob.size()));
    if (!output)
    {
        throw std::runtime_error{"Failed to write vault"};
    }
}

Repository Repository::load(const std::filesystem::path& path, std::string_view password)
{
    std::ifstream input{path, std::ios::binary};
    if (!input.is_open())
    {
        throw std::runtime_error{"Failed to open vault for reading"};
    }
    Buffer blob{std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
    if (blob.empty())
    {
        throw std::runtime_error{"Vault file is empty"};
    }
    return Repository::unseal(blob, password);
}

} // namespace colony::programs::archive::vault
