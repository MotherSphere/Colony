#include "programs/archive/crypto/hkdf.hpp"

#include <openssl/evp.h>
#include <openssl/kdf.h>

#include <stdexcept>

namespace colony::programs::archive::crypto
{
namespace
{
class EvpPkeyCtx
{
public:
    explicit EvpPkeyCtx(int type)
        : ctx_{EVP_PKEY_CTX_new_id(type, nullptr)}
    {
        if (!ctx_)
        {
            throw std::runtime_error{"Failed to allocate EVP_PKEY_CTX"};
        }
    }

    ~EvpPkeyCtx()
    {
        EVP_PKEY_CTX_free(ctx_);
    }

    EVP_PKEY_CTX* get() noexcept { return ctx_; }

private:
    EVP_PKEY_CTX* ctx_;
};
}

std::vector<std::uint8_t> hkdf_sha256(
    std::span<const std::uint8_t> input_key_material,
    std::span<const std::uint8_t> salt,
    std::span<const std::uint8_t> info,
    std::size_t output_length)
{
    EvpPkeyCtx ctx{EVP_PKEY_HKDF};
    if (EVP_PKEY_derive_init(ctx.get()) != 1)
    {
        throw std::runtime_error{"EVP_PKEY_derive_init failed"};
    }

    if (EVP_PKEY_CTX_set_hkdf_md(ctx.get(), EVP_sha256()) != 1)
    {
        throw std::runtime_error{"Failed to set HKDF hash function"};
    }

    if (!salt.empty())
    {
        if (EVP_PKEY_CTX_set1_hkdf_salt(ctx.get(), salt.data(), salt.size()) != 1)
        {
            throw std::runtime_error{"Failed to set HKDF salt"};
        }
    }

    if (EVP_PKEY_CTX_set1_hkdf_key(ctx.get(), input_key_material.data(), input_key_material.size()) != 1)
    {
        throw std::runtime_error{"Failed to set HKDF input key material"};
    }

    if (!info.empty())
    {
        if (EVP_PKEY_CTX_add1_hkdf_info(ctx.get(), info.data(), info.size()) != 1)
        {
            throw std::runtime_error{"Failed to set HKDF info"};
        }
    }

    std::vector<std::uint8_t> output(output_length);
    size_t len = output_length;
    if (EVP_PKEY_derive(ctx.get(), output.data(), &len) != 1)
    {
        throw std::runtime_error{"HKDF derivation failed"};
    }

    output.resize(len);
    return output;
}

} // namespace colony::programs::archive::crypto
