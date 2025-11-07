#pragma once

#include <cmath>

namespace colony::ui
{

constexpr float kUiScale = 0.82f;

constexpr int Scale(int value)
{
    if (value <= 0)
    {
        return value;
    }
    const int scaled = static_cast<int>(value * kUiScale + 0.5f);
    return scaled < 1 ? 1 : scaled;
}

constexpr float Scale(float value)
{
    return value * kUiScale;
}

inline int ScaleDynamic(int value)
{
    if (value <= 0)
    {
        return value;
    }
    const int scaled = static_cast<int>(std::lround(static_cast<double>(value) * kUiScale));
    return scaled < 1 ? 1 : scaled;
}

inline float ScaleDynamic(float value)
{
    return static_cast<float>(value * kUiScale);
}

} // namespace colony::ui
