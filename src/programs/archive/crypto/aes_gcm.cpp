#include "programs/archive/crypto/aes_gcm.hpp"

#include <openssl/evp.h>

#include <stdexcept>

namespace colony::programs::archive::crypto
{
namespace
{
constexpr std::size_t kAes256KeySize = 32;

class EvpCipherCtx
{
public:
    EvpCipherCtx()
        : ctx_{EVP_CIPHER_CTX_new()}
    {
        if (!ctx_)
        {
            throw std::runtime_error{"Failed to allocate EVP_CIPHER_CTX"};
        }
    }

    ~EvpCipherCtx()
    {
        EVP_CIPHER_CTX_free(ctx_);
    }

    EVP_CIPHER_CTX* get() noexcept { return ctx_; }

private:
    EVP_CIPHER_CTX* ctx_;
};

void ensure_key_size(std::span<const std::uint8_t> key)
{
    if (key.size() != kAes256KeySize)
    {
        throw std::invalid_argument{"AES-256-GCM requires a 32-byte key"};
    }
}
}

GcmCiphertext aes256gcm_encrypt(
    std::span<const std::uint8_t> key,
    std::span<const std::uint8_t> nonce,
    std::span<const std::uint8_t> plaintext,
    std::span<const std::uint8_t> associated)
{
    ensure_key_size(key);

    EvpCipherCtx ctx;
    if (EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1)
    {
        throw std::runtime_error{"EVP_EncryptInit_ex failed"};
    }

    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, nonce.size(), nullptr) != 1)
    {
        throw std::runtime_error{"Failed to set IV length"};
    }

    if (EVP_EncryptInit_ex(ctx.get(), nullptr, nullptr, key.data(), nonce.data()) != 1)
    {
        throw std::runtime_error{"Failed to initialise AES-256-GCM key"};
    }

    int len = 0;
    if (!associated.empty())
    {
        if (EVP_EncryptUpdate(ctx.get(), nullptr, &len, associated.data(), associated.size()) != 1)
        {
            throw std::runtime_error{"Failed to process associated data"};
        }
    }

    GcmCiphertext result;
    result.ciphertext.resize(plaintext.size());
    if (EVP_EncryptUpdate(ctx.get(), result.ciphertext.data(), &len, plaintext.data(), plaintext.size()) != 1)
    {
        throw std::runtime_error{"Failed to encrypt plaintext"};
    }
    int ciphertext_len = len;

    if (EVP_EncryptFinal_ex(ctx.get(), result.ciphertext.data() + ciphertext_len, &len) != 1)
    {
        throw std::runtime_error{"Failed to finalise AES-256-GCM encryption"};
    }
    ciphertext_len += len;
    result.ciphertext.resize(ciphertext_len);

    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, result.tag.size(), result.tag.data()) != 1)
    {
        throw std::runtime_error{"Failed to obtain GCM authentication tag"};
    }

    return result;
}

std::vector<std::uint8_t> aes256gcm_decrypt(
    std::span<const std::uint8_t> key,
    std::span<const std::uint8_t> nonce,
    std::span<const std::uint8_t> ciphertext,
    std::span<const std::uint8_t> tag,
    std::span<const std::uint8_t> associated)
{
    ensure_key_size(key);

    EvpCipherCtx ctx;
    if (EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1)
    {
        throw std::runtime_error{"EVP_DecryptInit_ex failed"};
    }

    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, nonce.size(), nullptr) != 1)
    {
        throw std::runtime_error{"Failed to set IV length"};
    }

    if (EVP_DecryptInit_ex(ctx.get(), nullptr, nullptr, key.data(), nonce.data()) != 1)
    {
        throw std::runtime_error{"Failed to initialise AES-256-GCM key"};
    }

    int len = 0;
    if (!associated.empty())
    {
        if (EVP_DecryptUpdate(ctx.get(), nullptr, &len, associated.data(), associated.size()) != 1)
        {
            throw std::runtime_error{"Failed to process associated data"};
        }
    }

    std::vector<std::uint8_t> plaintext(ciphertext.size());
    if (EVP_DecryptUpdate(ctx.get(), plaintext.data(), &len, ciphertext.data(), ciphertext.size()) != 1)
    {
        throw std::runtime_error{"Failed to decrypt ciphertext"};
    }
    int plaintext_len = len;

    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, tag.size(), const_cast<std::uint8_t*>(tag.data())) != 1)
    {
        throw std::runtime_error{"Failed to set authentication tag"};
    }

    const int final_status = EVP_DecryptFinal_ex(ctx.get(), plaintext.data() + plaintext_len, &len);
    if (final_status != 1)
    {
        throw std::runtime_error{"AES-256-GCM authentication failed"};
    }

    plaintext_len += len;
    plaintext.resize(plaintext_len);
    return plaintext;
}

} // namespace colony::programs::archive::crypto
