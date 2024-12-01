#include <utility>

#include "gfx/ui/ImGuiWrapper.hpp"

#include "profiler.hpp"

#include <imgui.h>

#include "gfx/mesh.hpp"
#include "gfx/vulkan/command_buffer.hpp"
#include "gfx/vulkan/descriptor_sets.hpp"
#include "gfx/vulkan/image.hpp"
#include "gfx/vulkan/image_view.hpp"
#include "gfx/vulkan/pipeline.hpp"
#include "gfx/vulkan/sampler.hpp"
#include "gfx/vulkan/shader_module.hpp"
#include "gfx/window.hpp"

namespace Eng::Gfx
{

static ImGuiKey ImGui_ImplGlfw_KeyToImGuiKey(int keycode, int scancode)
{
    IM_UNUSED(scancode);
    switch (keycode)
    {
    case GLFW_KEY_TAB:
        return ImGuiKey_Tab;
    case GLFW_KEY_LEFT:
        return ImGuiKey_LeftArrow;
    case GLFW_KEY_RIGHT:
        return ImGuiKey_RightArrow;
    case GLFW_KEY_UP:
        return ImGuiKey_UpArrow;
    case GLFW_KEY_DOWN:
        return ImGuiKey_DownArrow;
    case GLFW_KEY_PAGE_UP:
        return ImGuiKey_PageUp;
    case GLFW_KEY_PAGE_DOWN:
        return ImGuiKey_PageDown;
    case GLFW_KEY_HOME:
        return ImGuiKey_Home;
    case GLFW_KEY_END:
        return ImGuiKey_End;
    case GLFW_KEY_INSERT:
        return ImGuiKey_Insert;
    case GLFW_KEY_DELETE:
        return ImGuiKey_Delete;
    case GLFW_KEY_BACKSPACE:
        return ImGuiKey_Backspace;
    case GLFW_KEY_SPACE:
        return ImGuiKey_Space;
    case GLFW_KEY_ENTER:
        return ImGuiKey_Enter;
    case GLFW_KEY_ESCAPE:
        return ImGuiKey_Escape;
    case GLFW_KEY_APOSTROPHE:
        return ImGuiKey_Apostrophe;
    case GLFW_KEY_COMMA:
        return ImGuiKey_Comma;
    case GLFW_KEY_MINUS:
        return ImGuiKey_Minus;
    case GLFW_KEY_PERIOD:
        return ImGuiKey_Period;
    case GLFW_KEY_SLASH:
        return ImGuiKey_Slash;
    case GLFW_KEY_SEMICOLON:
        return ImGuiKey_Semicolon;
    case GLFW_KEY_EQUAL:
        return ImGuiKey_Equal;
    case GLFW_KEY_LEFT_BRACKET:
        return ImGuiKey_LeftBracket;
    case GLFW_KEY_BACKSLASH:
        return ImGuiKey_Backslash;
    case GLFW_KEY_RIGHT_BRACKET:
        return ImGuiKey_RightBracket;
    case GLFW_KEY_GRAVE_ACCENT:
        return ImGuiKey_GraveAccent;
    case GLFW_KEY_CAPS_LOCK:
        return ImGuiKey_CapsLock;
    case GLFW_KEY_SCROLL_LOCK:
        return ImGuiKey_ScrollLock;
    case GLFW_KEY_NUM_LOCK:
        return ImGuiKey_NumLock;
    case GLFW_KEY_PRINT_SCREEN:
        return ImGuiKey_PrintScreen;
    case GLFW_KEY_PAUSE:
        return ImGuiKey_Pause;
    case GLFW_KEY_KP_0:
        return ImGuiKey_Keypad0;
    case GLFW_KEY_KP_1:
        return ImGuiKey_Keypad1;
    case GLFW_KEY_KP_2:
        return ImGuiKey_Keypad2;
    case GLFW_KEY_KP_3:
        return ImGuiKey_Keypad3;
    case GLFW_KEY_KP_4:
        return ImGuiKey_Keypad4;
    case GLFW_KEY_KP_5:
        return ImGuiKey_Keypad5;
    case GLFW_KEY_KP_6:
        return ImGuiKey_Keypad6;
    case GLFW_KEY_KP_7:
        return ImGuiKey_Keypad7;
    case GLFW_KEY_KP_8:
        return ImGuiKey_Keypad8;
    case GLFW_KEY_KP_9:
        return ImGuiKey_Keypad9;
    case GLFW_KEY_KP_DECIMAL:
        return ImGuiKey_KeypadDecimal;
    case GLFW_KEY_KP_DIVIDE:
        return ImGuiKey_KeypadDivide;
    case GLFW_KEY_KP_MULTIPLY:
        return ImGuiKey_KeypadMultiply;
    case GLFW_KEY_KP_SUBTRACT:
        return ImGuiKey_KeypadSubtract;
    case GLFW_KEY_KP_ADD:
        return ImGuiKey_KeypadAdd;
    case GLFW_KEY_KP_ENTER:
        return ImGuiKey_KeypadEnter;
    case GLFW_KEY_KP_EQUAL:
        return ImGuiKey_KeypadEqual;
    case GLFW_KEY_LEFT_SHIFT:
        return ImGuiKey_LeftShift;
    case GLFW_KEY_LEFT_CONTROL:
        return ImGuiKey_LeftCtrl;
    case GLFW_KEY_LEFT_ALT:
        return ImGuiKey_LeftAlt;
    case GLFW_KEY_LEFT_SUPER:
        return ImGuiKey_LeftSuper;
    case GLFW_KEY_RIGHT_SHIFT:
        return ImGuiKey_RightShift;
    case GLFW_KEY_RIGHT_CONTROL:
        return ImGuiKey_RightCtrl;
    case GLFW_KEY_RIGHT_ALT:
        return ImGuiKey_RightAlt;
    case GLFW_KEY_RIGHT_SUPER:
        return ImGuiKey_RightSuper;
    case GLFW_KEY_MENU:
        return ImGuiKey_Menu;
    case GLFW_KEY_0:
        return ImGuiKey_0;
    case GLFW_KEY_1:
        return ImGuiKey_1;
    case GLFW_KEY_2:
        return ImGuiKey_2;
    case GLFW_KEY_3:
        return ImGuiKey_3;
    case GLFW_KEY_4:
        return ImGuiKey_4;
    case GLFW_KEY_5:
        return ImGuiKey_5;
    case GLFW_KEY_6:
        return ImGuiKey_6;
    case GLFW_KEY_7:
        return ImGuiKey_7;
    case GLFW_KEY_8:
        return ImGuiKey_8;
    case GLFW_KEY_9:
        return ImGuiKey_9;
    case GLFW_KEY_A:
        return ImGuiKey_A;
    case GLFW_KEY_B:
        return ImGuiKey_B;
    case GLFW_KEY_C:
        return ImGuiKey_C;
    case GLFW_KEY_D:
        return ImGuiKey_D;
    case GLFW_KEY_E:
        return ImGuiKey_E;
    case GLFW_KEY_F:
        return ImGuiKey_F;
    case GLFW_KEY_G:
        return ImGuiKey_G;
    case GLFW_KEY_H:
        return ImGuiKey_H;
    case GLFW_KEY_I:
        return ImGuiKey_I;
    case GLFW_KEY_J:
        return ImGuiKey_J;
    case GLFW_KEY_K:
        return ImGuiKey_K;
    case GLFW_KEY_L:
        return ImGuiKey_L;
    case GLFW_KEY_M:
        return ImGuiKey_M;
    case GLFW_KEY_N:
        return ImGuiKey_N;
    case GLFW_KEY_O:
        return ImGuiKey_O;
    case GLFW_KEY_P:
        return ImGuiKey_P;
    case GLFW_KEY_Q:
        return ImGuiKey_Q;
    case GLFW_KEY_R:
        return ImGuiKey_R;
    case GLFW_KEY_S:
        return ImGuiKey_S;
    case GLFW_KEY_T:
        return ImGuiKey_T;
    case GLFW_KEY_U:
        return ImGuiKey_U;
    case GLFW_KEY_V:
        return ImGuiKey_V;
    case GLFW_KEY_W:
        return ImGuiKey_W;
    case GLFW_KEY_X:
        return ImGuiKey_X;
    case GLFW_KEY_Y:
        return ImGuiKey_Y;
    case GLFW_KEY_Z:
        return ImGuiKey_Z;
    case GLFW_KEY_F1:
        return ImGuiKey_F1;
    case GLFW_KEY_F2:
        return ImGuiKey_F2;
    case GLFW_KEY_F3:
        return ImGuiKey_F3;
    case GLFW_KEY_F4:
        return ImGuiKey_F4;
    case GLFW_KEY_F5:
        return ImGuiKey_F5;
    case GLFW_KEY_F6:
        return ImGuiKey_F6;
    case GLFW_KEY_F7:
        return ImGuiKey_F7;
    case GLFW_KEY_F8:
        return ImGuiKey_F8;
    case GLFW_KEY_F9:
        return ImGuiKey_F9;
    case GLFW_KEY_F10:
        return ImGuiKey_F10;
    case GLFW_KEY_F11:
        return ImGuiKey_F11;
    case GLFW_KEY_F12:
        return ImGuiKey_F12;
    case GLFW_KEY_F13:
        return ImGuiKey_F13;
    case GLFW_KEY_F14:
        return ImGuiKey_F14;
    case GLFW_KEY_F15:
        return ImGuiKey_F15;
    case GLFW_KEY_F16:
        return ImGuiKey_F16;
    case GLFW_KEY_F17:
        return ImGuiKey_F17;
    case GLFW_KEY_F18:
        return ImGuiKey_F18;
    case GLFW_KEY_F19:
        return ImGuiKey_F19;
    case GLFW_KEY_F20:
        return ImGuiKey_F20;
    case GLFW_KEY_F21:
        return ImGuiKey_F21;
    case GLFW_KEY_F22:
        return ImGuiKey_F22;
    case GLFW_KEY_F23:
        return ImGuiKey_F23;
    case GLFW_KEY_F24:
        return ImGuiKey_F24;
    default:
        return ImGuiKey_None;
    }
}

void ImGuiWrapper::mouse_button_callback(int button, int action, int)
{
    ImGui::SetCurrentContext(imgui_context);
    update_key_modifiers();
    ImGuiIO& io = ImGui::GetIO();
    if (button >= 0 && button < ImGuiMouseButton_COUNT)
        io.AddMouseButtonEvent(button, action == GLFW_PRESS);
}

void ImGuiWrapper::scroll_callback(double xoffset, double yoffset)
{
    ImGui::SetCurrentContext(imgui_context);
    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseWheelEvent(static_cast<float>(xoffset), static_cast<float>(yoffset));
}

void ImGuiWrapper::windows_focus_callback(int focused) const
{
    ImGui::SetCurrentContext(imgui_context);
    ImGuiIO& io = ImGui::GetIO();
    io.AddFocusEvent(focused != 0);
}

void ImGuiWrapper::cursor_pos_callback(double x, double y)
{
    ImGui::SetCurrentContext(imgui_context);
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(static_cast<float>(x), static_cast<float>(y));
    LastValidMousePos = ImVec2(static_cast<float>(x), static_cast<float>(y));
}

void ImGuiWrapper::update_key_modifiers() const
{
    ImGui::SetCurrentContext(imgui_context);
    ImGuiIO& io = ImGui::GetIO();
    if (auto win = target_window.lock())
    {
        io.AddKeyEvent(ImGuiMod_Ctrl, (glfwGetKey(win->raw(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) || (glfwGetKey(win->raw(), GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS));
        io.AddKeyEvent(ImGuiMod_Shift, (glfwGetKey(win->raw(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) || (glfwGetKey(win->raw(), GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS));
        io.AddKeyEvent(ImGuiMod_Alt, (glfwGetKey(win->raw(), GLFW_KEY_LEFT_ALT) == GLFW_PRESS) || (glfwGetKey(win->raw(), GLFW_KEY_RIGHT_ALT) == GLFW_PRESS));
        io.AddKeyEvent(ImGuiMod_Super, (glfwGetKey(win->raw(), GLFW_KEY_LEFT_SUPER) == GLFW_PRESS) || (glfwGetKey(win->raw(), GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS));
    }
}

void ImGuiWrapper::cursor_enter_callback(int entered)
{
    ImGui::SetCurrentContext(imgui_context);
    ImGuiIO& io = ImGui::GetIO();
    if (entered)
    {
        io.AddMousePosEvent(LastValidMousePos.x, LastValidMousePos.y);
    }
    else if (!entered)
    {
        LastValidMousePos = io.MousePos;
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    }
}

void ImGuiWrapper::char_callback(unsigned int c)
{
    ImGui::SetCurrentContext(imgui_context);
    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharacter(c);
}

void ImGuiWrapper::key_callback(int keycode, int scancode, int action, int)
{
    if (action != GLFW_PRESS && action != GLFW_RELEASE)
        return;

    update_key_modifiers();

    ImGuiIO& io        = ImGui::GetIO();
    ImGuiKey imgui_key = ImGui_ImplGlfw_KeyToImGuiKey(keycode, scancode);
    io.AddKeyEvent(imgui_key, (action == GLFW_PRESS));
    io.SetKeyEventNativeData(imgui_key, keycode, scancode); // To support legacy indexing (<1.87 user code)
}

void ImGuiWrapper::update_mouse_cursor() const
{
    ImGuiIO& io = ImGui::GetIO();
    if (auto window = target_window.lock())
    {
        if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) || glfwGetInputMode(window->raw(), GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
            return;

        ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
        // (those braces are here to reduce diff with multi-viewports support in 'docking' branch)
        {
            if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
            {
                // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
                glfwSetInputMode(window->raw(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            }
            else
            {
                // Show OS mouse cursor
                // FIXME-PLATFORM: Unfocused windows seems to fail changing the mouse cursor with GLFW 3.2, but 3.3 works here.
                glfwSetCursor(window->raw(), cursor_map[imgui_cursor] ? cursor_map[imgui_cursor] : cursor_map[ImGuiMouseCursor_Arrow]);
                glfwSetInputMode(window->raw(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }
    }
}

void ImGuiWrapper::update_mouse_data() const
{
    ImGuiIO& io = ImGui::GetIO();

    // (those braces are here to reduce diff with multi-viewports support in 'docking' branch)
    if (auto window = target_window.lock())
    {
        const bool is_window_focused = glfwGetWindowAttrib(window->raw(), GLFW_FOCUSED) != 0;
        if (is_window_focused)
        {
            // (Optional) Set OS mouse position from Dear ImGui if requested (rarely used, only when io.ConfigNavMoveSetMousePos is enabled by user)
            if (io.WantSetMousePos)
                glfwSetCursorPos(window->raw(), (double)io.MousePos.x, (double)io.MousePos.y);
        }
    }
}

static inline float Saturate(float v)
{
    return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v;
}
void ImGuiWrapper::update_gamepads()
{
    ImGuiIO& io = ImGui::GetIO();
    if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0) // FIXME: Technically feeding gamepad shouldn't depend on this now that they are regular inputs.
        return;

    io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
#if GLFW_HAS_GAMEPAD_API
    GLFWgamepadstate gamepad;
    if (!glfwGetGamepadState(GLFW_JOYSTICK_1, &gamepad))
        return;
#define MAP_BUTTON(KEY_NO, BUTTON_NO, _UNUSED)                   \
    do                                                           \
    {                                                            \
        io.AddKeyEvent(KEY_NO, gamepad.buttons[BUTTON_NO] != 0); \
    } while (0)
#define MAP_ANALOG(KEY_NO, AXIS_NO, _UNUSED, V0, V1)          \
    do                                                        \
    {                                                         \
        float v = gamepad.axes[AXIS_NO];                      \
        v       = (v - V0) / (V1 - V0);                       \
        io.AddKeyAnalogEvent(KEY_NO, v > 0.10f, Saturate(v)); \
    } while (0)
#else
    int                  axes_count = 0, buttons_count = 0;
    const float*         axes       = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &axes_count);
    const unsigned char* buttons    = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &buttons_count);
    if (axes_count == 0 || buttons_count == 0)
        return;
#define MAP_BUTTON(KEY_NO, _UNUSED, BUTTON_NO)                                                   \
    do                                                                                           \
    {                                                                                            \
        io.AddKeyEvent(KEY_NO, (buttons_count > BUTTON_NO && buttons[BUTTON_NO] == GLFW_PRESS)); \
    } while (0)
#define MAP_ANALOG(KEY_NO, _UNUSED, AXIS_NO, V0, V1)           \
    do                                                         \
    {                                                          \
        float v = (axes_count > AXIS_NO) ? axes[AXIS_NO] : V0; \
        v       = (v - V0) / (V1 - V0);                        \
        io.AddKeyAnalogEvent(KEY_NO, v > 0.10f, Saturate(v));  \
    } while (0)
#endif
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    MAP_BUTTON(ImGuiKey_GamepadStart, GLFW_GAMEPAD_BUTTON_START, 7);
    MAP_BUTTON(ImGuiKey_GamepadBack, GLFW_GAMEPAD_BUTTON_BACK, 6);
    MAP_BUTTON(ImGuiKey_GamepadFaceLeft, GLFW_GAMEPAD_BUTTON_X, 2);  // Xbox X, PS Square
    MAP_BUTTON(ImGuiKey_GamepadFaceRight, GLFW_GAMEPAD_BUTTON_B, 1); // Xbox B, PS Circle
    MAP_BUTTON(ImGuiKey_GamepadFaceUp, GLFW_GAMEPAD_BUTTON_Y, 3);    // Xbox Y, PS Triangle
    MAP_BUTTON(ImGuiKey_GamepadFaceDown, GLFW_GAMEPAD_BUTTON_A, 0);  // Xbox A, PS Cross
    MAP_BUTTON(ImGuiKey_GamepadDpadLeft, GLFW_GAMEPAD_BUTTON_DPAD_LEFT, 13);
    MAP_BUTTON(ImGuiKey_GamepadDpadRight, GLFW_GAMEPAD_BUTTON_DPAD_RIGHT, 11);
    MAP_BUTTON(ImGuiKey_GamepadDpadUp, GLFW_GAMEPAD_BUTTON_DPAD_UP, 10);
    MAP_BUTTON(ImGuiKey_GamepadDpadDown, GLFW_GAMEPAD_BUTTON_DPAD_DOWN, 12);
    MAP_BUTTON(ImGuiKey_GamepadL1, GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, 4);
    MAP_BUTTON(ImGuiKey_GamepadR1, GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, 5);
    MAP_ANALOG(ImGuiKey_GamepadL2, GLFW_GAMEPAD_AXIS_LEFT_TRIGGER, 4, -0.75f, +1.0f);
    MAP_ANALOG(ImGuiKey_GamepadR2, GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER, 5, -0.75f, +1.0f);
    MAP_BUTTON(ImGuiKey_GamepadL3, GLFW_GAMEPAD_BUTTON_LEFT_THUMB, 8);
    MAP_BUTTON(ImGuiKey_GamepadR3, GLFW_GAMEPAD_BUTTON_RIGHT_THUMB, 9);
    MAP_ANALOG(ImGuiKey_GamepadLStickLeft, GLFW_GAMEPAD_AXIS_LEFT_X, 0, -0.25f, -1.0f);
    MAP_ANALOG(ImGuiKey_GamepadLStickRight, GLFW_GAMEPAD_AXIS_LEFT_X, 0, +0.25f, +1.0f);
    MAP_ANALOG(ImGuiKey_GamepadLStickUp, GLFW_GAMEPAD_AXIS_LEFT_Y, 1, -0.25f, -1.0f);
    MAP_ANALOG(ImGuiKey_GamepadLStickDown, GLFW_GAMEPAD_AXIS_LEFT_Y, 1, +0.25f, +1.0f);
    MAP_ANALOG(ImGuiKey_GamepadRStickLeft, GLFW_GAMEPAD_AXIS_RIGHT_X, 2, -0.25f, -1.0f);
    MAP_ANALOG(ImGuiKey_GamepadRStickRight, GLFW_GAMEPAD_AXIS_RIGHT_X, 2, +0.25f, +1.0f);
    MAP_ANALOG(ImGuiKey_GamepadRStickUp, GLFW_GAMEPAD_AXIS_RIGHT_Y, 3, -0.25f, -1.0f);
    MAP_ANALOG(ImGuiKey_GamepadRStickDown, GLFW_GAMEPAD_AXIS_RIGHT_Y, 3, +0.25f, +1.0f);
#undef MAP_BUTTON
#undef MAP_ANALOG
}


ImGuiWrapper::ImGuiWrapper(std::string in_name, const std::string& render_pass, std::weak_ptr<Device> in_device, std::weak_ptr<Window> in_target_window)
    : device(std::move(in_device)), target_window(std::move(in_target_window)), name(std::move(in_name))
{
    last_time = std::chrono::steady_clock::now();

    target_window.lock()->on_input_char.add_object(this, &ImGuiWrapper::char_callback);
    target_window.lock()->on_scroll.add_object(this, &ImGuiWrapper::scroll_callback);
    target_window.lock()->on_change_cursor_pos.add_object(this, &ImGuiWrapper::cursor_pos_callback);
    target_window.lock()->on_input_key.add_object(this, &ImGuiWrapper::key_callback);
    target_window.lock()->on_mouse_button.add_object(this, &ImGuiWrapper::mouse_button_callback);

    //bd->PrevUserCallbackWindowFocus = glfwSetWindowFocusCallback(window, ImGui_ImplGlfw_WindowFocusCallback);
    //bd->PrevUserCallbackCursorEnter = glfwSetCursorEnterCallback(window, ImGui_ImplGlfw_CursorEnterCallback);
    //bd->PrevUserCallbackMonitor     = glfwSetMonitorCallback(ImGui_ImplGlfw_MonitorCallback);

    mesh = Mesh::create(name + "_mesh", device, sizeof(ImDrawVert), EBufferType::IMMEDIATE);

    image_sampler = Sampler::create(name + "_generic_sampler", device, Sampler::CreateInfos{});

    auto compilation_result = ShaderCompiler::Compiler::get().create_session("internal/imgui")->compile(render_pass, PermutationDescription{});
    if (!compilation_result.errors.empty())
    {
        for (const auto& error : compilation_result.errors)
            LOG_ERROR("Imgui shader compilation failed : {}", error.message);
    }

    if (compilation_result.stages.empty())
        LOG_ERROR("Imgui material is not compatible with render pass : {}", render_pass);

    std::vector<std::shared_ptr<ShaderModule>> modules;
    for (const auto& stage : compilation_result.stages | std::views::values)
        modules.emplace_back(ShaderModule::create(device, stage));

    auto rp = device.lock()->get_render_pass(render_pass);
    if (!rp.lock())
    {
        LOG_ERROR("There is no render pass named {}", render_pass);
        return;
    }

    imgui_material = Pipeline::create(name + "_pipeline", device, rp, modules,
                                      Pipeline::CreateInfos{.options{
                                                                .culling = ECulling::None,
                                                                .alpha = EAlphaMode::Translucent,
                                                                .depth_test = false,
                                                            },
                                                            .vertex_inputs = std::vector{
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
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;

    ImGuiPlatformIO& platform_io            = ImGui::GetPlatformIO();
    platform_io.Platform_SetClipboardTextFn = [](ImGuiContext*, const char* text)
    {
        glfwSetClipboardString(nullptr, text);
    };
    platform_io.Platform_GetClipboardTextFn = [](ImGuiContext*)
    {
        return glfwGetClipboardString(nullptr);
    };

    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark();

    style.Colors[ImGuiCol_FrameBg]            = ImVec4(.18f, .18f, .18f, 1);
    style.Colors[ImGuiCol_CheckMark]          = ImVec4(.62f, .62f, .62f, 1);
    style.Colors[ImGuiCol_SliderGrab]         = ImVec4(.62f, .62f, .62f, 1);
    style.Colors[ImGuiCol_Button]             = ImVec4(.27f, .27f, .27f, 1);
    style.Colors[ImGuiCol_Header]             = ImVec4(.28f, .28f, .28f, 1);
    style.Colors[ImGuiCol_Tab]                = ImVec4(.08f, .08f, .08f, .86f);
    style.Colors[ImGuiCol_TabUnfocused]       = ImVec4(.1f, .1f, .1f, .86f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(.2f, .2f, .2f, 1);
    style.Colors[ImGuiCol_DockingEmptyBg]     = ImVec4(.07f, .07f, .07f, 1);
    style.Colors[ImGuiCol_TitleBgActive]      = ImVec4(.11f, .11f, .11f, 1);
    style.Colors[ImGuiCol_WindowBg]           = ImVec4(.06f, .06f, .06f, 1);
    style.Colors[ImGuiCol_TabActive]          = ImVec4(.52f, .52f, .52f, 1);

    style.WindowRounding     = 0;
    style.ScrollbarRounding  = 0;
    style.TabRounding        = 0;
    style.WindowBorderSize   = 1;
    style.PopupBorderSize    = 1;
    style.WindowTitleAlign.x = 0.5f;
    style.FramePadding.x     = 12.f;
    style.FramePadding.y     = 10.f;
    style.WindowPadding.x    = 4.f;
    style.WindowPadding.y    = 4.f;
    style.GrabMinSize        = 12.f;
    style.ScrollbarSize      = 16.f;
    style.IndentSpacing      = 17.f;

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
    if (std::filesystem::exists("resources/fonts/Roboto-Medium.ttf"))
        io.Fonts->AddFontFromFileTTF("resources/fonts/Roboto-Medium.ttf", 16.f);
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    font_texture = Image::create(name + "_font_image", device,
                                 ImageParameter{
                                     .format = ColorFormat::R8G8B8A8_UNORM,
                                     .buffer_type = EBufferType::IMMUTABLE,
                                     .width = static_cast<uint32_t>(width),
                                     .height = static_cast<uint32_t>(height),
                                 },
                                 BufferData(pixels, 4, width * height));
    font_texture_view     = ImageView::create(name + "_font_image_view", font_texture);
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

void ImGuiWrapper::begin(glm::uvec2 draw_res)
{
    if (!imgui_material)
        return;
    ImGui::SetCurrentContext(imgui_context);
    ImGuiIO& io = ImGui::GetIO();
    // Setup display size (every frame to accommodate for start_window resizing)
    io.DisplaySize             = ImVec2(static_cast<float>(draw_res.x), static_cast<float>(draw_res.y));
    io.DisplayFramebufferScale = ImVec2(1.0, 1.0);
    auto new_time              = std::chrono::steady_clock::now();
    io.DeltaTime               = static_cast<float>(std::chrono::duration_cast<std::chrono::nanoseconds>(new_time - last_time).count()) / 1000000000.0f;
    last_time                  = new_time;

    update_mouse_data();
    update_mouse_cursor();
    update_gamepads();

    const ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (imgui_cursor != ImGuiMouseCursor_None)
        glfwSetCursor(target_window.lock()->raw(), cursor_map[imgui_cursor]);

    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(-4, -4));
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(draw_res.x) + 8.f, static_cast<float>(draw_res.y) + 8.f));
    if (ImGui::Begin("BackgroundHUD", nullptr, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus))
    {
        ImGui::DockSpace(ImGui::GetID("Master dockSpace"), ImVec2(0.f, 0.f), ImGuiDockNodeFlags_PassthruCentralNode);
    }
    ImGui::End();
}

void ImGuiWrapper::prepare_all_window()
{
    if (!imgui_material)
        return;
    PROFILER_SCOPE(ImGuiPrepareWindows);
    ImGui::SetCurrentContext(imgui_context);

    for (int64_t i = windows.size() - 1; i >= 0; --i)
    {
        if (windows[i]->b_open)
            windows[i]->draw_internal(*this);
        else
            windows.erase(windows.begin() + i);
    }
}

void ImGuiWrapper::end(CommandBuffer& cmd)
{
    if (!imgui_material)
        return;
    ImGui::SetCurrentContext(imgui_context);

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
            .scale_x = scale_x,
            .scale_y = scale_y,
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

    cmd.bind_pipeline(imgui_material);

    bool used_other_image = true;

    ankerl::unordered_dense::set<ImTextureID> unused_image;
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
                        .width = static_cast<uint32_t>(clip_rect.z - clip_rect.x),
                        .height = static_cast<uint32_t>(clip_rect.w - clip_rect.y),
                    });

                    // Bind descriptor set with font or user texture
                    if (pcmd->TextureId)
                    {
                        used_other_image = true;
                        if (const auto found_image_view = per_image_ids.find(pcmd->TextureId); found_image_view != per_image_ids.end())
                        {
                            unused_image.erase(pcmd->TextureId);
                            if (const auto found_descriptors = per_image_descriptor.find(found_image_view->second); found_descriptors != per_image_descriptor.end())
                            {
                                if (!found_descriptors->second.second)
                                {
                                    const auto descriptors = DescriptorSet::create(name + "_descriptor:" + found_image_view->second->get_name(), device, imgui_material, true);
                                    descriptors->bind_image("sTexture", found_image_view->second);
                                    descriptors->bind_sampler("sSampler", image_sampler);
                                    found_descriptors->second.second = descriptors;
                                }
                                cmd.bind_descriptors(*found_descriptors->second.second, *imgui_material);
                            }
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
    if (!imgui_material)
        return 0;
    if (!image_view)
        return 0;
    auto found_descriptor = per_image_descriptor.find(image_view);
    if (found_descriptor == per_image_descriptor.end())
    {
        assert(max_texture_id != UINT64_MAX);
        ImTextureID new_id = ++max_texture_id;
        per_image_descriptor.emplace(image_view, std::pair{new_id, nullptr});
        per_image_ids.emplace(new_id, image_view);
        return new_id;
    }
    return found_descriptor->second.first;
}
} // namespace Eng::Gfx