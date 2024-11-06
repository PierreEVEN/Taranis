#pragma once
#include <memory>

namespace Engine::Gfx
{
class Device;
class CommandBuffer;
class RenderPassInstanceBase;

class RenderPassInterface
{
  public:
    virtual void init(const std::weak_ptr<Device>& device, const RenderPassInstanceBase& render_pass)   = 0;
    virtual void render(const RenderPassInstanceBase& render_pass, const CommandBuffer& command_buffer) = 0;
};

class DefaultRenderPassInterface : public RenderPassInterface
{
  public:
    void init(const std::weak_ptr<Device>&, const RenderPassInstanceBase&) override
    {
    }

    void render(const RenderPassInstanceBase&, const CommandBuffer&) override
    {
    }
};
} // namespace Engine