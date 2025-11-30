#include "app/application.h"

#include "core/content_loader.hpp"
#include "frontend/utils/font_loader.hpp"
#include "frontend/views/dashboard_page.hpp"
#include "nexus/nexus_main.hpp"
#include "ui/layout.hpp"
#include "ui/theme.hpp"
#include "utils/color.hpp"
#include "utils/drawing.hpp"
#include "utils/asset_paths.hpp"
#include "utils/font_manager.hpp"
#include "utils/text.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cctype>
#include <filesystem>
#include <ctime>
#include <cstring>
#include <iomanip>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <system_error>
#include <thread>
#include <vector>
#include <cstdlib>
#if !defined(_WIN32)
#include <unistd.h>
#endif

namespace colony
{

void Application::BuildHubPanel()
{
    SDL_Renderer* renderer = rendererHost_.Renderer();
    if (!renderer)
    {
        return;
    }

    const auto& hubConfig = content_.hub;

    hubSearchTokens_ = TokenizeHubSearch(hubSearchQuery_);

    ui::panels::HubContent hubContent;
    hubContent.searchPlaceholder = GetLocalizedString("hub.search.placeholder", "Rechercher une destination");

    if (!hubConfig.headlineLocalizationKey.empty())
    {
        hubContent.headline = GetLocalizedString(hubConfig.headlineLocalizationKey, hubConfig.headlineLocalizationKey);
    }
    if (hubContent.headline.empty())
    {
        hubContent.headline = content_.brandName.empty() ? std::string{"COLONY"} : content_.brandName;
    }

    if (!hubConfig.descriptionLocalizationKey.empty())
    {
        hubContent.description = GetLocalizedString(
            hubConfig.descriptionLocalizationKey,
            hubConfig.descriptionLocalizationKey);
    }
    if (hubContent.description.empty())
    {
        hubContent.description = GetLocalizedString("hub.status", "Select a destination to continue.");
    }

    for (const auto& highlightKey : hubConfig.highlightLocalizationKeys)
    {
        if (!highlightKey.empty())
        {
            hubContent.highlights.push_back(GetLocalizedString(highlightKey, highlightKey));
        }
    }

    hubRenderedBranchIds_.clear();
    hubContent.branches.reserve(hubConfig.branches.size());

    for (std::size_t index = 0; index < hubConfig.branches.size(); ++index)
    {
        const auto& branch = hubConfig.branches[index];

        std::string title = branch.titleLocalizationKey.empty()
            ? branch.id
            : GetLocalizedString(branch.titleLocalizationKey, branch.id);
        std::string description = branch.descriptionLocalizationKey.empty()
            ? branch.id
            : GetLocalizedString(branch.descriptionLocalizationKey, branch.descriptionLocalizationKey);

        std::vector<std::string> tags;
        tags.reserve(branch.tagLocalizationKeys.size());
        for (const auto& tagKey : branch.tagLocalizationKeys)
        {
            if (!tagKey.empty())
            {
                tags.push_back(GetLocalizedString(tagKey, tagKey));
            }
        }

        std::string metricsText = branch.metricsLocalizationKey.empty()
            ? std::string{}
            : GetLocalizedString(branch.metricsLocalizationKey, branch.metricsLocalizationKey);

        std::string haystack = title;
        haystack.push_back(' ');
        haystack += description;
        for (const auto& tag : tags)
        {
            haystack.push_back(' ');
            haystack += tag;
        }
        if (!metricsText.empty())
        {
            haystack.push_back(' ');
            haystack += metricsText;
        }
        const std::string normalizedHaystack = NormalizeHubSearchString(haystack);
        bool matchesQuery = true;
        for (const auto& token : hubSearchTokens_)
        {
            if (normalizedHaystack.find(token) == std::string::npos)
            {
                matchesQuery = false;
                break;
            }
        }
        if (!matchesQuery)
        {
            continue;
        }

        ui::panels::HubBranchContent branchContent;
        branchContent.id = branch.id;
        branchContent.title = title;
        branchContent.description = description;
        branchContent.accent = branch.accentColor.empty() ? theme_.channelBadge
                                                         : color::ParseHexColor(branch.accentColor, theme_.channelBadge);
        branchContent.tags = tags;
        branchContent.actionLabel = branch.actionLocalizationKey.empty()
            ? GetLocalizedString("hub.branch.default_action", "Open")
            : GetLocalizedString(branch.actionLocalizationKey, branchContent.title);
        branchContent.metrics = metricsText;

        if (!branch.channelId.empty())
        {
            auto channelIt = std::find_if(
                content_.channels.begin(),
                content_.channels.end(),
                [&](const Channel& channel) { return channel.id == branch.channelId; });
            if (channelIt != content_.channels.end())
            {
                branchContent.channelLabel = std::string{"Canal : "} + channelIt->label;
            }
        }

        if (!branch.programId.empty())
        {
            branchContent.programLabel = std::string{"Programme : "} + branch.programId;
        }

        branchContent.detailBullets.reserve(branchContent.tags.size());
        for (const auto& tag : branchContent.tags)
        {
            branchContent.detailBullets.push_back(std::string{"#"} + tag);
        }

        hubContent.branches.emplace_back(std::move(branchContent));
        hubRenderedBranchIds_.push_back(branch.id);
    }

    if (hubContent.highlights.empty())
    {
        const int count = static_cast<int>(hubContent.branches.size());
        if (!hubSearchTokens_.empty())
        {
            hubContent.highlights.push_back(std::to_string(count) + (count == 1 ? " résultat" : " résultats"));
        }
        else
        {
            const int total = static_cast<int>(hubConfig.branches.size());
            hubContent.highlights.push_back(std::to_string(total) + (total == 1 ? " destination" : " destinations"));
        }
    }

    if (!hubConfig.primaryActionLocalizationKey.empty())
    {
        hubContent.primaryActionLabel =
            GetLocalizedString(hubConfig.primaryActionLocalizationKey, hubConfig.primaryActionLocalizationKey);
    }
    if (!hubConfig.primaryActionDescriptionLocalizationKey.empty())
    {
        hubContent.primaryActionDescription = GetLocalizedString(
            hubConfig.primaryActionDescriptionLocalizationKey,
            hubConfig.primaryActionDescriptionLocalizationKey);
    }

    hubContent.widgets.reserve(hubConfig.widgets.size());
    for (const auto& widget : hubConfig.widgets)
    {
        ui::panels::HubWidgetContent widgetContent;
        widgetContent.id = widget.id;
        widgetContent.title = widget.titleLocalizationKey.empty() ? widget.id
                                                                  : GetLocalizedString(widget.titleLocalizationKey, widget.id);
        widgetContent.description = widget.descriptionLocalizationKey.empty()
            ? std::string{}
            : GetLocalizedString(widget.descriptionLocalizationKey, widget.descriptionLocalizationKey);
        for (const auto& itemKey : widget.itemLocalizationKeys)
        {
            if (!itemKey.empty())
            {
                widgetContent.items.push_back(GetLocalizedString(itemKey, itemKey));
            }
        }
        widgetContent.accent = widget.accentColor.empty() ? theme_.channelBadge
                                                          : color::ParseHexColor(widget.accentColor, theme_.channelBadge);
        hubContent.widgets.emplace_back(std::move(widgetContent));
    }

    const int widgetPageCount = hubContent.widgets.empty()
        ? 0
        : (static_cast<int>(hubContent.widgets.size()) + hubWidgetsPerPage_ - 1) / hubWidgetsPerPage_;
    hubWidgetPageCount_ = widgetPageCount;
    if (widgetPageCount == 0)
    {
        hubWidgetPage_ = 0;
    }
    else
    {
        hubWidgetPage_ = std::clamp(hubWidgetPage_, 0, widgetPageCount - 1);
    }

    if (focusedHubBranchIndex_ >= static_cast<int>(hubContent.branches.size()))
    {
        focusedHubBranchIndex_ = hubContent.branches.empty() ? -1 : 0;
    }
    if (hubContent.branches.empty())
    {
        hoveredHubBranchIndex_ = -1;
    }

    hubPanel_.Build(
        renderer,
        hubContent,
        fonts_.heroTitle.get(),
        fonts_.heroBody.get(),
        fonts_.tileTitle.get(),
        fonts_.tileSubtitle.get(),
        theme_);

    EnsureHubScrollWithinBounds();
}

void Application::ResetHubInteractionState()
{
    hoveredHubBranchIndex_ = -1;
    focusedHubBranchIndex_ = -1;
    hubBranchHitboxes_.clear();
    hubRenderedBranchIds_.clear();
    hubSearchTokens_.clear();
    hubScrollOffset_ = 0;
    hubScrollMaxOffset_ = 0;
    hubScrollViewportValid_ = false;
    hubSearchFocused_ = false;
    hubSearchQuery_.clear();
    hubWidgetPage_ = 0;
    hubWidgetPageCount_ = 0;
    hubWidgetPagerHitboxes_.clear();
    hubSearchInputRect_.reset();
    hubSearchClearRect_.reset();
    hubHeroToggleRect_.reset();
    hubDetailActionRect_.reset();
}

std::vector<std::string> Application::TokenizeHubSearch(std::string_view value) const
{
    std::vector<std::string> tokens;
    std::string normalized = NormalizeHubSearchString(value);
    std::string current;
    current.reserve(normalized.size());
    for (char ch : normalized)
    {
        if (ch == ' ')
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }
        }
        else
        {
            current.push_back(ch);
        }
    }
    if (!current.empty())
    {
        tokens.push_back(current);
    }
    return tokens;
}

std::string Application::NormalizeHubSearchString(std::string_view value) const
{
    std::string normalized;
    normalized.reserve(value.size());
    bool previousSpace = false;
    for (unsigned char raw : value)
    {
        if (std::isalnum(raw) != 0)
        {
            normalized.push_back(static_cast<char>(std::tolower(raw)));
            previousSpace = false;
        }
        else if (std::isspace(raw) != 0)
        {
            if (!previousSpace && !normalized.empty())
            {
                normalized.push_back(' ');
                previousSpace = true;
            }
        }
        else
        {
            if (!previousSpace && !normalized.empty())
            {
                normalized.push_back(' ');
                previousSpace = true;
            }
        }
    }
    if (!normalized.empty() && normalized.back() == ' ')
    {
        normalized.pop_back();
    }
    return normalized;
}

void Application::EnsureHubScrollWithinBounds()
{
    hubScrollOffset_ = std::clamp(hubScrollOffset_, 0, std::max(0, hubScrollMaxOffset_));
}

void Application::FocusHubSearch()
{
    hubSearchFocused_ = true;
    UpdateTextInputState();
}

void Application::ClearHubSearchQuery()
{
    hubSearchQuery_.clear();
    hubSearchTokens_.clear();
    hubScrollOffset_ = 0;
}

void Application::SyncFocusedHubBranch()
{
    hoveredHubBranchIndex_ = focusedHubBranchIndex_;
    if (!hubScrollViewportValid_ || focusedHubBranchIndex_ < 0)
    {
        return;
    }

    auto it = std::find_if(
        hubBranchHitboxes_.begin(),
        hubBranchHitboxes_.end(),
        [&](const ui::panels::HubRenderResult::BranchHitbox& hitbox) { return hitbox.branchIndex == focusedHubBranchIndex_; });
    if (it == hubBranchHitboxes_.end())
    {
        return;
    }

    const SDL_Rect& rect = it->rect;
    const int viewportTop = hubScrollViewport_.y;
    const int viewportBottom = hubScrollViewport_.y + hubScrollViewport_.h;
    if (rect.y < viewportTop)
    {
        hubScrollOffset_ = std::clamp(hubScrollOffset_ - (viewportTop - rect.y), 0, hubScrollMaxOffset_);
    }
    else if (rect.y + rect.h > viewportBottom)
    {
        hubScrollOffset_ = std::clamp(hubScrollOffset_ + (rect.y + rect.h - viewportBottom), 0, hubScrollMaxOffset_);
    }
}

void Application::HandleHubMouseWheel(const SDL_MouseWheelEvent& wheel)
{
    if (!hubScrollViewportValid_)
    {
        return;
    }

    int mouseX = 0;
    int mouseY = 0;
    SDL_GetMouseState(&mouseX, &mouseY);
    if (!PointInRect(hubScrollViewport_, mouseX, mouseY))
    {
        return;
    }

    const int scrollStep = ui::Scale(96);
    hubScrollOffset_ = std::clamp(
        hubScrollOffset_ - wheel.y * scrollStep,
        0,
        std::max(0, hubScrollMaxOffset_));
}

void Application::HandleHubMouseClick(int x, int y)
{
    bool handled = false;

    if (hubHeroToggleRect_ && PointInRect(*hubHeroToggleRect_, x, y))
    {
        isHubHeroCollapsed_ = !isHubHeroCollapsed_;
        handled = true;
        BuildHubPanel();
    }

    if (!handled && hubSearchClearRect_ && PointInRect(*hubSearchClearRect_, x, y) && !hubSearchQuery_.empty())
    {
        ClearHubSearchQuery();
        BuildHubPanel();
        handled = true;
    }

    if (!handled && hubSearchInputRect_ && PointInRect(*hubSearchInputRect_, x, y))
    {
        FocusHubSearch();
        handled = true;
    }

    if (!handled)
    {
        for (const auto& pagerHitbox : hubWidgetPagerHitboxes_)
        {
            if (!pagerHitbox.enabled)
            {
                continue;
            }
            if (PointInRect(pagerHitbox.rect, x, y))
            {
                const int maxPage = std::max(0, hubWidgetPageCount_ - 1);
                switch (pagerHitbox.type)
                {
                case ui::panels::HubRenderResult::WidgetPagerHitbox::Type::Previous:
                case ui::panels::HubRenderResult::WidgetPagerHitbox::Type::Next:
                case ui::panels::HubRenderResult::WidgetPagerHitbox::Type::Page:
                    hubWidgetPage_ = std::clamp(pagerHitbox.pageIndex, 0, maxPage);
                    break;
                }
                handled = true;
                break;
            }
        }
    }

    if (!handled && hubDetailActionRect_ && PointInRect(*hubDetailActionRect_, x, y))
    {
        if (focusedHubBranchIndex_ >= 0)
        {
            ActivateHubBranchByIndex(focusedHubBranchIndex_);
        }
        handled = true;
    }

    if (!handled)
    {
        for (const auto& hitbox : hubBranchHitboxes_)
        {
            if (PointInRect(hitbox.rect, x, y))
            {
                focusedHubBranchIndex_ = hitbox.branchIndex;
                hoveredHubBranchIndex_ = hitbox.branchIndex;
                hubSearchFocused_ = false;
                UpdateTextInputState();
                handled = true;
                break;
            }
        }
    }

    if (!handled)
    {
        hubSearchFocused_ = false;
    }

    if (!handled && hubSearchFocused_)
    {
        FocusHubSearch();
    }
    else if (!hubSearchFocused_)
    {
        UpdateTextInputState();
    }
}

void Application::HandleHubMouseMotion(const SDL_MouseMotionEvent& motion)
{
    int hoveredIndex = -1;
    for (const auto& hitbox : hubBranchHitboxes_)
    {
        if (PointInRect(hitbox.rect, motion.x, motion.y))
        {
            hoveredIndex = hitbox.branchIndex;
            break;
        }
    }
    hoveredHubBranchIndex_ = hoveredIndex;
}

bool Application::HandleHubKeyDown(SDL_Keycode key)
{
    const int branchCount = static_cast<int>(hubRenderedBranchIds_.size());

    switch (key)
    {
    case SDLK_ESCAPE:
        if (hubSearchFocused_)
        {
            if (!hubSearchQuery_.empty())
            {
                ClearHubSearchQuery();
                BuildHubPanel();
            }
            else
            {
                hubSearchFocused_ = false;
                UpdateTextInputState();
            }
            return true;
        }
        EnterMainInterface();
        return true;
    case SDLK_BACKSPACE:
        if (hubSearchFocused_)
        {
            if (!hubSearchQuery_.empty())
            {
                auto eraseIt = hubSearchQuery_.end();
                do
                {
                    --eraseIt;
                } while (eraseIt != hubSearchQuery_.begin() && ((*eraseIt & 0xC0) == 0x80));
                hubSearchQuery_.erase(eraseIt, hubSearchQuery_.end());
                BuildHubPanel();
            }
            else
            {
                hubSearchFocused_ = false;
                UpdateTextInputState();
            }
            return true;
        }
        EnterMainInterface();
        return true;
    case SDLK_SLASH:
        if (!hubSearchFocused_)
        {
            FocusHubSearch();
            return true;
        }
        break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        if (hubSearchFocused_)
        {
            hubSearchFocused_ = false;
            UpdateTextInputState();
            return true;
        }
        if (focusedHubBranchIndex_ >= 0 && focusedHubBranchIndex_ < branchCount)
        {
            ActivateHubBranchByIndex(focusedHubBranchIndex_);
        }
        return true;
    case SDLK_SPACE:
        if (hubSearchFocused_)
        {
            return false;
        }
        if (focusedHubBranchIndex_ >= 0 && focusedHubBranchIndex_ < branchCount)
        {
            ActivateHubBranchByIndex(focusedHubBranchIndex_);
        }
        return true;
    case SDLK_PAGEDOWN:
        if (hubScrollViewportValid_)
        {
            hubScrollOffset_ = std::clamp(hubScrollOffset_ + hubScrollViewport_.h, 0, hubScrollMaxOffset_);
            return true;
        }
        break;
    case SDLK_PAGEUP:
        if (hubScrollViewportValid_)
        {
            hubScrollOffset_ = std::clamp(hubScrollOffset_ - hubScrollViewport_.h, 0, hubScrollMaxOffset_);
            return true;
        }
        break;
    case SDLK_HOME:
        hubScrollOffset_ = 0;
        if (branchCount > 0)
        {
            focusedHubBranchIndex_ = 0;
            SyncFocusedHubBranch();
        }
        return true;
    case SDLK_END:
        hubScrollOffset_ = hubScrollMaxOffset_;
        if (branchCount > 0)
        {
            focusedHubBranchIndex_ = branchCount - 1;
            SyncFocusedHubBranch();
        }
        return true;
    case SDLK_LEFT:
    case SDLK_UP:
        if (branchCount == 0)
        {
            return true;
        }
        if (focusedHubBranchIndex_ < 0)
        {
            focusedHubBranchIndex_ = branchCount - 1;
        }
        else
        {
            focusedHubBranchIndex_ = (focusedHubBranchIndex_ - 1 + branchCount) % branchCount;
        }
        SyncFocusedHubBranch();
        return true;
    case SDLK_RIGHT:
    case SDLK_DOWN:
        if (branchCount == 0)
        {
            return true;
        }
        if (focusedHubBranchIndex_ < 0)
        {
            focusedHubBranchIndex_ = 0;
        }
        else
        {
            focusedHubBranchIndex_ = (focusedHubBranchIndex_ + 1) % branchCount;
        }
        SyncFocusedHubBranch();
        return true;
    case SDLK_TAB:
        if (branchCount == 0)
        {
            return true;
        }
        if ((SDL_GetModState() & KMOD_SHIFT) != 0)
        {
            if (focusedHubBranchIndex_ < 0)
            {
                focusedHubBranchIndex_ = branchCount - 1;
            }
            else
            {
                focusedHubBranchIndex_ = (focusedHubBranchIndex_ - 1 + branchCount) % branchCount;
            }
        }
        else
        {
            if (focusedHubBranchIndex_ < 0)
            {
                focusedHubBranchIndex_ = 0;
            }
            else
            {
                focusedHubBranchIndex_ = (focusedHubBranchIndex_ + 1) % branchCount;
            }
        }
        SyncFocusedHubBranch();
        return true;
    default:
        break;
    }

    return false;
}


void Application::ActivateHubBranch(const std::string& branchId)
{
    const int index = FindHubBranchIndexById(branchId);
    if (index < 0)
    {
        return;
    }

    auto visibleIt = std::find(hubRenderedBranchIds_.begin(), hubRenderedBranchIds_.end(), branchId);
    if (visibleIt != hubRenderedBranchIds_.end())
    {
        focusedHubBranchIndex_ = static_cast<int>(std::distance(hubRenderedBranchIds_.begin(), visibleIt));
    }
    else
    {
        focusedHubBranchIndex_ = -1;
    }
    const auto& branch = content_.hub.branches[static_cast<std::size_t>(index)];

    EnterMainInterface();

    int targetChannelIndex = -1;
    if (!branch.channelId.empty())
    {
        for (std::size_t i = 0; i < content_.channels.size(); ++i)
        {
            if (content_.channels[i].id == branch.channelId)
            {
                targetChannelIndex = static_cast<int>(i);
                break;
            }
        }
    }

    auto findChannelForProgram = [&](const std::string& programId) {
        for (std::size_t i = 0; i < content_.channels.size(); ++i)
        {
            const auto& channel = content_.channels[i];
            if (std::find(channel.programs.begin(), channel.programs.end(), programId) != channel.programs.end())
            {
                return static_cast<int>(i);
            }
        }
        return -1;
    };

    const bool hasProgramTarget = !branch.programId.empty();
    if (targetChannelIndex == -1 && hasProgramTarget)
    {
        targetChannelIndex = findChannelForProgram(branch.programId);
    }

    if (targetChannelIndex != -1)
    {
        navigationController_.Activate(targetChannelIndex);
        if (hasProgramTarget)
        {
            auto& channel = content_.channels[static_cast<std::size_t>(targetChannelIndex)];
            auto it = std::find(channel.programs.begin(), channel.programs.end(), branch.programId);
            if (it != channel.programs.end())
            {
                const int programIndex = static_cast<int>(std::distance(channel.programs.begin(), it));
                channelSelections_[targetChannelIndex] = programIndex;
                ActivateProgramInChannel(programIndex);
            }
            else
            {
                ActivateProgram(branch.programId);
            }
        }
    }
    else if (hasProgramTarget)
    {
        ActivateProgram(branch.programId);
    }
}

void Application::ActivateHubBranchByIndex(int index)
{
    if (index < 0 || index >= static_cast<int>(hubRenderedBranchIds_.size()))
    {
        return;
    }

    ActivateHubBranch(hubRenderedBranchIds_[static_cast<std::size_t>(index)]);
}

int Application::FindHubBranchIndexById(const std::string& branchId) const
{
    for (std::size_t i = 0; i < content_.hub.branches.size(); ++i)
    {
        if (content_.hub.branches[i].id == branchId)
        {
            return static_cast<int>(i);
        }
    }

    return -1;
}

} // namespace colony

