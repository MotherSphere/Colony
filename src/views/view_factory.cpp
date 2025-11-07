#include "views/view_factory.hpp"

#include "views/chart_view.hpp"
#include "views/media_view.hpp"
#include "views/simple_text_view.hpp"

#include <string_view>

namespace colony
{

ViewPtr ViewFactory::CreateSimpleTextView(const std::string& id) const
{
    return std::make_unique<SimpleTextView>(id);
}

ViewPtr ViewFactory::CreateView(const std::string& id, std::string_view type) const
{
    if (type == "chart")
    {
        return std::make_unique<ChartView>(id);
    }
    if (type == "media")
    {
        return std::make_unique<MediaView>(id);
    }
    return CreateSimpleTextView(id);
}

} // namespace colony
