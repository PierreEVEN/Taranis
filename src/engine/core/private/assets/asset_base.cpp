#include "assets/asset_base.hpp"

#include "assets/asset_registry.hpp"

namespace Eng
{

AssetBase::~AssetBase()
{
    free(name);
    if (auto cl = registry->assets.find(get_class()); cl != registry->assets.end())
        cl->second.erase(this);
}

} // namespace Eng