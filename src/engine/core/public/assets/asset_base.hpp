#pragma once
#include <string>

namespace Engine
{
class AssetBase
{
public:
    AssetBase(AssetBase&)  = delete;
    AssetBase(AssetBase&&) = delete;
    virtual ~AssetBase()   = default;

    const std::string& get_name() const
    {
        return name;
    }

    void destroy();

protected:
    AssetBase() = default;

private:
    friend class AssetRegistry;
    std::string name;
    void*       base_ptr;
    size_t      type_size;
};
}