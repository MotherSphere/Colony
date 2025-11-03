#include "ui/program_visuals.hpp"

#include "utils/color.hpp"
#include "utils/text_wrapping.hpp"

#include <algorithm>
#include <string>

namespace colony::ui
{

ProgramVisuals BuildProgramVisuals(
    const colony::ViewContent& content,
    SDL_Renderer* renderer,
    TTF_Font* heroTitleFont,
    TTF_Font* heroSubtitleFont,
    TTF_Font* heroBodyFont,
    TTF_Font* buttonFont,
    TTF_Font* tileTitleFont,
    TTF_Font* tileSubtitleFont,
    TTF_Font* tileMetaFont,
    TTF_Font* patchTitleFont,
    TTF_Font* patchBodyFont,
    TTF_Font* statusFont,
    SDL_Color heroTitleColor,
    SDL_Color heroBodyColor,
    SDL_Color heroSubtitleColor,
    SDL_Color mutedColor,
    SDL_Color statusBarTextColor,
    SDL_Color gradientFallbackStart,
    SDL_Color gradientFallbackEnd)
{
    ProgramVisuals visuals;
    visuals.content = &content;
    visuals.heroTitle = colony::CreateTextTexture(renderer, heroTitleFont, content.heading, heroTitleColor);
    if (!content.tagline.empty())
    {
        visuals.heroTagline = colony::CreateTextTexture(renderer, heroSubtitleFont, content.tagline, heroSubtitleColor);
    }
    if (!content.availability.empty())
    {
        visuals.availability = colony::CreateTextTexture(renderer, heroBodyFont, content.availability, heroBodyColor);
    }
    if (!content.version.empty())
    {
        visuals.version = colony::CreateTextTexture(renderer, tileMetaFont, content.version, mutedColor);
    }
    if (!content.installState.empty())
    {
        visuals.installState = colony::CreateTextTexture(renderer, tileMetaFont, content.installState, mutedColor);
    }
    if (!content.lastLaunched.empty())
    {
        visuals.lastLaunched = colony::CreateTextTexture(renderer, tileMetaFont, content.lastLaunched, mutedColor);
    }
    visuals.actionLabel = colony::CreateTextTexture(renderer, buttonFont, content.primaryActionLabel, heroTitleColor);
    visuals.tileTitle = colony::CreateTextTexture(renderer, tileTitleFont, content.heading, heroTitleColor);

    std::string subtitle = !content.tagline.empty() ? content.tagline
        : (content.paragraphs.empty() ? std::string{} : content.paragraphs.front());
    if (!subtitle.empty())
    {
        visuals.tileSubtitle = colony::CreateTextTexture(renderer, tileSubtitleFont, subtitle, mutedColor);
    }

    std::string meta;
    if (!content.version.empty())
    {
        meta.append(content.version);
    }
    if (!content.installState.empty())
    {
        if (!meta.empty())
        {
            meta.append(" • ");
        }
        meta.append(content.installState);
    }
    if (!meta.empty())
    {
        visuals.tileMeta = colony::CreateTextTexture(renderer, tileMetaFont, meta, mutedColor);
    }

    if (!content.statusMessage.empty())
    {
        visuals.statusBar = colony::CreateTextTexture(renderer, statusFont, content.statusMessage, statusBarTextColor);
    }

    visuals.accent = colony::color::ParseHexColor(content.accentColor, SDL_Color{91, 150, 255, SDL_ALPHA_OPAQUE});
    visuals.gradientStart = colony::color::ParseHexColor(content.heroGradient[0], gradientFallbackStart);
    visuals.gradientEnd = colony::color::ParseHexColor(content.heroGradient[1], gradientFallbackEnd);

    visuals.sections.reserve(content.sections.size());
    for (const auto& section : content.sections)
    {
        ProgramVisuals::PatchSection patchSection;
        patchSection.title = colony::CreateTextTexture(renderer, patchTitleFont, section.title, heroTitleColor);
        visuals.sections.emplace_back(std::move(patchSection));
    }

    return visuals;
}

void RebuildDescription(
    ProgramVisuals& visuals,
    SDL_Renderer* renderer,
    TTF_Font* font,
    int maxWidth,
    SDL_Color bodyColor)
{
    if (renderer == nullptr || font == nullptr || maxWidth <= 0)
    {
        return;
    }
    if (visuals.descriptionWidth == maxWidth)
    {
        return;
    }

    visuals.descriptionWidth = maxWidth;
    visuals.descriptionLines.clear();

    const int lineSkip = TTF_FontLineSkip(font);
    for (const auto& paragraph : visuals.content->paragraphs)
    {
        const auto wrappedLines = colony::WrapTextToWidth(font, paragraph, maxWidth);
        std::vector<colony::TextTexture> lineTextures;
        lineTextures.reserve(wrappedLines.size());
        for (const auto& line : wrappedLines)
        {
            if (line.empty())
            {
                colony::TextTexture placeholder{};
                placeholder.width = 0;
                placeholder.height = std::max(lineSkip, 0);
                lineTextures.emplace_back(std::move(placeholder));
                continue;
            }
            lineTextures.emplace_back(colony::CreateTextTexture(renderer, font, line, bodyColor));
        }
        if (!lineTextures.empty())
        {
            visuals.descriptionLines.emplace_back(std::move(lineTextures));
        }
    }
}

void RebuildHighlights(
    ProgramVisuals& visuals,
    SDL_Renderer* renderer,
    TTF_Font* font,
    int maxWidth,
    SDL_Color textColor)
{
    if (renderer == nullptr || font == nullptr || maxWidth <= 0)
    {
        return;
    }
    if (visuals.highlightsWidth == maxWidth)
    {
        return;
    }

    visuals.highlightsWidth = maxWidth;
    visuals.highlightLines.clear();

    const int lineSkip = TTF_FontLineSkip(font);
    const int bulletIndent = 24;
    const int availableWidth = std::max(0, maxWidth - bulletIndent);

    for (const auto& highlight : visuals.content->heroHighlights)
    {
        const auto wrappedLines = colony::WrapTextToWidth(font, highlight, availableWidth);
        if (wrappedLines.empty())
        {
            continue;
        }

        std::vector<WrappedLine> lines;
        lines.reserve(wrappedLines.size());
        for (std::size_t i = 0; i < wrappedLines.size(); ++i)
        {
            std::string text = wrappedLines[i];
            bool indent = i != 0;
            if (!indent)
            {
                text.insert(0, "\xE2\x80\xA2 ");
            }
            else
            {
                text.insert(0, "  ");
            }

            WrappedLine line;
            if (text.empty())
            {
                line.texture.width = 0;
                line.texture.height = std::max(lineSkip, 0);
            }
            else
            {
                line.texture = colony::CreateTextTexture(renderer, font, text, textColor);
            }
            line.indent = indent;
            lines.emplace_back(std::move(line));
        }
        if (!lines.empty())
        {
            visuals.highlightLines.emplace_back(std::move(lines));
        }
    }
}

void RebuildSections(
    ProgramVisuals& visuals,
    SDL_Renderer* renderer,
    TTF_Font* titleFont,
    TTF_Font* bodyFont,
    int maxWidth,
    SDL_Color titleColor,
    SDL_Color bodyColor)
{
    if (renderer == nullptr || bodyFont == nullptr || maxWidth <= 0)
    {
        return;
    }

    for (std::size_t i = 0; i < visuals.sections.size(); ++i)
    {
        auto& sectionVisual = visuals.sections[i];
        if (sectionVisual.width == maxWidth)
        {
            continue;
        }

        sectionVisual.width = maxWidth;
        sectionVisual.lines.clear();

        if (titleFont != nullptr)
        {
            const std::string& titleText = visuals.content->sections[i].title;
            sectionVisual.title = colony::CreateTextTexture(renderer, titleFont, titleText, titleColor);
        }

        const int bulletIndent = 20;
        const int availableWidth = std::max(0, maxWidth - bulletIndent);
        const int lineSkip = TTF_FontLineSkip(bodyFont);

        for (const auto& option : visuals.content->sections[i].options)
        {
            const auto wrappedLines = colony::WrapTextToWidth(bodyFont, option, availableWidth);
            if (wrappedLines.empty())
            {
                continue;
            }
            std::vector<WrappedLine> optionLines;
            optionLines.reserve(wrappedLines.size());
            for (std::size_t lineIndex = 0; lineIndex < wrappedLines.size(); ++lineIndex)
            {
                bool indent = lineIndex != 0;
                std::string lineText = wrappedLines[lineIndex];
                if (!indent)
                {
                    lineText.insert(0, "\xE2\x80\xA2 ");
                }
                else
                {
                    lineText.insert(0, "  ");
                }

                WrappedLine line;
                if (lineText.empty())
                {
                    line.texture.width = 0;
                    line.texture.height = std::max(lineSkip, 0);
                }
                else
                {
                    line.texture = colony::CreateTextTexture(renderer, bodyFont, lineText, bodyColor);
                }
                line.indent = indent;
                optionLines.emplace_back(std::move(line));
            }
            if (!optionLines.empty())
            {
                sectionVisual.lines.emplace_back(std::move(optionLines));
            }
        }
    }
}

} // namespace colony::ui
