#pragma once
#include <memory>

namespace Eng::Gfx
{
class Device;

class DeviceResource : public std::enable_shared_from_this<DeviceResource>
{
  public:
    DeviceResource(std::weak_ptr<Device> in_device) : device_ref(std::move(in_device))
    {
    }

    virtual ~DeviceResource() = default;

    const std::weak_ptr<Device>& device() const
    {
        return device_ref;
    }

  private:
    std::weak_ptr<Device> device_ref;
};
} // namespace Eng::Gfx