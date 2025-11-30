#include "input/input_router.h"

namespace colony::input
{

void InputRouter::RegisterHandler(Uint32 eventType, Handler handler)
{
    handlers_[eventType].push_back(std::move(handler));
}

bool InputRouter::Dispatch(const SDL_Event& event, bool& running) const
{
    auto it = handlers_.find(event.type);
    if (it == handlers_.end())
    {
        return false;
    }

    for (const auto& handler : it->second)
    {
        if (handler(event, running))
        {
            return true;
        }
    }

    return false;
}

} // namespace colony::input
