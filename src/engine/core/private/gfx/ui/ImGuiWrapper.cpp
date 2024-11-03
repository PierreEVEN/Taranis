
#include <utility>

#include "gfx/ui/ImGuiWrapper.hpp"

#include <imgui.h>

#include "gfx/mesh.hpp"
#include "gfx/vulkan/image.hpp"
#include "gfx/vulkan/pipeline.hpp"
#include "gfx/vulkan/shader_module.hpp"

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
	ImGuiWrapper::ImGuiWrapper(const std::weak_ptr<RenderPassObject>& render_pass, std::weak_ptr<Device> in_device) : device(std::move(
		in_device))
	{
		mesh = std::make_shared<Mesh>(device, sizeof(ImDrawVert), EBufferType::IMMEDIATE);

		ShaderCompiler compiler;
		const auto vertex_code = compiler.compile_raw(IMGUI_VERTEX, "main", EShaderStage::Vertex, "internal://imgui");
		const auto fragment_code = compiler.compile_raw(IMGUI_FRAGMENT, "main", EShaderStage::Fragment,
		                                                "internal://imgui");
		if (!vertex_code)
			LOG_FATAL("Failed to compile imgui vertex shader : {}", vertex_code.error());
		if (!fragment_code)
			LOG_FATAL("Failed to compile imgui fragment shader : {}", fragment_code.error());

		pipeline = std::make_shared<Pipeline>(device, render_pass, std::vector{
			                                      std::make_shared<ShaderModule>(device, vertex_code.get()),
			                                      std::make_shared<ShaderModule>(device, fragment_code.get())
		                                      }, Pipeline::CreateInfos{
			                                      .culling = ECulling::None,
			                                      .alpha_mode = EAlphaMode::Translucent,
			                                      .depth_test = false
		                                      });



        imgui_context = ImGui::CreateContext();
        IMGUI_CHECKVERSION();

        ImGuiIO& io = ImGui::GetIO();
        if (io.BackendPlatformUserData != nullptr)
            LOG_FATAL("Imgui has already been initialized !");

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        io.BackendPlatformUserData = nullptr;
        io.BackendPlatformName = "imgui_impl_internal";
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;      // We can honor GetMouseCursor() values (optional)
        io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;       // We can honor io.WantSetMousePos requests (optional, rarely used)
        io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports; // We can create multi-viewports on the Platform side (optional)
        // io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can set io.MouseHoveredViewport correctly (optional, not easy)
        /*
        io.KeyMap[ImGuiKey_Tab] = static_cast<int>(application::inputs::EButtons::Keyboard_Tab);
        io.KeyMap[ImGuiKey_LeftArrow] = static_cast<int>(application::inputs::EButtons::Keyboard_Left);
        io.KeyMap[ImGuiKey_RightArrow] = static_cast<int>(application::inputs::EButtons::Keyboard_Right);
        io.KeyMap[ImGuiKey_UpArrow] = static_cast<int>(application::inputs::EButtons::Keyboard_Up);
        io.KeyMap[ImGuiKey_DownArrow] = static_cast<int>(application::inputs::EButtons::Keyboard_Down);
        io.KeyMap[ImGuiKey_PageUp] = static_cast<int>(application::inputs::EButtons::Keyboard_PageUp);
        io.KeyMap[ImGuiKey_PageDown] = static_cast<int>(application::inputs::EButtons::Keyboard_PageDown);
        io.KeyMap[ImGuiKey_Home] = static_cast<int>(application::inputs::EButtons::Keyboard_Home);
        io.KeyMap[ImGuiKey_End] = static_cast<int>(application::inputs::EButtons::Keyboard_End);
        io.KeyMap[ImGuiKey_Insert] = static_cast<int>(application::inputs::EButtons::Keyboard_Insert);
        io.KeyMap[ImGuiKey_Delete] = static_cast<int>(application::inputs::EButtons::Keyboard_Delete);
        io.KeyMap[ImGuiKey_Backspace] = static_cast<int>(application::inputs::EButtons::Keyboard_Backspace);
        io.KeyMap[ImGuiKey_Space] = static_cast<int>(application::inputs::EButtons::Keyboard_Space);
        io.KeyMap[ImGuiKey_Enter] = static_cast<int>(application::inputs::EButtons::Keyboard_Enter);
        io.KeyMap[ImGuiKey_Escape] = static_cast<int>(application::inputs::EButtons::Keyboard_Escape);
        io.KeyMap[ImGuiKey_KeyPadEnter] = static_cast<int>(application::inputs::EButtons::Keyboard_Enter);
        io.KeyMap[ImGuiKey_A] = static_cast<int>(application::inputs::EButtons::Keyboard_A);
        io.KeyMap[ImGuiKey_C] = static_cast<int>(application::inputs::EButtons::Keyboard_C);
        io.KeyMap[ImGuiKey_V] = static_cast<int>(application::inputs::EButtons::Keyboard_V);
        io.KeyMap[ImGuiKey_X] = static_cast<int>(application::inputs::EButtons::Keyboard_X);
        io.KeyMap[ImGuiKey_Y] = static_cast<int>(application::inputs::EButtons::Keyboard_Y);
        io.KeyMap[ImGuiKey_Z] = static_cast<int>(application::inputs::EButtons::Keyboard_Z);
		
        io.SetClipboardTextFn = set_clipboard_text;
        io.GetClipboardTextFn = get_clipboard_text;
        */
        ImGuiStyle& style = ImGui::GetStyle();
        ImGui::StyleColorsDark();
        style.WindowRounding = 0;
        style.ScrollbarRounding = 0;
        style.TabRounding = 0;
        style.WindowBorderSize = 1;
        style.PopupBorderSize = 1;
        style.WindowTitleAlign.x = 0.5f;
        style.FramePadding.x = 6.f;
        style.FramePadding.y = 6.f;
        style.WindowPadding.x = 4.f;
        style.WindowPadding.y = 4.f;
        style.GrabMinSize = 16.f;
        style.ScrollbarSize = 20.f;
        style.IndentSpacing = 30.f;

        ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        main_viewport->PlatformHandle = nullptr;
        /*
        cursor_map[ImGuiMouseCursor_Arrow] = application::ECursorStyle::ARROW;
        cursor_map[ImGuiMouseCursor_TextInput] = application::ECursorStyle::I_BEAM;
        cursor_map[ImGuiMouseCursor_ResizeAll] = application::ECursorStyle::SIZE_ALL;
        cursor_map[ImGuiMouseCursor_ResizeNS] = application::ECursorStyle::SIZE_NS;
        cursor_map[ImGuiMouseCursor_ResizeEW] = application::ECursorStyle::SIZE_WE;
        cursor_map[ImGuiMouseCursor_ResizeNESW] = application::ECursorStyle::SIZE_NESW;
        cursor_map[ImGuiMouseCursor_ResizeNWSE] = application::ECursorStyle::SIZE_NWSE;
        cursor_map[ImGuiMouseCursor_Hand] = application::ECursorStyle::HAND;
        cursor_map[ImGuiMouseCursor_NotAllowed] = application::ECursorStyle::NOT_ALLOWED;
		*/
        // Create font texture
        uint8_t* pixels;
        int      width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        font_texture = std::make_shared<Image>(device, ImageParameter{
            .buffer_type = EBufferType::IMMUTABLE,
            .width = static_cast<uint32_t>(width),
            .height = static_cast<uint32_t>(height),
            .format = ColorFormat::R8G8B8A8_UNORM,
            }, BufferData(pixels, 4, width * height));

        io.Fonts->TexID = reinterpret_cast<ImTextureID>(font_texture.get());
	}

	void ImGuiWrapper::draw(const CommandBuffer& cmd)
	{
        ImGuiIO& io = ImGui::GetIO();

        // Setup display size (every frame to accommodate for start_window resizing)
        io.DisplaySize = ImVec2(static_cast<float>(context.draw_width), static_cast<float>(context.draw_height));
        io.DisplayFramebufferScale = ImVec2(1.0, 1.0);
        io.DeltaTime = static_cast<float>(application::get()->delta_time());

        // Update mouse
        io.MouseDown[0] = application::inputs::Key(application::inputs::EButtons::Mouse_Left).get_bool_value();
        io.MouseDown[1] = application::inputs::Key(application::inputs::EButtons::Mouse_Right).get_bool_value();
        io.MouseDown[2] = application::inputs::Key(application::inputs::EButtons::Mouse_Middle).get_bool_value();
        io.MouseDown[3] = application::inputs::Key(application::inputs::EButtons::Mouse_1).get_bool_value();
        io.MouseDown[4] = application::inputs::Key(application::inputs::EButtons::Mouse_2).get_bool_value();
        io.MouseHoveredViewport = 0;
        io.MousePos = ImVec2(application::inputs::Key(application::inputs::EAxis::Mouse_X).get_float_value(), application::inputs::Key(application::inputs::EAxis::Mouse_Y).get_float_value());

        const ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
        if (imgui_cursor != ImGuiMouseCursor_None)
            application::get()->set_cursor(cursor_map[imgui_cursor]);
        else
            LOG_WARNING("//@TODO : handle none cursor");

        ImGui::NewFrame();


        ImGui::EndFrame();
        ImGui::Render();
        const auto* draw_data = ImGui::GetDrawData();
        // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
        const int fb_width = static_cast<int>(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
        const int fb_height = static_cast<int>(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
        if (fb_width <= 0 || fb_height <= 0 || draw_data->TotalVtxCount == 0)
            return;

        /**
         * BUILD VERTEX BUFFERS
         */
        size_t vtx_offset = 0;
        size_t idx_offset = 0;
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            mesh->set_indexed_vertices(
                vtx_offset, 
                BufferData(cmd_list->VtxBuffer.Data, sizeof(ImDrawVert), cmd_list->VtxBuffer.Size),
                idx_offset,
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
            .scale_x = scale_x,
            .scale_y = scale_y,
            .translate_x = -1.0f - draw_data->DisplayPos.x * scale_x,
            .translate_y = 1.0f - draw_data->DisplayPos.y * scale_y,
        };
        command_buffer->push_constant(true, imgui_material_instance.get(), constant_data);

        /**
         * Draw meshs
         */

         // Will project scissor/clipping rectangles into framebuffer space
        const ImVec2 clip_off = draw_data->DisplayPos;       // (0,0) unless using multi-viewports
        const ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

        // Render command lists
        // (Because we merged all buffers into a single one, we maintain our own offset into them)
        int global_vtx_offset = 0;
        int global_idx_offset = 0;

        imgui_material_instance->bind_texture("sTexture", font_texture);
        imgui_material_instance->bind_sampler("sSampler", global_sampler);
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

                    if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
                    {
                        // Negative offsets are illegal for vkCmdSetScissor
                        if (clip_rect.x < 0.0f)
                            clip_rect.x = 0.0f;
                        if (clip_rect.y < 0.0f)
                            clip_rect.y = 0.0f;

                        // Apply scissor/clipping rectangle
                        command_buffer->set_scissor(gfx::Scissor{
                            .offset_x = static_cast<int32_t>(clip_rect.x),
                            .offset_y = static_cast<int32_t>(clip_rect.y),
                            .width = static_cast<uint32_t>(clip_rect.z - clip_rect.x),
                            .height = static_cast<uint32_t>(clip_rect.w - clip_rect.y),
                            });

                        // Bind descriptor set with font or user texture
                        if (pcmd->TextureId && false)
                            imgui_material_instance->bind_texture("test", nullptr); // TODO handle textures

                        command_buffer->draw_mesh(mesh, imgui_material_instance.get(), pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, pcmd->ElemCount);
                    }
                }
            }
            global_idx_offset += cmd_list->IdxBuffer.Size;
            global_vtx_offset += cmd_list->VtxBuffer.Size;
        }
	}
}
