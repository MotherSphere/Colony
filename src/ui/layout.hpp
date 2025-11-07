#pragma once

#include <algorithm>
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

inline float ResponsiveScaleForWidth(int width)
{
    const float normalized = static_cast<float>(width) / 1600.0f;
    return std::clamp(normalized, 0.75f, 1.15f);
}

struct LayoutMetrics
{
    int navRailWidth = 0;
    int libraryWidth = 0;
    int heroWidth = 0;
    int statusBarHeight = 0;
    float motionScale = 1.0f;
};

inline LayoutMetrics ComputeLayoutMetrics(int windowWidth, int windowHeight)
{
    LayoutMetrics metrics;
    const float responsiveScale = ResponsiveScaleForWidth(windowWidth);
    metrics.navRailWidth = static_cast<int>(std::clamp(windowWidth / 14, Scale(72), Scale(120)) * responsiveScale);
    metrics.libraryWidth = static_cast<int>(std::clamp(windowWidth / 3, Scale(240), Scale(360)) * responsiveScale);
    metrics.heroWidth = std::max(0, windowWidth - metrics.navRailWidth - metrics.libraryWidth);
    metrics.statusBarHeight = std::max(Scale(48), static_cast<int>(Scale(52) * responsiveScale));
    metrics.motionScale = std::clamp(responsiveScale, 0.8f, 1.1f);
    return metrics;
}

} // namespace colony::ui
