#pragma once

#include <SDL2/SDL.h>

#include <functional>
#include <unordered_map>
#include <vector>

namespace colony::input
{

class InputRouter
{
  public:
    using Handler = std::function<bool(const SDL_Event&, bool&)>;

    void RegisterHandler(Uint32 eventType, Handler handler);
    bool Dispatch(const SDL_Event& event, bool& running) const;

  private:
    std::unordered_map<Uint32, std::vector<Handler>> handlers_{};
};

} // namespace colony::input
