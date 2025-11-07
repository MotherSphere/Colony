#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <utility>

namespace colony::sdl
{

template <typename Pointer, void (*Deleter)(Pointer*)>
class Handle
{
  public:
    Handle() = default;
    explicit Handle(Pointer* pointer) noexcept : pointer_(pointer) {}

    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;

    Handle(Handle&& other) noexcept : pointer_(std::exchange(other.pointer_, nullptr)) {}
    Handle& operator=(Handle&& other) noexcept
    {
        if (this != &other)
        {
            reset();
            pointer_ = std::exchange(other.pointer_, nullptr);
        }
        return *this;
    }

    ~Handle() { reset(); }

    [[nodiscard]] Pointer* get() const noexcept { return pointer_; }
    [[nodiscard]] Pointer* operator->() const noexcept { return pointer_; }
    [[nodiscard]] Pointer& operator*() const noexcept { return *pointer_; }
    explicit operator bool() const noexcept { return pointer_ != nullptr; }

    Pointer* release() noexcept { return std::exchange(pointer_, nullptr); }

    void reset(Pointer* pointer = nullptr) noexcept
    {
        if (pointer_ != nullptr)
        {
            Deleter(pointer_);
        }
        pointer_ = pointer;
    }

  private:
    Pointer* pointer_{};
};

using WindowHandle = Handle<SDL_Window, SDL_DestroyWindow>;
using RendererHandle = Handle<SDL_Renderer, SDL_DestroyRenderer>;
using TextureHandle = Handle<SDL_Texture, SDL_DestroyTexture>;
using FontHandle = Handle<TTF_Font, TTF_CloseFont>;

} // namespace colony::sdl
