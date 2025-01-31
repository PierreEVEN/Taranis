#pragma once
#include "scene_component.hpp"
#include "scene/components/primitive_component.gen.hpp"

namespace Eng
{
class SceneView;

class PrimitiveComponent : public SceneComponent
{
    REFLECT_BODY()

public:
    virtual void draw(Gfx::CommandBuffer& command_buffer, const SceneView& view) = 0;
};
}
