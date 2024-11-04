#include <utility>

#include "gfx/ui/ImGuiWrapper.hpp"

#include <imgui.h>

#include "engine.hpp"
#include "gfx/mesh.hpp"
#include "gfx/vulkan/command_buffer.hpp"
#include "gfx/vulkan/descriptor_sets.hpp"
#include "gfx/vulkan/image.hpp"
#include "gfx/vulkan/image_view.hpp"
#include "gfx/vulkan/pipeline.hpp"
#include "gfx/vulkan/sampler.hpp"
#include "gfx/vulkan/shader_module.hpp"
#include "gfx/window.hpp"

static const char* IMGUI_VERTEX = "\
struct VSInput {\
    [[vk::location(0)]] float2 aPos : POSITION;\
    [[vk::location(1)]] float2 aUV : TEXCOORD;\
    [[vk::location(2)]] float4 aColor : COLOR;\
};\
struct VsToFs {\
    float4 Pos : SV_Position;\
    float4 Color : COLOR;\
    float2 UV : TEXCOORD;\
};\
struct PushConsts {\
    float2 uScale;\
    float2 uTranslate;\
};\
[[vk::push_constant]] ConstantBuffer<PushConsts> pc;\
VsToFs main(VSInput input) {\
    VsToFs Out;\
    Out.Color = input.aColor;\
    Out.UV = input.aUV;\
    Out.Pos = float4(input.aPos * pc.uScale + pc.uTranslate, 0, 1);\
    return Out;\
}";

static const char* IMGUI_FRAGMENT = "\
struct VsToFs {\
    float4 Pos : SV_Position;\
    float4 Color : COLOR;\
    float2 UV : TEXCOORD;\
};\
[[vk::binding(0)]] Texture2D	 sTexture;\
[[vk::binding(1)]] SamplerState sSampler;\
\
float4 main(VsToFs input) : SV_TARGET{\
    return input.Color * sTexture.Sample(sSampler, input.UV);\
}";

namespace Engine
{
ImGuiWrapper::ImGuiWrapper(std::string in_name, const std::weak_ptr<VkRendererPass>& render_pass, std::weak_ptr<Device> in_device, std::weak_ptr<Window> in_target_window)
    : device(std::move(in_device)), target_window(std::move(in_target_window)), name(std::move(in_name))
{
    mesh = std::make_shared<Mesh>(name + "_mesh", device, sizeof(ImDrawVert), EBufferType::IMMEDIATE);

    image_sampler = std::make_shared<Sampler>(name + "_generic_sampler", device, Sampler::CreateInfos{});

    ShaderCompiler compiler;
    const auto     vertex_code   = compiler.compile_raw(IMGUI_VERTEX, "main", EShaderStage::Vertex, "internal://imgui");
    const auto     fragment_code = compiler.compile_raw(IMGUI_FRAGMENT, "main", EShaderStage::Fragment, "internal://imgui");
    if (!vertex_code)
        LOG_FATAL("Failed to compile imgui vertex shader : {}", vertex_code.error())
    if (!fragment_code)
        LOG_FATAL("Failed to compile imgui fragment shader : {}", fragment_code.error())
    const auto vertex_temp = std::make_shared<ShaderModule>(device, vertex_code.get());

    imgui_material =
        std::make_shared<Pipeline>(name + "_pipeline", device, render_pass, std::vector{std::make_shared<ShaderModule>(device, vertex_code.get()), std::make_shared<ShaderModule>(device, fragment_code.get())},
                                   Pipeline::CreateInfos{.culling              = ECulling::None,
                                                         .alpha_mode           = EAlphaMode::Translucent,
                                                         .depth_test           = false,
                                                         .stage_input_override = std::vector{
                                                             StageInputOutputDescription{0, 0, ColorFormat::R32G32_SFLOAT},
                                                             StageInputOutputDescription{1, 8, ColorFormat::R32G32_SFLOAT},
                                                             StageInputOutputDescription{2, 16, ColorFormat::R8G8B8A8_UNORM},
                                                         }});

    imgui_context = ImGui::CreateContext();
    ImGui::SetCurrentContext(imgui_context);
    IMGUI_CHECKVERSION();

    ImGuiIO& io = ImGui::GetIO();
    if (io.BackendPlatformUserData != nullptr)
        LOG_FATAL("Imgui has already been initialized !")

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    io.BackendPlatformUserData = nullptr;
    io.BackendPlatformName     = "imgui_impl_internal";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors; // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
    // We can honor io.WantSetMousePos requests (optional, rarely used)
    io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;
    // We can create multi-viewports on the Platform side (optional)
    // io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can set io.MouseHoveredViewport correctly (optional, not easy)

    io.KeyMap[ImGuiKey_Tab]        = static_cast<int>(GLFW_KEY_TAB);
    io.KeyMap[ImGuiKey_LeftArrow]  = static_cast<int>(GLFW_KEY_LEFT);
    io.KeyMap[ImGuiKey_RightArrow] = static_cast<int>(GLFW_KEY_RIGHT);
    io.KeyMap[ImGuiKey_UpArrow]    = static_cast<int>(GLFW_KEY_UP);
    io.KeyMap[ImGuiKey_DownArrow]  = static_cast<int>(GLFW_KEY_DOWN);
    io.KeyMap[ImGuiKey_PageUp]     = static_cast<int>(GLFW_KEY_PAGE_UP);
    io.KeyMap[ImGuiKey_PageDown]   = static_cast<int>(GLFW_KEY_PAGE_DOWN);
    io.KeyMap[ImGuiKey_Home]       = static_cast<int>(GLFW_KEY_HOME);
    io.KeyMap[ImGuiKey_End]        = static_cast<int>(GLFW_KEY_END);
    io.KeyMap[ImGuiKey_Insert]     = static_cast<int>(GLFW_KEY_INSERT);
    io.KeyMap[ImGuiKey_Delete]     = static_cast<int>(GLFW_KEY_DELETE);
    io.KeyMap[ImGuiKey_Backspace]  = static_cast<int>(GLFW_KEY_BACKSPACE);
    io.KeyMap[ImGuiKey_Space]      = static_cast<int>(GLFW_KEY_SPACE);
    io.KeyMap[ImGuiKey_Enter]      = static_cast<int>(GLFW_KEY_ENTER);
    io.KeyMap[ImGuiKey_Escape]     = static_cast<int>(GLFW_KEY_ESCAPE);
    io.KeyMap[ImGuiKey_A]          = static_cast<int>(GLFW_KEY_A);
    io.KeyMap[ImGuiKey_C]          = static_cast<int>(GLFW_KEY_C);
    io.KeyMap[ImGuiKey_V]          = static_cast<int>(GLFW_KEY_V);
    io.KeyMap[ImGuiKey_X]          = static_cast<int>(GLFW_KEY_X);
    io.KeyMap[ImGuiKey_Y]          = static_cast<int>(GLFW_KEY_Y);
    io.KeyMap[ImGuiKey_Z]          = static_cast<int>(GLFW_KEY_Z);

    // io.SetClipboardTextFn = set_clipboard_text;
    // io.GetClipboardTextFn = get_clipboard_text;

    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark();
    style.WindowRounding     = 0;
    style.ScrollbarRounding  = 0;
    style.TabRounding        = 0;
    style.WindowBorderSize   = 1;
    style.PopupBorderSize    = 1;
    style.WindowTitleAlign.x = 0.5f;
    style.FramePadding.x     = 6.f;
    style.FramePadding.y     = 6.f;
    style.WindowPadding.x    = 4.f;
    style.WindowPadding.y    = 4.f;
    style.GrabMinSize        = 16.f;
    style.ScrollbarSize      = 20.f;
    style.IndentSpacing      = 30.f;

    ImGuiViewport* main_viewport  = ImGui::GetMainViewport();
    main_viewport->PlatformHandle = nullptr;

    cursor_map[ImGuiMouseCursor_Arrow]      = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    cursor_map[ImGuiMouseCursor_TextInput]  = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    cursor_map[ImGuiMouseCursor_ResizeAll]  = glfwCreateStandardCursor(GLFW_RESIZE_ALL_CURSOR);
    cursor_map[ImGuiMouseCursor_ResizeNS]   = glfwCreateStandardCursor(GLFW_RESIZE_NS_CURSOR);
    cursor_map[ImGuiMouseCursor_ResizeEW]   = glfwCreateStandardCursor(GLFW_RESIZE_EW_CURSOR);
    cursor_map[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);
    cursor_map[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
    cursor_map[ImGuiMouseCursor_Hand]       = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
    cursor_map[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_NOT_ALLOWED_CURSOR);

    // Create font texture
    uint8_t* pixels;
    int      width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    font_texture          = std::make_shared<Image>(name + "_font_image", device,
                                                    ImageParameter{
                                                        .format      = ColorFormat::R8G8B8A8_UNORM,
                                                        .buffer_type = EBufferType::IMMUTABLE,
                                                        .width       = static_cast<uint32_t>(width),
                                                        .height      = static_cast<uint32_t>(height),
                                           },
                                                    BufferData(pixels, 4, width * height));
    font_texture_view     = std::make_shared<ImageView>(name + "_font_image_view", font_texture);
    imgui_font_descriptor = DescriptorSet::create(name + "_font_descriptors", device, imgui_material);
    imgui_font_descriptor->bind_image("sTexture", font_texture_view);
    imgui_font_descriptor->bind_sampler("sSampler", image_sampler);
}

ImGuiWrapper::~ImGuiWrapper()
{
    for (size_t i = 0; i < ImGuiMouseCursor_COUNT; ++i)
        if (cursor_map[i])
            glfwDestroyCursor(cursor_map[i]);
}

void ImGuiWrapper::draw(const CommandBuffer& cmd, glm::uvec2 draw_res)
{
    ImGui::SetCurrentContext(imgui_context);
    ImGuiIO& io = ImGui::GetIO();
    // Setup display size (every frame to accommodate for start_window resizing)
    io.DisplaySize             = ImVec2(static_cast<float>(draw_res.x), static_cast<float>(draw_res.y));
    io.DisplayFramebufferScale = ImVec2(1.0, 1.0);
    io.DeltaTime               = static_cast<float>(Engine::get().delta_second);

    // Update mouse
    io.MouseDown[0]         = glfwGetMouseButton(target_window.lock()->raw(), GLFW_MOUSE_BUTTON_1);
    io.MouseDown[1]         = glfwGetMouseButton(target_window.lock()->raw(), GLFW_MOUSE_BUTTON_2);
    io.MouseDown[2]         = glfwGetMouseButton(target_window.lock()->raw(), GLFW_MOUSE_BUTTON_3);
    io.MouseDown[3]         = glfwGetMouseButton(target_window.lock()->raw(), GLFW_MOUSE_BUTTON_4);
    io.MouseDown[4]         = glfwGetMouseButton(target_window.lock()->raw(), GLFW_MOUSE_BUTTON_5);
    io.MouseHoveredViewport = 0;

    double x_pos, y_pos;
    glfwGetCursorPos(target_window.lock()->raw(), &x_pos, &y_pos);

    io.MousePos = ImVec2(static_cast<float>(x_pos), static_cast<float>(y_pos));

    const ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (imgui_cursor != ImGuiMouseCursor_None)
        glfwSetCursor(target_window.lock()->raw(), cursor_map[imgui_cursor]);

    ImGui::NewFrame();

    if (ImGui::Begin("test"))
    {
        ImGui::LabelText("ms", "%lf", Engine::get().delta_second * 1000);
        ImGui::LabelText("fps", "%lf", 1.0 / Engine::get().delta_second);
    }
    ImGui::End();

    ImGui::ShowDemoWindow(nullptr);

    ImGui::EndFrame();
    ImGui::Render();
    const auto* draw_data = ImGui::GetDrawData();
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    const int fb_width  = static_cast<int>(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    const int fb_height = static_cast<int>(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0 || draw_data->TotalVtxCount == 0)
        return;

    /**
     * BUILD VERTEX BUFFERS
     */

    size_t vtx_size = 0;
    size_t idx_size = 0;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        vtx_size += cmd_list->VtxBuffer.Size;
        idx_size += cmd_list->IdxBuffer.Size;
    }
    mesh->reserve_indices(idx_size, IndexBufferType::Uint16);
    mesh->reserve_vertices(vtx_size);

    size_t vtx_offset = 0;
    size_t idx_offset = 0;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        mesh->set_indexed_vertices(vtx_offset, BufferData(cmd_list->VtxBuffer.Data, sizeof(ImDrawVert), cmd_list->VtxBuffer.Size), idx_offset,
                                   BufferData(cmd_list->IdxBuffer.Data, sizeof(ImDrawIdx), cmd_list->IdxBuffer.Size));
        vtx_offset += cmd_list->VtxBuffer.Size;
        idx_offset += cmd_list->IdxBuffer.Size;
    }

    /**
     * PREPARE MATERIALS
     */

    float scale_x = 2.0f / draw_data->DisplaySize.x;
    float scale_y = -2.0f / draw_data->DisplaySize.y;
    const struct ConstantData
    {
        float scale_x;
        float scale_y;
        float translate_x;
        float translate_y;
    } constant_data = {
        .scale_x     = scale_x,
        .scale_y     = scale_y,
        .translate_x = -1.0f - draw_data->DisplayPos.x * scale_x,
        .translate_y = 1.0f - draw_data->DisplayPos.y * scale_y,
    };
    cmd.push_constant(EShaderStage::Vertex, *imgui_material, constant_data);

    /**
     * Draw meshs
     */

    // Will project scissor/clipping rectangles into framebuffer space
    const ImVec2 clip_off   = draw_data->DisplayPos; // (0,0) unless using multi-viewports
    const ImVec2 clip_scale = draw_data->FramebufferScale;
    // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int global_vtx_offset = 0;
    int global_idx_offset = 0;

    cmd.bind_pipeline(*imgui_material);

    bool used_other_image = true;

    std::unordered_set<ImTextureID> unused_image;
    std::ranges::transform(per_image_ids, std::inserter(unused_image, unused_image.end()),
                           [](auto pair)
                           {
                               return pair.first;
                           });

    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr)
                pcmd->UserCallback(cmd_list, pcmd);
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec4 clip_rect;
                clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

                if (clip_rect.x < static_cast<float>(fb_width) && clip_rect.y < static_cast<float>(fb_height) && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
                {
                    // Negative offsets are illegal for vkCmdSetScissor
                    if (clip_rect.x < 0.0f)
                        clip_rect.x = 0.0f;
                    if (clip_rect.y < 0.0f)
                        clip_rect.y = 0.0f;

                    // Apply scissor / clipping rectangle
                    cmd.set_scissor(Scissor{
                        .offset_x = static_cast<int32_t>(clip_rect.x),
                        .offset_y = static_cast<int32_t>(clip_rect.y),
                        .width    = static_cast<uint32_t>(clip_rect.z - clip_rect.x),
                        .height   = static_cast<uint32_t>(clip_rect.w - clip_rect.y),
                    });

                    // Bind descriptor set with font or user texture
                    if (pcmd->TextureId)
                    {
                        used_other_image = true;
                        if (const auto found_image_view = per_image_ids.find(pcmd->TextureId); found_image_view != per_image_ids.end())
                        {
                            unused_image.erase(pcmd->TextureId);
                            if (const auto found_descriptors = per_image_descriptor.find(found_image_view->second); found_descriptors != per_image_descriptor.end())
                                cmd.bind_descriptors(*found_descriptors->second.second, *imgui_material);
                        }
                    }
                    else if (used_other_image)
                        cmd.bind_descriptors(*imgui_font_descriptor, *imgui_material);

                    cmd.draw_mesh(*mesh, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, pcmd->ElemCount);
                }
            }
        }
        global_idx_offset += cmd_list->IdxBuffer.Size;
        global_vtx_offset += cmd_list->VtxBuffer.Size;
    }

    // Clear unused descriptors
    for (const auto& image_id : unused_image)
        if (const auto image = per_image_ids.find(image_id); image != per_image_ids.end())
        {
            per_image_descriptor.erase(image->second);
            per_image_ids.erase(image_id);
        }
}

ImTextureID ImGuiWrapper::add_image(const std::shared_ptr<ImageView>& image_view)
{
    auto found_descriptor = per_image_descriptor.find(image_view);
    if (found_descriptor == per_image_descriptor.end())
    {
        assert(max_texture_id != UINT64_MAX);
        const auto descriptors = DescriptorSet::create(name + "_descriptor:" + image_view->get_name(), device, imgui_material, true);
        descriptors->bind_image("sTexture", image_view);
        descriptors->bind_sampler("sSampler", image_sampler);
        ImTextureID new_id = ++max_texture_id;
        per_image_descriptor.emplace(image_view, std::pair{new_id, descriptors});
        per_image_ids.emplace(new_id, image_view);
        return new_id;
    }
    return found_descriptor->second.first;
}
} // namespace Engine