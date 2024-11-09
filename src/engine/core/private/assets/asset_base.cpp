#include "assets/asset_base.hpp"

#include "assets/asset_registry.hpp"

namespace Eng
{

AssetBase::~AssetBase()
{
    free(name);
    registry->assets.erase(this);
}

} // namespace Eng