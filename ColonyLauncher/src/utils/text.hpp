#pragma once

#include "utils/sdl_wrappers.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <string>
#include <string_view>

namespace colony
{

struct TextTexture
{
    sdl::TextureHandle texture;
    int width{};
    int height{};
};

inline TextTexture CreateTextTexture(SDL_Renderer* renderer, TTF_Font* font, std::string_view text, SDL_Color color)
{
    const std::string textString{text};
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, textString.c_str(), color);
    if (surface == nullptr)
    {
        return {};
    }

    sdl::TextureHandle texture{SDL_CreateTextureFromSurface(renderer, surface)};
    if (!texture)
    {
        SDL_FreeSurface(surface);
        return {};
    }

    TextTexture result{std::move(texture), surface->w, surface->h};
    SDL_FreeSurface(surface);
    return result;
}

inline void RenderTexture(SDL_Renderer* renderer, const TextTexture& textTexture, const SDL_Rect& rect)
{
    if (textTexture.texture)
    {
        SDL_RenderCopy(renderer, textTexture.texture.get(), nullptr, &rect);
    }
}

} // namespace colony
