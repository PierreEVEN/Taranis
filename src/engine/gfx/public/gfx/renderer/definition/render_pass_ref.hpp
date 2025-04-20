#pragma once

#include "render_pass_id.hpp"
#include <format>

namespace Eng::Gfx
{

class RenderPassTestClass
{

    friend class Renderer;
    friend class RenderNode;

    RenderPassTestClass(RenderPassGenericId generic_name, RenderPassUid unique_id) : name(std::move(generic_name)), id(unique_id)
    {
    }

    RenderPassGenericId name;
    RenderPassUid       id = 0;

  public:
    RenderPassTestClass() = default;

    const RenderPassGenericId& generic_id() const
    {
        return name;
    }

    RenderPassUid unique_id() const
    {
        return id;
    }

    std::string to_string() const
    {
        return "<" + name + ":" + std::to_string(id) + ">";
    }

    bool operator==(const RenderPassTestClass& other) const
    {
        return id == other.id;
    }

    friend std::ostream& operator<<(std::ostream& stream, const RenderPassTestClass& ref)
    {
        return stream << ref.to_string();
    }
};

class RenderPassRef
{
    friend class Renderer;
    friend class RenderNode;

    RenderPassRef(RenderPassGenericId generic_name, RenderPassUid unique_id) : name(std::move(generic_name)), id(unique_id)
    {
    }

    RenderPassGenericId name;
    RenderPassUid       id = 0;

public:
    RenderPassRef() = default;

    const RenderPassGenericId& generic_id() const
    {
        return name;
    }

    RenderPassUid unique_id() const
    {
        return id;
    }

    std::string to_string() const
    {
        return "<" + name + ":" + std::to_string(id) + ">";
    }

    bool operator==(const RenderPassRef& other) const
    {
        return id == other.id;
    }

    friend std::ostream& operator<<(std::ostream& stream, const RenderPassRef& ref)
    {
        return stream << ref.to_string();
    }
};
} // namespace Eng::Gfx

template <> struct std::hash<Eng::Gfx::RenderPassTestClass>
{
    size_t operator()(const Eng::Gfx::RenderPassTestClass& ctx) const noexcept
    {
        return std::hash<std::string>()(ctx.generic_id());
    }
};

template <> struct std::hash<Eng::Gfx::RenderPassRef>
{
    size_t operator()(const Eng::Gfx::RenderPassRef& ctx) const noexcept
    {
        return std::hash<std::string>()(ctx.generic_id());
    }
};


template <> struct std::formatter<Eng::Gfx::RenderPassRef>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const Eng::Gfx::RenderPassRef& obj, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "{}", obj.to_string());
    }
};
