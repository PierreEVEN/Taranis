#include "cmaa.hpp"

#include "engine.hpp"
#include "object_ptr.hpp"
#include "assets/material_asset.hpp"
#include "gfx/renderer/definition/renderer.hpp"
#include "gfx/vulkan/compute_pipeline.hpp"
#include "gfx/vulkan/shader_module.hpp"
#include "shader_compiler/shader_compiler.hpp"


class CmaaRenderPass : public Eng::Gfx::IRenderPass
{
public:
    CmaaRenderPass()
    {
    }

    void init(const Eng::Gfx::RenderPassInstance& rp) override
    {
        auto session                  = ShaderCompiler::Compiler::get().create_session("cmaa2");
        auto cr_cmaa2_edges_color_2x2 = session->compile("cmaa2_edges_color_2x2", session->get_default_permutations_description());
        if (!cr_cmaa2_edges_color_2x2.errors.empty())
        {
            for (const auto err : cr_cmaa2_edges_color_2x2.errors)
                LOG_ERROR("Failed to compile cmaa2_edges_color_2x2 shader : {}:{} : {}", err.line + 1, err.column, err.message);
            return;
        }
        LOG_INFO("Compiled cmaa2_edges_color_2x2");
        std::shared_ptr<Eng::Gfx::ShaderModule> sm_cmaa2_edges_color_2x2 = Eng::Gfx::ShaderModule::create(Eng::Engine::get().get_device(), cr_cmaa2_edges_color_2x2.stages.find(Eng::Gfx::EShaderStage::Compute)->second);
        LOG_INFO("Created cmaa2_edges_color_2x2 module");
        std::shared_ptr<Eng::Gfx::ComputePipeline> pipeline_cmaa2_edges_color_2x2 = Eng::Gfx::ComputePipeline::create("cmaa2_edges_color_2x2", Eng::Engine::get().get_device(), sm_cmaa2_edges_color_2x2);
        LOG_INFO("Created cmaa2_edges_color_2x2 pipeline");

        auto cr_cmaa2_compute_dispatch_args = session->compile("cmaa2_compute_dispatch_args", session->get_default_permutations_description());
        if (!cr_cmaa2_compute_dispatch_args.errors.empty())
        {
            for (const auto err : cr_cmaa2_compute_dispatch_args.errors)
                LOG_ERROR("Failed to compile cmaa2_compute_dispatch_args shader : {}:{} : {}", err.line + 1, err.column, err.message);
            return;
        }
        LOG_INFO("Compiled cmaa2_compute_dispatch_args");
        std::shared_ptr<Eng::Gfx::ShaderModule> sm_cmaa2_compute_dispatch_args =
            Eng::Gfx::ShaderModule::create(Eng::Engine::get().get_device(), cr_cmaa2_compute_dispatch_args.stages.find(Eng::Gfx::EShaderStage::Compute)->second);
        LOG_INFO("Created cmaa2_compute_dispatch_args module");
        std::shared_ptr<Eng::Gfx::ComputePipeline> pipeline_cmaa2_compute_dispatch_args = Eng::Gfx::ComputePipeline::create("cmaa2_compute_dispatch_args", Eng::Engine::get().get_device(), sm_cmaa2_compute_dispatch_args);
        LOG_INFO("Created cmaa2_compute_dispatch_args pipeline");

        auto cr_cmaa2_process_candidates = session->compile("cmaa2_process_candidates", session->get_default_permutations_description());
        if (!cr_cmaa2_process_candidates.errors.empty())
        {
            for (const auto err : cr_cmaa2_process_candidates.errors)
                LOG_ERROR("Failed to compile cmaa2_process_candidates shader : {}:{} : {}", err.line + 1, err.column, err.message);
            return;
        }
        LOG_INFO("Compiled cmaa2_process_candidates");

        auto cr_cmaa2_deferred_color_apply2x2 = session->compile("cmaa2_deferred_color_apply2x2", session->get_default_permutations_description());
        if (!cr_cmaa2_deferred_color_apply2x2.errors.empty())
        {
            for (const auto err : cr_cmaa2_deferred_color_apply2x2.errors)
                LOG_ERROR("Failed to compile cmaa2_deferred_color_apply2x2 shader : {}:{} : {}", err.line + 1, err.column, err.message);
            return;
        }
        LOG_INFO("Compiled cmaa2_deferred_color_apply2x2");

        auto cr_cmaa2_debug_draw_edges = session->compile("cmaa2_debug_draw_edges", session->get_default_permutations_description());
        if (!cr_cmaa2_debug_draw_edges.errors.empty())
        {
            for (const auto err : cr_cmaa2_debug_draw_edges.errors)
                LOG_ERROR("Failed to compile cmaa2_debug_draw_edges shader : {}:{} : {}", err.line + 1, err.column, err.message);
            return;
        }
        LOG_INFO("Compiled cmaa2_debug_draw_edges");

    }

    void pre_draw(const Eng::Gfx::RenderPassInstance&) override
    {

    }

    void draw(const Eng::Gfx::RenderPassInstance&, Eng::Gfx::CommandBuffer&, size_t) override
    {

    }

    void pre_submit(const Eng::Gfx::RenderPassInstance&) override
    {

    }
};


void Cmaa2::append_to_renderer(Eng::Gfx::Renderer& renderer)
{
    renderer["cmaa2"]
        .require("gbuffer_resolve")
        .require("gbuffers")
        .render_pass<CmaaRenderPass>()
        [Eng::Gfx::Attachment::slot("target").format(Eng::Gfx::ColorFormat::R8G8B8A8_UNORM)];
}