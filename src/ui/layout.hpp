#pragma once

#include <algorithm>
#include <cmath>

namespace colony::ui
{

inline float& UiScaleStorage()
{
    static float scale = 0.82f;
    return scale;
}

inline void SetUiScale(float scale)
{
    constexpr float kMinScale = 0.6f;
    constexpr float kMaxScale = 1.1f;
    UiScaleStorage() = std::clamp(scale, kMinScale, kMaxScale);
}

inline float GetUiScale() noexcept
{
    return UiScaleStorage();
}

inline int Scale(int value)
{
    if (value <= 0)
    {
        return value;
    }

    const float scale = GetUiScale();
    const int scaled = static_cast<int>(static_cast<float>(value) * scale + 0.5f);
    return scaled < 1 ? 1 : scaled;
}

inline float Scale(float value)
{
    return value * GetUiScale();
}

inline int ScaleDynamic(int value)
{
    if (value <= 0)
    {
        return value;
    }

    const float scale = GetUiScale();
    const int scaled = static_cast<int>(std::lround(static_cast<double>(value) * scale));
    return scaled < 1 ? 1 : scaled;
}

inline float ScaleDynamic(float value)
{
    return static_cast<float>(value * GetUiScale());
}

} // namespace colony::ui
