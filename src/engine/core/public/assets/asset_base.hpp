#pragma once
#include <string>

namespace Engine
{
enum class AssetType
{
    Mesh,
    Texture,
    Material,
    MaterialInstance
};


class AssetBase
{
public:
    virtual ~AssetBase() = default;

    const char* get_name() const
    {
        return name;
    }

    void destroy();

    virtual AssetType get_type() const = 0;

protected:
    AssetBase()
    {
    }

private:
    friend class AssetRegistry;
    char*  name;
    void*  base_ptr;
    size_t type_size;
};
}