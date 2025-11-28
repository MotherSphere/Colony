#include "utils/text_wrapping.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace colony
{
namespace
{
std::size_t Utf8CharLength(unsigned char lead)
{
    if (lead < 0x80)
    {
        return 1;
    }
    if ((lead >> 5) == 0x6)
    {
        return 2;
    }
    if ((lead >> 4) == 0xE)
    {
        return 3;
    }
    if ((lead >> 3) == 0x1E)
    {
        return 4;
    }
    return 1;
}

int MeasureWidth(TTF_Font* font, const std::string& text)
{
    if (text.empty())
    {
        return 0;
    }

    int width = 0;
    int height = 0;
    if (TTF_SizeUTF8(font, text.c_str(), &width, &height) != 0)
    {
        return -1;
    }
    return width;
}

std::size_t FindFittingPrefix(TTF_Font* font, std::string_view text, int maxWidth)
{
    std::size_t best = 0;
    std::size_t index = 0;
    while (index < text.size())
    {
        std::size_t charLength = Utf8CharLength(static_cast<unsigned char>(text[index]));
        charLength = std::min(charLength, text.size() - index);
        std::string candidate{text.substr(0, index + charLength)};
        const int width = MeasureWidth(font, candidate);
        if (width < 0)
        {
            return text.size();
        }
        if (width > maxWidth)
        {
            break;
        }
        best = index + charLength;
        index += charLength;
    }
    return best;
}

std::vector<std::string> BreakLongWord(TTF_Font* font, const std::string& word, int maxWidth)
{
    std::string remaining = word;
    std::vector<std::string> chunks;
    while (!remaining.empty())
    {
        const std::size_t prefix = FindFittingPrefix(font, remaining, maxWidth);
        std::size_t length = prefix;
        if (length == 0)
        {
            length = Utf8CharLength(static_cast<unsigned char>(remaining.front()));
        }
        std::string chunk = remaining.substr(0, length);
        chunks.emplace_back(std::move(chunk));
        remaining.erase(0, length);
    }
    return chunks;
}

} // namespace

std::vector<std::string> WrapTextToWidth(TTF_Font* font, std::string_view text, int maxWidth)
{
    std::vector<std::string> lines;
    if (font == nullptr)
    {
        if (!text.empty())
        {
            lines.emplace_back(text);
        }
        return lines;
    }

    if (maxWidth <= 0)
    {
        if (!text.empty())
        {
            lines.emplace_back(text);
        }
        return lines;
    }

    const std::string source{text};
    std::string currentLine;

    auto pushLine = [&](bool forceEmpty = false) {
        if (!currentLine.empty() || forceEmpty)
        {
            lines.emplace_back(std::move(currentLine));
            currentLine.clear();
        }
    };

    std::size_t position = 0;
    while (position < source.size())
    {
        std::size_t nextDelimiter = source.find_first_of(" \t\n\r", position);
        std::string word = source.substr(position, nextDelimiter - position);
        char delimiter = nextDelimiter == std::string::npos ? '\0' : source[nextDelimiter];

        if (!word.empty())
        {
            std::string candidate;
            if (currentLine.empty())
            {
                candidate = word;
            }
            else
            {
                candidate.reserve(currentLine.size() + 1 + word.size());
                candidate.append(currentLine);
                candidate.push_back(' ');
                candidate.append(word);
            }

            const int candidateWidth = MeasureWidth(font, candidate.empty() ? word : candidate);
            if (!candidate.empty() && candidateWidth >= 0 && candidateWidth <= maxWidth)
            {
                currentLine = std::move(candidate);
            }
            else if (currentLine.empty())
            {
                const int wordWidth = MeasureWidth(font, word);
                if (wordWidth > maxWidth && wordWidth >= 0)
                {
                    const auto chunks = BreakLongWord(font, word, maxWidth);
                    if (!chunks.empty())
                    {
                        std::string trailingSegment = std::move(chunks.back());
                        for (std::size_t i = 0; i + 1 < chunks.size(); ++i)
                        {
                            lines.emplace_back(std::move(chunks[i]));
                        }
                        currentLine = std::move(trailingSegment);
                    }
                }
                else
                {
                    currentLine = std::move(word);
                }
            }
            else
            {
                pushLine();
                const int wordWidth = MeasureWidth(font, word);
                if (wordWidth > maxWidth && wordWidth >= 0)
                {
                    const auto chunks = BreakLongWord(font, word, maxWidth);
                    if (!chunks.empty())
                    {
                        std::string trailingSegment = std::move(chunks.back());
                        for (std::size_t i = 0; i + 1 < chunks.size(); ++i)
                        {
                            lines.emplace_back(std::move(chunks[i]));
                        }
                        currentLine = std::move(trailingSegment);
                    }
                }
                else
                {
                    currentLine = std::move(word);
                }
            }
        }

        if (delimiter == '\n' || delimiter == '\r')
        {
            pushLine(true);
        }

        if (nextDelimiter == std::string::npos)
        {
            break;
        }
        position = nextDelimiter + 1;
    }

    if (!currentLine.empty())
    {
        lines.emplace_back(std::move(currentLine));
    }

    return lines;
}

} // namespace colony

