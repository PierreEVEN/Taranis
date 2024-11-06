#pragma once
#include "macros.hpp"

#include <memory>
#include <vector>
#include "scene\scene.gen.hpp"

namespace Engine
{
class SceneComponent;

class Scene
{
    REFLECT_BODY();

  public:




private:
    std::vector<std::shared_ptr<SceneComponent>> objects;
};
} // namespace Engine
