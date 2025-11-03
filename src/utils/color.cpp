#include "utils/color.hpp"

#include <algorithm>
#include <cctype>
#include <string>

namespace colony::color
{
namespace
{
int HexDigitToInt(char c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f')
    {
        return 10 + (c - 'a');
    }
    if (c >= 'A' && c <= 'F')
    {
        return 10 + (c - 'A');
    }
    return -1;
}

bool ExpandShortHex(std::string& value)
{
    if (value.size() == 3 || value.size() == 4)
    {
        std::string expanded;
        expanded.reserve(value.size() * 2);
        for (char c : value)
        {
            if (HexDigitToInt(c) < 0)
            {
                return false;
            }
            expanded.push_back(c);
            expanded.push_back(c);
        }
        value = std::move(expanded);
        return true;
    }
    return false;
}

bool ParseChannelPair(const std::string& value, std::size_t index, Uint8& out)
{
    const int high = HexDigitToInt(value[index]);
    const int low = HexDigitToInt(value[index + 1]);
    if (high < 0 || low < 0)
    {
        return false;
    }
    out = static_cast<Uint8>((high << 4) | low);
    return true;
}
} // namespace

SDL_Color ParseHexColor(std::string_view hex, SDL_Color fallback)
{
    std::string cleaned;
    cleaned.reserve(hex.size());
    for (char c : hex)
    {
        if (c == '#')
        {
            continue;
        }
        if (!std::isspace(static_cast<unsigned char>(c)))
        {
            cleaned.push_back(c);
        }
    }

    if (cleaned.size() == 3 || cleaned.size() == 4)
    {
        if (!ExpandShortHex(cleaned))
        {
            return fallback;
        }
    }

    if (cleaned.size() != 6 && cleaned.size() != 8)
    {
        return fallback;
    }

    Uint8 r = 0;
    Uint8 g = 0;
    Uint8 b = 0;
    Uint8 a = SDL_ALPHA_OPAQUE;
    if (!ParseChannelPair(cleaned, 0, r) || !ParseChannelPair(cleaned, 2, g) || !ParseChannelPair(cleaned, 4, b))
    {
        return fallback;
    }
    if (cleaned.size() == 8)
    {
        if (!ParseChannelPair(cleaned, 6, a))
        {
            return fallback;
        }
    }

    return SDL_Color{r, g, b, a};
}

SDL_Color Mix(const SDL_Color& a, const SDL_Color& b, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    auto blend = [&](Uint8 componentA, Uint8 componentB) {
        return static_cast<Uint8>(componentA + static_cast<int>((componentB - componentA) * t));
    };
    return SDL_Color{blend(a.r, b.r), blend(a.g, b.g), blend(a.b, b.b), blend(a.a, b.a)};
}

void RenderVerticalGradient(SDL_Renderer* renderer, const SDL_Rect& area, SDL_Color top, SDL_Color bottom)
{
    if (renderer == nullptr || area.h <= 0)
    {
        return;
    }

    for (int offset = 0; offset < area.h; ++offset)
    {
        const float t = area.h > 1 ? static_cast<float>(offset) / static_cast<float>(area.h - 1) : 0.0f;
        const SDL_Color color = Mix(top, bottom, t);
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderDrawLine(renderer, area.x, area.y + offset, area.x + area.w, area.y + offset);
    }
}

} // namespace colony::color
