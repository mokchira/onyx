#include "r_renderpass.h"
#include "r_render.h"

#include "t_def.h"
#include "v_video.h"

void obdn_r_CreateRenderPass(const Obdn_R_RenderPassInfo *info, VkRenderPass *pRenderPass)
{
    // only allow one subpass for now
    assert(info->subpassCount == 1);

    const VkSubpassDependency dependency0 = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    const VkSubpassDependency dependency1 = {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    VkSubpassDependency dependencies[] = {
        dependency0, dependency1
    };

    VkRenderPassCreateInfo ci = {
        .subpassCount = 1,
        .pSubpasses = info->pSubpasses,
        .attachmentCount = info->attachmentCount,
        .pAttachments = info->pAttachments,
        .dependencyCount = 2,
        .pDependencies = dependencies,
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO
    };

    V_ASSERT( vkCreateRenderPass(device, &ci, NULL, pRenderPass) );
}

void obdn_r_CreateRenderPass_Color(const VkAttachmentLoadOp loadOp, 
        const VkImageLayout initialLayout, const VkImageLayout finalLayout,
        const VkFormat colorFormat,
        VkRenderPass* pRenderPass)
{
    const VkAttachmentDescription attachmentColor = {
        .format = colorFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT, // TODO look into what this means
        .loadOp = loadOp,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = initialLayout,
        .finalLayout = finalLayout,
    };

    const VkAttachmentReference referenceColor = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };
    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &referenceColor,
        .pDepthStencilAttachment = NULL,
        .inputAttachmentCount = 0,
        .preserveAttachmentCount = 0,
    };

    const VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkAttachmentDescription attachments[] = {
        attachmentColor,
    };

    VkRenderPassCreateInfo ci = {
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .attachmentCount = OBDN_ARRAY_SIZE(attachments),
        .pAttachments = attachments,
        .dependencyCount = 1,
        .pDependencies = &dependency,
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO
    };

    V_ASSERT( vkCreateRenderPass(device, &ci, NULL, pRenderPass) );
}

void obdn_r_CreateRenderPass_ColorDepth(
        const VkImageLayout colorInitialLayout, const VkImageLayout colorFinalLayout,
        const VkImageLayout depthInitialLayout, const VkImageLayout depthFinalLayout,
        const VkAttachmentLoadOp  colorLoadOp, const VkAttachmentStoreOp colorStoreOp,
        const VkAttachmentLoadOp  depthLoadOp, const VkAttachmentStoreOp depthStoreOp,
        const VkFormat colorFormat,
        const VkFormat depthFormat,
        VkRenderPass* pRenderPass)
{
    const VkAttachmentDescription attachmentColor = {
        .format         = colorFormat,
        .samples        = VK_SAMPLE_COUNT_1_BIT, // TODO look into what this means
        .loadOp         = colorLoadOp,
        .storeOp        = colorStoreOp,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = colorInitialLayout,
        .finalLayout    = colorFinalLayout,
    };

    const VkAttachmentDescription attachmentDepth = {
        .format         = depthFormat,
        .samples        = VK_SAMPLE_COUNT_1_BIT, // TODO look into what this means
        .loadOp         = depthLoadOp,
        .storeOp        = depthStoreOp,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = depthInitialLayout,
        .finalLayout    = depthFinalLayout,
    };

    const VkAttachmentReference referenceColor = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    const VkAttachmentReference referenceDepth = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &referenceColor,
        .pDepthStencilAttachment = &referenceDepth,
        .inputAttachmentCount = 0,
        .preserveAttachmentCount = 0,
    };

    const VkSubpassDependency dependency1 = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
    };

    const VkSubpassDependency dependency2 = {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        .dstAccessMask = 0,
    };

    VkAttachmentDescription attachments[] = {
        attachmentColor,
        attachmentDepth,
    };

    const VkSubpassDependency dependencies[] = {
        dependency1, dependency2
    };

    VkRenderPassCreateInfo ci = {
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .attachmentCount = 2,
        .pAttachments = attachments,
        .dependencyCount = OBDN_ARRAY_SIZE(dependencies),
        .pDependencies = dependencies,
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO
    };

    V_ASSERT( vkCreateRenderPass(device, &ci, NULL, pRenderPass) );
}

void obdn_r_CreateRenderPass_ColorDepthMSAA(const VkSampleCountFlags sampleCount, const VkAttachmentLoadOp loadOp, 
        const VkImageLayout initialLayout, const VkImageLayout finalLayout,
        const VkFormat colorFormat,
        const VkFormat depthFormat,
        VkRenderPass* pRenderPass)
{
    const VkAttachmentDescription attachmentColor = {
        .format = colorFormat,
        .samples = sampleCount, // TODO look into what this means
        .loadOp = loadOp,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = initialLayout,
        .finalLayout = VK_IMAGE_LAYOUT_GENERAL,
    };

    const VkAttachmentDescription attachmentDepth = {
        .format = depthFormat,
        .samples = sampleCount, // TODO look into what this means
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    const VkAttachmentDescription attachmentPresent = {
        .format = colorFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT, // TODO look into what this means
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = finalLayout,
    };

    VkAttachmentDescription attachments[] = {
        attachmentColor,
        attachmentDepth,
        attachmentPresent
    };

    const VkAttachmentReference referenceColor = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    const VkAttachmentReference referenceDepth = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    const VkAttachmentReference referenceResolve = {
        .attachment = 2,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pResolveAttachments  = &referenceResolve,
        .pColorAttachments    = &referenceColor,
        .pDepthStencilAttachment = &referenceDepth,
        .inputAttachmentCount = 0,
        .preserveAttachmentCount = 0,
    };

    const VkSubpassDependency dependency0 = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    const VkSubpassDependency dependency1 = {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    VkSubpassDependency dependencies[] = {
        dependency0, dependency1
    };

    VkRenderPassCreateInfo ci = {
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .attachmentCount = 3,
        .pAttachments = attachments,
        .dependencyCount = 2,
        .pDependencies = dependencies,
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO
    };

    V_ASSERT( vkCreateRenderPass(device, &ci, NULL, pRenderPass) );
}
