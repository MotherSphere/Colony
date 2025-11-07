#pragma once

#include "views/view.hpp"

#include <memory>
#include <string>
#include <string_view>

namespace colony
{

class ViewFactory
{
  public:
    ViewPtr CreateSimpleTextView(const std::string& id) const;
    ViewPtr CreateView(const std::string& id, std::string_view type) const;
};

} // namespace colony
