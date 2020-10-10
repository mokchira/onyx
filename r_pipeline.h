#ifndef TANTO_R_PIPELINE_H
#define TANTO_R_PIPELINE_H

#include "v_def.h"

#define TANTO_MAX_PIPELINES 10
#define TANTO_MAX_DESCRIPTOR_SETS 10
#define TANTO_MAX_BINDINGS 10
#define TANTO_MAX_PUSH_CONSTANTS 5

#define TANTO_SPVDIR "/home/michaelb/dev/tanto/shaders/spv"

extern VkPipeline       pipelines[TANTO_MAX_PIPELINES];
extern VkDescriptorSet  descriptorSets[TANTO_MAX_DESCRIPTOR_SETS];
extern VkPipelineLayout pipelineLayouts[TANTO_MAX_PIPELINES];

typedef struct {
    uint32_t           descriptorCount;
    VkDescriptorType   type;
    VkShaderStageFlags stageFlags;
} Tanto_R_DescriptorBinding;

typedef struct {
    int    id;
    size_t bindingCount;
    Tanto_R_DescriptorBinding bindings[TANTO_MAX_BINDINGS];
} Tanto_R_DescriptorSet;

typedef struct {
    int id;
    size_t descriptorSetCount;
    int    descriptorSetIds[TANTO_MAX_DESCRIPTOR_SETS];
    size_t pushConstantCount;
    VkPushConstantRange pushConstantsRanges[TANTO_MAX_PUSH_CONSTANTS];
} Tanto_R_PipelineLayout;

typedef enum {
    TANTO_R_PIPELINE_RASTER_TYPE,
    TANTO_R_PIPELINE_RAYTRACE_TYPE,
    TANTO_R_PIPELINE_POSTPROC_TYPE
} Tanto_R_PipelineType;

typedef enum {
    TANTO_R_RENDER_PASS_SWAPCHAIN_TYPE,
    TANTO_R_RENDER_PASS_OFFSCREEN_TYPE
} Tanto_R_RenderPassType;

typedef struct {
    const Tanto_R_RenderPassType renderPassType;
    const char* vertShader;
    const char* fragShader;
} Tanto_R_PipelineRasterInfo;

typedef struct {
} Tanto_R_PipelineRayTraceInfo;

typedef struct {
    const int   id;
    const Tanto_R_PipelineType type;
    const int   pipelineLayoutId;
    const Tanto_R_PipelineRasterInfo rasterInfo;
    const Tanto_R_PipelineRayTraceInfo raytraceInfo;
} Tanto_R_PipelineInfo;


void tanto_r_InitDescriptorSets(const Tanto_R_DescriptorSet* const sets, const int count);
void tanto_r_InitPipelineLayouts(const Tanto_R_PipelineLayout* const layouts, const int count);
void tanto_r_InitPipelines(const Tanto_R_PipelineInfo* const pipelineInfos, const int count);
void tanto_r_CleanUpPipelines(void);

#endif /* end of include guard: R_PIPELINE_H */

