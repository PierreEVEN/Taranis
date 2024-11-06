#pragma once
#include "class.hpp"

#include "assets\asset_base.gen.hpp"
#include <string>

namespace Engine
{
class AssetBase
{
    REFLECT_BODY()
  public:
    AssetBase(AssetBase&)  = delete;
    AssetBase(AssetBase&&) = delete;
    virtual ~AssetBase()   = default;

    const char* get_name() const
    {
        return name;
    }

    void destroy();

  protected:
    AssetBase() = default;

  private:
    friend class AssetRegistry;
    char*  name;
    void*  base_ptr;
    size_t type_size;
};
} // namespace Engine
