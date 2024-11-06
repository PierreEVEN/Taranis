#include "assets/asset_base.hpp"

#include "assets/asset_registry.hpp"
#include "engine.hpp"

namespace Engine
{
void AssetBase::destroy()
{

    Engine::get().asset_registry().destroy_object(this);
}
} // namespace Engine