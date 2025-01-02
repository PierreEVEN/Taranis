#include "cmaa.hpp"

#include "engine.hpp"
#include "object_ptr.hpp"
#include "assets/material_asset.hpp"
#include "gfx/renderer/definition/renderer.hpp"
#include "gfx/renderer/instance/render_pass_instance_base.hpp"
#include "gfx/vulkan/command_buffer.hpp"
#include "gfx/vulkan/compute_pipeline.hpp"
#include "gfx/vulkan/descriptor_sets.hpp"
#include "gfx/vulkan/image_view.hpp"
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

    void on_create_framebuffer(const Eng::Gfx::RenderPassInstanceBase& rp) override
    {
        auto device       = Eng::Engine::get().get_device();
        auto input_image  = rp.get_dependencies("gbuffer_resolve")[0].lock()->get_image_resource(rp.get_dependencies("gbuffer_resolve")[0].lock()->get_image_resources()[0]).lock();
        auto output_image = rp.get_image_resource(rp.get_image_resources()[0]).lock();
        g_workingDeferredBlendItemListHeads =
            Eng::Gfx::ImageView::create("g_workingDeferredBlendItemListHeads", Eng::Gfx::Image::create("g_workingDeferredBlendItemListHeads", device,
                                                                                                       Eng::Gfx::ImageParameter{.format     = Eng::Gfx::ColorFormat::R32_UINT,
                                                                                                                                .is_storage = true,
                                                                                                                                .width      = static_cast<uint32_t>(static_cast<float>(rp.resolution().x + 1) / 2.f),
                                                                                                                                .height     = static_cast<uint32_t>(static_cast<float>(rp.resolution().y + 1) / 2.f)}));
        g_workingEdges = Eng::Gfx::ImageView::create("g_workingEdges", Eng::Gfx::Image::create("g_workingEdges", device,
                                                                                               Eng::Gfx::ImageParameter{.format     = Eng::Gfx::ColorFormat::R8_UINT,
                                                                                                                        .is_storage = true,
                                                                                                                        .width      = static_cast<uint32_t>(static_cast<float>(rp.resolution().x)),
                                                                                                                        .height     = static_cast<uint32_t>(static_cast<float>(rp.resolution().y))}));

        int requiredCandidatePixels = rp.resolution().x * rp.resolution().y / 4;
        g_workingShapeCandidates    = Eng::Gfx::Buffer::create("g_workingShapeCandidates", device, {Eng::Gfx::EBufferUsage::GPU_MEMORY, Eng::Gfx::EBufferType::IMMEDIATE}, sizeof(uint32_t), requiredCandidatePixels);
        g_workingControlBuffer      = Eng::Gfx::Buffer::create("g_workingControlBuffer", device, {Eng::Gfx::EBufferUsage::GPU_MEMORY, Eng::Gfx::EBufferType::IMMEDIATE}, sizeof(uint32_t), requiredCandidatePixels);
        g_workingDeferredBlendLocationList = Eng::Gfx::Buffer::create("g_workingDeferredBlendLocationList", device, {Eng::Gfx::EBufferUsage::GPU_MEMORY, Eng::Gfx::EBufferType::IMMEDIATE}, sizeof(uint32_t), 16);
        g_workingDeferredBlendItemList =
            Eng::Gfx::Buffer::create("g_workingDeferredBlendItemList", device, {Eng::Gfx::EBufferUsage::GPU_MEMORY, Eng::Gfx::EBufferType::IMMEDIATE}, sizeof(uint32_t), requiredCandidatePixels);

        g_workingExecuteIndirectBuffer = Eng::Gfx::Buffer::create("g_workingExecuteIndirectBuffer", device, {Eng::Gfx::EBufferUsage::INDIRECT_DRAW_ARGUMENT, Eng::Gfx::EBufferType::IMMEDIATE}, sizeof(uint32_t), 4);

        descriptors_cmaa2_edges_color_2x2->bind_image("g_workingDeferredBlendItemListHeads", g_workingDeferredBlendItemListHeads);
        descriptors_cmaa2_edges_color_2x2->bind_image("g_workingEdges", g_workingEdges);
        descriptors_cmaa2_edges_color_2x2->bind_image("g_inoutColorReadonly", input_image);
        descriptors_cmaa2_edges_color_2x2->bind_buffer("g_workingShapeCandidates", g_workingShapeCandidates);
        descriptors_cmaa2_edges_color_2x2->bind_buffer("g_workingControlBuffer", g_workingControlBuffer);

        descriptors_cmaa2_compute_dispatch_args->bind_buffer("g_workingControlBuffer", g_workingControlBuffer);
        descriptors_cmaa2_compute_dispatch_args->bind_buffer("g_workingExecuteIndirectBuffer", g_workingExecuteIndirectBuffer);
        descriptors_cmaa2_compute_dispatch_args->bind_buffer("g_workingShapeCandidates", g_workingShapeCandidates);

        descriptors_cmaa2_process_candidates->bind_image("g_inoutColorReadonly", input_image);
        descriptors_cmaa2_process_candidates->bind_image("g_workingEdges", g_workingEdges);
        descriptors_cmaa2_process_candidates->bind_image("g_workingDeferredBlendItemListHeads", g_workingDeferredBlendItemListHeads);
        descriptors_cmaa2_process_candidates->bind_buffer("g_workingShapeCandidates", g_workingShapeCandidates);
        descriptors_cmaa2_process_candidates->bind_buffer("g_workingControlBuffer", g_workingControlBuffer);
        descriptors_cmaa2_process_candidates->bind_buffer("g_workingDeferredBlendLocationList", g_workingDeferredBlendLocationList);
        descriptors_cmaa2_process_candidates->bind_buffer("g_workingDeferredBlendItemList", g_workingDeferredBlendItemList);

        descriptors_cmaa2_compute_dispatch_args->bind_buffer("g_workingDeferredBlendLocationList", g_workingDeferredBlendLocationList);

        descriptors_cmaa2_deferred_color_apply2x2->bind_buffer("g_workingControlBuffer", g_workingControlBuffer);
        descriptors_cmaa2_deferred_color_apply2x2->bind_buffer("g_workingDeferredBlendLocationList", g_workingDeferredBlendLocationList);
        descriptors_cmaa2_deferred_color_apply2x2->bind_image("g_workingDeferredBlendItemListHeads", g_workingDeferredBlendItemListHeads);
        descriptors_cmaa2_deferred_color_apply2x2->bind_buffer("g_workingDeferredBlendItemList", g_workingDeferredBlendItemList);
        descriptors_cmaa2_deferred_color_apply2x2->bind_image("g_inoutColorWriteonly", output_image);

        descriptors_cmaa2_debug_draw_edges->bind_image("g_inoutColorWriteonly", output_image);
        descriptors_cmaa2_debug_draw_edges->bind_image("g_workingEdges", g_workingEdges);
    }

    void pre_draw(const Eng::Gfx::RenderPassInstanceBase&) override
    {

    }

    void draw(const Eng::Gfx::RenderPassInstanceBase& rp, Eng::Gfx::CommandBuffer& cmd, size_t) override
    {

        // We have to clear m_workingControlBufferResource during the first run so just execute second ComputeDispatchArgs that does it anyway,
        // there are no bad side-effects from this and it saves creating another shader/pso
        // Reminder: consider using ID3D12CommandList2::WriteBufferImmediate instead.
        if (b_first_run)
        {
            b_first_run = false;
            cmd.bind_compute_pipeline(*pipeline_cmaa2_compute_dispatch_args);
            cmd.bind_descriptors(*descriptors_cmaa2_compute_dispatch_args, *pipeline_cmaa2_compute_dispatch_args);
            cmd.dispatch_compute(2);
        }

        // first pass edge detect
        {
            cmd.bind_compute_pipeline(*pipeline_cmaa2_edges_color_2x2);
            cmd.bind_descriptors(*descriptors_cmaa2_edges_color_2x2, *pipeline_cmaa2_edges_color_2x2);
            constexpr int CMAA2_CS_INPUT_KERNEL_SIZE_X = 16;
            constexpr int CMAA2_CS_INPUT_KERNEL_SIZE_Y = 16;
            int           csOutputKernelSizeX          = CMAA2_CS_INPUT_KERNEL_SIZE_X - 2;
            int           csOutputKernelSizeY          = CMAA2_CS_INPUT_KERNEL_SIZE_Y - 2;
            int           threadGroupCountX            = (rp.resolution().x + csOutputKernelSizeX * 2 - 1) / (csOutputKernelSizeX * 2);
            int           threadGroupCountY            = (rp.resolution().y + csOutputKernelSizeY * 2 - 1) / (csOutputKernelSizeY * 2);
            cmd.dispatch_compute(threadGroupCountX, threadGroupCountY);
        }

        // Set up for the first DispatchIndirect
        {
            cmd.bind_compute_pipeline(*pipeline_cmaa2_compute_dispatch_args);
            cmd.bind_descriptors(*descriptors_cmaa2_compute_dispatch_args, *pipeline_cmaa2_compute_dispatch_args);
            cmd.dispatch_compute(2, 1);
        }

        // Process shape candidates DispatchIndirect
        {
            cmd.bind_compute_pipeline(*pipeline_cmaa2_process_candidates);
            cmd.bind_descriptors(*descriptors_cmaa2_process_candidates, *pipeline_cmaa2_process_candidates);
            cmd.dispatch_compute_indirect(*g_workingExecuteIndirectBuffer);
        }

        // Set up for the second DispatchIndirect
        {
            cmd.bind_compute_pipeline(*pipeline_cmaa2_compute_dispatch_args);
            cmd.bind_descriptors(*descriptors_cmaa2_compute_dispatch_args, *pipeline_cmaa2_compute_dispatch_args);
            cmd.dispatch_compute(1, 2);
        }

        // Resolve & apply blended colors
        {
            cmd.bind_compute_pipeline(*pipeline_cmaa2_deferred_color_apply2x2);
            cmd.bind_descriptors(*descriptors_cmaa2_deferred_color_apply2x2, *pipeline_cmaa2_deferred_color_apply2x2);
            cmd.dispatch_compute_indirect(*g_workingExecuteIndirectBuffer);
        }

        if (true)
        {
            int tgcX = (rp.resolution().x + 16 - 1) / 16;
            int tgcY = (rp.resolution().y + 16 - 1) / 16;

            cmd.bind_compute_pipeline(*pipeline_cmaa2_debug_draw_edges);
            cmd.bind_descriptors(*descriptors_cmaa2_debug_draw_edges, *pipeline_cmaa2_debug_draw_edges);
            cmd.dispatch_compute(tgcX, tgcY);
        }
    }

    void pre_submit(const Eng::Gfx::RenderPassInstanceBase&) override
    {

    }

private:
    std::shared_ptr<Eng::Gfx::ComputePipeline> pipeline_cmaa2_edges_color_2x2;
    std::shared_ptr<Eng::Gfx::DescriptorSet>   descriptors_cmaa2_edges_color_2x2;
    std::shared_ptr<Eng::Gfx::ImageView>       g_workingDeferredBlendItemListHeads;
    std::shared_ptr<Eng::Gfx::ImageView>       g_workingEdges;
    std::shared_ptr<Eng::Gfx::Buffer>          g_workingShapeCandidates;
    std::shared_ptr<Eng::Gfx::Buffer>          g_workingControlBuffer;
    std::shared_ptr<Eng::Gfx::Buffer>          g_workingDeferredBlendLocationList;
    std::shared_ptr<Eng::Gfx::Buffer>          g_workingDeferredBlendItemList;

    std::shared_ptr<Eng::Gfx::Buffer> g_workingExecuteIndirectBuffer;

    std::shared_ptr<Eng::Gfx::ComputePipeline> pipeline_cmaa2_compute_dispatch_args;
    std::shared_ptr<Eng::Gfx::DescriptorSet>   descriptors_cmaa2_compute_dispatch_args;

    std::shared_ptr<Eng::Gfx::ComputePipeline> pipeline_cmaa2_process_candidates;
    std::shared_ptr<Eng::Gfx::DescriptorSet>   descriptors_cmaa2_process_candidates;

    std::shared_ptr<Eng::Gfx::ComputePipeline> pipeline_cmaa2_deferred_color_apply2x2;
    std::shared_ptr<Eng::Gfx::DescriptorSet>   descriptors_cmaa2_deferred_color_apply2x2;

    std::shared_ptr<Eng::Gfx::ComputePipeline> pipeline_cmaa2_debug_draw_edges;
    std::shared_ptr<Eng::Gfx::DescriptorSet>   descriptors_cmaa2_debug_draw_edges;

    bool b_first_run = true;
};

void Cmaa2::append_to_renderer(Eng::Gfx::Renderer& renderer)
{
    renderer["cmaa2"]
        .require("gbuffer_resolve")
        .render_pass<CmaaRenderPass>()
        .compute_pass(true)
        [Eng::Gfx::Attachment::slot("target").format(Eng::Gfx::ColorFormat::R8G8B8A8_UNORM)];
}