#include "assets/asset_base.hpp"

#include "engine.hpp"
#include "assets/asset_registry.hpp"

namespace Engine
{
void AssetBase::destroy()
{

    Engine::get().asset_registry().destroy_object(this);

}
}