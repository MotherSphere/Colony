#include "views/view_factory.hpp"

#include "views/simple_text_view.hpp"

namespace colony
{

ViewPtr ViewFactory::CreateSimpleTextView(const std::string& id) const
{
    return std::make_unique<SimpleTextView>(id);
}

} // namespace colony
