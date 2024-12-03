#include "content_browser.hpp"

#include <imgui.h>
#include <ranges>

void ContentBrowser::draw(Eng::Gfx::ImGuiWrapper&)
{

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {10, 7});
    if (ImGui::Button("Create"))
    {
        ImGui::OpenPopup("CreatePopup");
    }

    if (ImGui::BeginPopup("CreatePopup"))
    {
        ImGui::EndPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Save All"))
        LOG_DEBUG("NOPE FOR NOW");

    ImGui::PopStyleVar();

    ImGui::Separator();
    drawHierarchy();

    ImGui::NextColumn();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {10, 3});
    ImGui::InputText("##searchBox", filter.data(), filter.size());

    // drawFilters();
    ImGui::Dummy({0, 5});

    ImGui::PopStyleVar();

    if (ImGui::BeginChild("contentAssets"))
    {

        float sizeX = ImGui::GetContentRegionAvail().x;

        int widthItems = static_cast<int>(sizeX / 70);
        ImGui::Columns(std::max(widthItems, 1), "", false);

        registry->for_each(
            [this](const TObjectPtr<Eng::AssetBase>& asset)
            {
                draw_asset_thumbnail(asset);
                ImGui::NextColumn();
            });
        ImGui::Columns(1);
    }
    ImGui::EndChild();
    ImGui::Columns(1);
}

void ContentBrowser::drawHierarchy()
{
    // Initialize left side size
    float windowSize = std::max(ImGui::GetContentRegionAvail().x / 7.f, 150.f);
    ImGui::Columns(2);
    if (b_set_column_width)
    {
        ImGui::SetColumnWidth(0, windowSize);
        b_set_column_width = false;
    }
    if (ImGui::BeginChild("folders"))
    {
        drawHierarchy("./resources");
    }
    ImGui::EndChild();
}

void ContentBrowser::drawHierarchy(const std::filesystem::path& f)
{
    if (!exists(f))
        return;
    int  flags           = ImGuiTreeNodeFlags_OpenOnDoubleClick;
    bool bHasFolderChild = false;

    for (auto const& dir_entry : std::filesystem::directory_iterator{f})
    {
        if (dir_entry.is_directory())
            for (auto const& child : std::filesystem::directory_iterator{dir_entry})
            {
                if (child.is_directory())
                {
                    bHasFolderChild = true;
                    break;
                }
            }
    }
    if (!bHasFolderChild)
        flags |= ImGuiTreeNodeFlags_Leaf;
    if (selected_file == f)
        flags |= ImGuiTreeNodeFlags_Selected;
    bool bExpand = ImGui::TreeNodeEx(f.filename().string().c_str(), flags);
    if (ImGui::IsItemClicked())
    {
        selected_file    = f;
        show_all_content = false;
    }
    if (bExpand)
    {
        for (auto const& child : std::filesystem::directory_iterator{f})
        {
            if (child.is_directory())
                drawHierarchy(child);
        }
        ImGui::TreePop();
    }
}

void ContentBrowser::draw_asset_thumbnail(const TObjectPtr<Eng::AssetBase>& asset)
{
    if (!asset)
        return;

    ImGui::BeginGroup();
    draw_asset_button(asset);
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        ImGui::SetDragDropPayload("DDOP_ASSET", asset->get_name(), std::string(asset->get_name()).size());
        draw_asset_button(asset);
        ImGui::EndDragDropSource();
    }
    ImGui::TextWrapped(asset->get_name());
    ImGui::EndGroup();
}

void ContentBrowser::draw_asset_button(const TObjectPtr<Eng::AssetBase>& asset)
{
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 0.2f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0, 4});

    // if (thumbnail == null)
    {
        ImGui::Dummy({0, 4});
        if (ImGui::Button(("#" + std::string(asset->get_name())).c_str(), {64, 64}))
        {
        }
    }
    /*else
    {
        if (ImGui.imageButton(thumbnail.getTextureHandle(), 64, 64, 0, 1, 1, 0))
        {
            new AssetWindow(asset, asset.getName());
        }
    }*/
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}