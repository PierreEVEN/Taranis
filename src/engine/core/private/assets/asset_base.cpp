#include "assets/asset_base.hpp"

#include "assets/asset_registry.hpp"

namespace Engine
{

Engine::AssetBase::~AssetBase()
{
    free(name);
    registry->assets.erase(this);
}

} // namespace Engine