#pragma once

#include "views/view.hpp"

#include <memory>
#include <string>

namespace colony
{

class ViewFactory
{
  public:
    ViewPtr CreateSimpleTextView(const std::string& id) const;
};

} // namespace colony
