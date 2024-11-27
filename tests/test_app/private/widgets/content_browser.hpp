#pragma once
#include "assets/asset_registry.hpp"
#include "gfx/ui/ui_window.hpp"

#include <array>

namespace Eng
{
class AssetRegistry;
}

class ContentBrowser : public Eng::UiWindow
{
  public:
    ContentBrowser(const std::string& name, Eng::AssetRegistry& asset_registry) : UiWindow(name), registry(&asset_registry)
    {
    }

  protected:
    void draw(Eng::Gfx::ImGuiWrapper& ctx) override;

    void drawHierarchy();
    void drawHierarchy(const std::filesystem::path& f);

    void draw_asset_thumbnail(const TObjectPtr<Eng::AssetBase>& asset);
    void draw_asset_button(const TObjectPtr<Eng::AssetBase>& asset);

    Eng::AssetRegistry*                  registry;
    std::optional<std::filesystem::path> selected_file;
    bool                                 show_all_content   = true;
    bool                                 b_set_column_width = true;
    std::array<char, 50>                 filter;
};