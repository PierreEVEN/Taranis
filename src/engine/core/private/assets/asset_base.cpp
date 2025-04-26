#include "assets/asset_base.hpp"

#include "assets/asset_registry.hpp"

namespace Eng
{

AssetBase::~AssetBase()
{
    registry->unregister_object(base_class, this);
    free(name);
    name = nullptr;
}

} // namespace Eng