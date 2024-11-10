#pragma once
#include <string>

namespace Eng
{
namespace Gfx
{
class ImGuiWrapper;
}

class UiWindow
{
public:
    virtual ~UiWindow() = default;

private:
    friend class Gfx::ImGuiWrapper;

    void close()
    {
        b_open = false;
    }

protected:
    UiWindow(std::string name);

    virtual void draw(Gfx::ImGuiWrapper& ctx) = 0;
    bool         b_enable_menu_bar            = false;

private:
    virtual void draw_internal(Gfx::ImGuiWrapper& ctx);
    bool         b_open = true;
    std::string  name;
};
}