#pragma once
#include <memory>
#include <string>

namespace Eng::Gfx
{
class Device;

class DeviceResource : public std::enable_shared_from_this<DeviceResource>
{
public:
    DeviceResource(std::string name, std::weak_ptr<Device> in_device) : _resource_name(std::move(name)), _device_ref(std::move(in_device))
    {
    }

    virtual ~DeviceResource() = default;

    const std::weak_ptr<Device>& device() const
    {
        return _device_ref;
    }

    const std::string& name() const
    {
        return _resource_name;
    }

private:
    std::string           _resource_name;
    std::weak_ptr<Device> _device_ref;
};
} // namespace Eng::Gfx