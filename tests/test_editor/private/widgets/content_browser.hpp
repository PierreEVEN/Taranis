#pragma once
#include "assets/asset_registry.hpp"
#include "gfx/ui/ui_window.hpp"

#include <array>

namespace Eng
{
class Scene;
}

namespace Eng
{
class AssetRegistry;
}

class ContentBrowser : public Eng::UiWindow
{
public:
    ContentBrowser(const std::string& name, Eng::AssetRegistry& asset_registry, std::shared_ptr<Eng::Scene> in_scene);

protected:
    void draw(Eng::Gfx::ImGuiWrapper& ctx) override;

    void drawHierarchy();
    void drawHierarchy(const std::filesystem::path& f);

    void draw_asset_thumbnail(const TObjectPtr<Eng::AssetBase>& asset, Eng::Gfx::ImGuiWrapper& ctx);
    void draw_asset_button(const TObjectPtr<Eng::AssetBase>& asset, Eng::Gfx::ImGuiWrapper& ctx);

    Eng::AssetRegistry*                  registry;
    std::optional<std::filesystem::path> selected_file;
    bool                                 show_all_content   = true;
    bool                                 b_set_column_width = true;
    std::array<char, 50>                 filter;
    size_t                               internal_draw_id = 0;
    std::shared_ptr<Eng::Scene>          scene;
};