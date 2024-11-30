#pragma once
#include <memory>

namespace Eng
{
class Scene;
}

class SceneView
{
public:
    SceneView(const std::weak_ptr<Eng::Scene>& in_source_scene)
    {
        source_scene = in_source_scene;
    }

  private:
    std::weak_ptr<Eng::Scene> source_scene;
};
