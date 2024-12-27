#include "cmaa.hpp"

#include "engine.hpp"
#include "object_ptr.hpp"
#include "assets/material_asset.hpp"
#include "gfx/renderer/definition/renderer.hpp"
#include "gfx/vulkan/command_buffer.hpp"
#include "gfx/vulkan/compute_pipeline.hpp"
#include "gfx/vulkan/descriptor_sets.hpp"
#include "gfx/vulkan/shader_module.hpp"
#include "shader_compiler/shader_compiler.hpp"


class CmaaRenderPass : public Eng::Gfx::IRenderPass
{
public:
    CmaaRenderPass()
    {
    }

    void init(const Eng::Gfx::RenderPassInstanceBase&) override
    {
        auto device                   = Eng::Engine::get().get_device();
        auto session                  = ShaderCompiler::Compiler::get().create_session("cmaa2");
        auto cr_cmaa2_edges_color_2x2 = session->compile("cmaa2_edges_color_2x2", session->get_default_permutations_description());
        if (!cr_cmaa2_edges_color_2x2.errors.empty())
        {
            for (const auto err : cr_cmaa2_edges_color_2x2.errors)
                LOG_ERROR("Failed to compile cmaa2_edges_color_2x2 shader : {}:{} : {}", err.line + 1, err.column, err.message);
            return;
        }
        std::shared_ptr<Eng::Gfx::ShaderModule> sm_cmaa2_edges_color_2x2 = Eng::Gfx::ShaderModule::create(device, cr_cmaa2_edges_color_2x2.stages.find(Eng::Gfx::EShaderStage::Compute)->second);
        pipeline_cmaa2_edges_color_2x2                                   = Eng::Gfx::ComputePipeline::create("cmaa2_edges_color_2x2", device, sm_cmaa2_edges_color_2x2);
        descriptors_cmaa2_edges_color_2x2                                = Eng::Gfx::DescriptorSet::create("cmaa2_edges_color_2x2", device, pipeline_cmaa2_edges_color_2x2->get_layout());

        auto cr_cmaa2_compute_dispatch_args = session->compile("cmaa2_compute_dispatch_args", session->get_default_permutations_description());
        if (!cr_cmaa2_compute_dispatch_args.errors.empty())
        {
            for (const auto err : cr_cmaa2_compute_dispatch_args.errors)
                LOG_ERROR("Failed to compile cmaa2_compute_dispatch_args shader : {}:{} : {}", err.line + 1, err.column, err.message);
            return;
        }
        std::shared_ptr<Eng::Gfx::ShaderModule> sm_cmaa2_compute_dispatch_args = Eng::Gfx::ShaderModule::create(device, cr_cmaa2_compute_dispatch_args.stages.find(Eng::Gfx::EShaderStage::Compute)->second);
        pipeline_cmaa2_compute_dispatch_args                                   = Eng::Gfx::ComputePipeline::create("cmaa2_compute_dispatch_args", device, sm_cmaa2_compute_dispatch_args);
        descriptors_cmaa2_compute_dispatch_args                                = Eng::Gfx::DescriptorSet::create("cmaa2_compute_dispatch_args", device, pipeline_cmaa2_compute_dispatch_args->get_layout());

        auto cr_cmaa2_process_candidates = session->compile("cmaa2_process_candidates", session->get_default_permutations_description());
        if (!cr_cmaa2_process_candidates.errors.empty())
        {
            for (const auto err : cr_cmaa2_process_candidates.errors)
                LOG_ERROR("Failed to compile cmaa2_process_candidates shader : {}:{} : {}", err.line + 1, err.column, err.message);
            return;
        }
        std::shared_ptr<Eng::Gfx::ShaderModule> sm_cmaa2_process_candidates = Eng::Gfx::ShaderModule::create(device, cr_cmaa2_process_candidates.stages.find(Eng::Gfx::EShaderStage::Compute)->second);
        pipeline_cmaa2_process_candidates                                   = Eng::Gfx::ComputePipeline::create("cmaa2_process_candidates", device, sm_cmaa2_process_candidates);
        descriptors_cmaa2_process_candidates                                = Eng::Gfx::DescriptorSet::create("cmaa2_process_candidates", device, pipeline_cmaa2_process_candidates->get_layout());

        auto cr_cmaa2_deferred_color_apply2x2 = session->compile("cmaa2_deferred_color_apply2x2", session->get_default_permutations_description());
        if (!cr_cmaa2_deferred_color_apply2x2.errors.empty())
        {
            for (const auto err : cr_cmaa2_deferred_color_apply2x2.errors)
                LOG_ERROR("Failed to compile cmaa2_deferred_color_apply2x2 shader : {}:{} : {}", err.line + 1, err.column, err.message);
            return;
        }
        std::shared_ptr<Eng::Gfx::ShaderModule> sm_cmaa2_deferred_color_apply2x2 = Eng::Gfx::ShaderModule::create(device, cr_cmaa2_deferred_color_apply2x2.stages.find(Eng::Gfx::EShaderStage::Compute)->second);
        pipeline_cmaa2_deferred_color_apply2x2                                   = Eng::Gfx::ComputePipeline::create("cmaa2_deferred_color_apply2x2", device, sm_cmaa2_deferred_color_apply2x2);
        descriptors_cmaa2_deferred_color_apply2x2                                = Eng::Gfx::DescriptorSet::create("cmaa2_deferred_color_apply2x2", device, pipeline_cmaa2_deferred_color_apply2x2->get_layout());

        auto cr_cmaa2_debug_draw_edges = session->compile("cmaa2_debug_draw_edges", session->get_default_permutations_description());
        if (!cr_cmaa2_debug_draw_edges.errors.empty())
        {
            for (const auto err : cr_cmaa2_debug_draw_edges.errors)
                LOG_ERROR("Failed to compile cmaa2_debug_draw_edges shader : {}:{} : {}", err.line + 1, err.column, err.message);
            return;
        }
        std::shared_ptr<Eng::Gfx::ShaderModule> sm_cmaa2_debug_draw_edges = Eng::Gfx::ShaderModule::create(device, cr_cmaa2_debug_draw_edges.stages.find(Eng::Gfx::EShaderStage::Compute)->second);
        pipeline_cmaa2_debug_draw_edges                                   = Eng::Gfx::ComputePipeline::create("cmaa2_debug_draw_edges", device, sm_cmaa2_debug_draw_edges);
        descriptors_cmaa2_debug_draw_edges                                = Eng::Gfx::DescriptorSet::create("cmaa2_debug_draw_edges", device, pipeline_cmaa2_debug_draw_edges->get_layout());

    }

    void pre_draw(const Eng::Gfx::RenderPassInstanceBase&) override
    {

    }

    void draw(const Eng::Gfx::RenderPassInstanceBase&, Eng::Gfx::CommandBuffer& cmd, size_t) override
    {
        cmd.bind_compute_pipeline(*pipeline_cmaa2_edges_color_2x2);
        cmd.bind_descriptors(*descriptors_cmaa2_edges_color_2x2, *pipeline_cmaa2_edges_color_2x2);
        cmd.dispatch_compute();

        cmd.bind_compute_pipeline(*pipeline_cmaa2_compute_dispatch_args);
        cmd.bind_descriptors(*descriptors_cmaa2_compute_dispatch_args, *pipeline_cmaa2_compute_dispatch_args);
        cmd.dispatch_compute();

        cmd.bind_compute_pipeline(*pipeline_cmaa2_process_candidates);
        cmd.bind_descriptors(*descriptors_cmaa2_process_candidates, *pipeline_cmaa2_process_candidates);
        cmd.dispatch_compute();

        cmd.bind_compute_pipeline(*pipeline_cmaa2_deferred_color_apply2x2);
        cmd.bind_descriptors(*descriptors_cmaa2_deferred_color_apply2x2, *pipeline_cmaa2_deferred_color_apply2x2);
        cmd.dispatch_compute();

        cmd.bind_compute_pipeline(*pipeline_cmaa2_debug_draw_edges);
        cmd.bind_descriptors(*descriptors_cmaa2_debug_draw_edges, *pipeline_cmaa2_debug_draw_edges);
        cmd.dispatch_compute();
    }

    void pre_submit(const Eng::Gfx::RenderPassInstanceBase&) override
    {

    }

private:
    std::shared_ptr<Eng::Gfx::ComputePipeline> pipeline_cmaa2_edges_color_2x2;
    std::shared_ptr<Eng::Gfx::DescriptorSet>   descriptors_cmaa2_edges_color_2x2;

    std::shared_ptr<Eng::Gfx::ComputePipeline> pipeline_cmaa2_compute_dispatch_args;
    std::shared_ptr<Eng::Gfx::DescriptorSet>   descriptors_cmaa2_compute_dispatch_args;

    std::shared_ptr<Eng::Gfx::ComputePipeline> pipeline_cmaa2_process_candidates;
    std::shared_ptr<Eng::Gfx::DescriptorSet>   descriptors_cmaa2_process_candidates;

    std::shared_ptr<Eng::Gfx::ComputePipeline> pipeline_cmaa2_deferred_color_apply2x2;
    std::shared_ptr<Eng::Gfx::DescriptorSet>   descriptors_cmaa2_deferred_color_apply2x2;

    std::shared_ptr<Eng::Gfx::ComputePipeline> pipeline_cmaa2_debug_draw_edges;
    std::shared_ptr<Eng::Gfx::DescriptorSet>   descriptors_cmaa2_debug_draw_edges;
};

void Cmaa2::append_to_renderer(Eng::Gfx::Renderer& renderer)
{
    renderer["cmaa2"]
        .require("gbuffer_resolve")
        .render_pass<CmaaRenderPass>()
        .compute_pass(true)
        [Eng::Gfx::Attachment::slot("target").format(Eng::Gfx::ColorFormat::R8G8B8A8_UNORM)];
}