#include <hell/common.h>
#include <hell/debug.h>
#include <hell/window.h>
#include <hell/cmd.h>
#include <coal/coal.h>
#include "common.h"
#include "v_swapchain.h"
#include "s_scene.h"
#include "r_pipeline.h"
#include "r_pipeline.h"
#include "r_geo.h"
#include "r_renderpass.h"
#include "s_scene.h"
#include "u_ui.h"
#include <alloca.h>

#define WIREFRAME 0

static Obdn_R_Primitive    triangle;

static VkFramebuffer       framebuffer;
static VkRenderPass        renderPass;
static VkPipelineLayout    pipelineLayout;
static VkPipeline          pipeline;
static VkFramebuffer       framebuffers[2];
static Obdn_Image          depthImage;

static VkFence      drawFence;
static VkSemaphore  acquireSemaphore;
static VkSemaphore  drawSemaphore;
static Obdn_Command drawCommands[2];

static Obdn_Swapchain* swapchain;

static const VkFormat depthFormat = VK_FORMAT_D24_UNORM_S8_UINT;

static Obdn_Instance*  oInstance;
static Obdn_Memory*    oMemory;
static Obdn_Swapchain* swapchain;
static Obdn_UI*        ui;
static VkDevice        device;

static Obdn_U_Widget* uiBox;
static Obdn_U_Widget* uiText;

static void createSurfaceDependent(void)
{
    VkExtent2D dim = obdn_GetSwapchainExtent(swapchain);
    depthImage = obdn_CreateImage(oMemory, dim.width, dim.height,
                       depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                       VK_IMAGE_ASPECT_DEPTH_BIT, VK_SAMPLE_COUNT_1_BIT, 1,
                       OBDN_V_MEMORY_DEVICE_TYPE);
    VkImageView attachments_0[2] = {obdn_GetSwapchainImageView(swapchain, 0), depthImage.view};
    VkImageView attachments_1[2] = {obdn_GetSwapchainImageView(swapchain, 1), depthImage.view};
    obdn_CreateFramebuffer(oInstance, 2, attachments_0, dim.width, dim.height, renderPass, &framebuffers[0]);
    obdn_CreateFramebuffer(oInstance, 2, attachments_1, dim.width, dim.height, renderPass, &framebuffers[1]);

    // ui recreation
    obdn_RecreateSwapchainDependentUI(ui, dim.width, dim.height, 2, obdn_GetSwapchainImageViews(swapchain));
}

static void destroySurfaceDependent(void)
{
    obdn_FreeImage(&depthImage);
    obdn_DestroyFramebuffer(oInstance, framebuffers[0]);
    obdn_DestroyFramebuffer(oInstance, framebuffers[1]);
}

static void onSwapchainRecreate(void)
{
    destroySurfaceDependent();
    createSurfaceDependent();
}

void init(void)
{
    uint8_t attrSize = 3 * sizeof(float);
    triangle = obdn_CreatePrimitive(oMemory, 3, 4, 1, &attrSize);
    Coal_Vec3* verts = (Coal_Vec3*)triangle.vertexRegion.hostData;
    verts[0].x =  0.0;
    verts[0].y = -1.0;
    verts[0].z =  0.0;
    verts[1].x = -1.0;
    verts[1].y =  1.0;
    verts[1].z =  0.0;
    verts[2].x =  1.0;
    verts[2].y =  1.0;
    verts[2].z =  0.0;
    Obdn_AttrIndex* indices = (Obdn_AttrIndex*)triangle.indexRegion.hostData;
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 0; // this last index is so we render the full triangle in line mode

    obdn_r_PrintPrim(&triangle);

    // call this render pass joe
    obdn_CreateRenderPass_ColorDepth(device, 
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE,
        obdn_GetSwapchainFormat(swapchain), depthFormat, &renderPass);
    Obdn_R_PipelineLayoutInfo pli = {0}; //nothin
    obdn_CreatePipelineLayouts(device, 1, &pli, &pipelineLayout);
    if (WIREFRAME)
        obdn_CreateGraphicsPipeline_Taurus(device, renderPass, pipelineLayout, VK_POLYGON_MODE_LINE, &pipeline);
    else
        obdn_CreateGraphicsPipeline_Taurus(device, renderPass, pipelineLayout, VK_POLYGON_MODE_FILL, &pipeline);

    createSurfaceDependent();
    obdn_CreateFence(device, &drawFence);
    obdn_CreateSemaphore(device, &drawSemaphore);
    obdn_CreateSemaphore(device, &acquireSemaphore);
    drawCommands[0] = obdn_CreateCommand(oInstance, OBDN_V_QUEUE_GRAPHICS_TYPE);
    drawCommands[1] = obdn_CreateCommand(oInstance, OBDN_V_QUEUE_GRAPHICS_TYPE);
}

#define TARGET_RENDER_INTERVAL 500 

void draw(void)
{
    static Hell_Tick timeOfLastRender = 0;
    static Hell_Tick timeSinceLastRender = TARGET_RENDER_INTERVAL;
    static uint64_t frameCounter = 0;
    timeSinceLastRender = hell_Time() - timeOfLastRender;
    if (timeSinceLastRender < TARGET_RENDER_INTERVAL)
        return;
    timeOfLastRender = hell_Time();
    timeSinceLastRender = 0;

    VkFence fence = VK_NULL_HANDLE;
    bool swapchainDirty;
    unsigned frameId = obdn_AcquireSwapchainImage(swapchain, &fence, &acquireSemaphore, &swapchainDirty);

    if (swapchainDirty)
        onSwapchainRecreate();

    Obdn_Command cmd = drawCommands[frameCounter % 2];

    obdn_ResetCommand(&cmd);
    
    obdn_BeginCommandBuffer(cmd.buffer);

    const VkExtent2D dim = obdn_GetSwapchainExtent(swapchain);
    obdn_CmdSetViewportScissorFull(cmd.buffer, dim.width, dim.height);

    obdn_CmdBeginRenderPass_ColorDepth(
        cmd.buffer, renderPass, framebuffers[frameId], dim.width,
        dim.height, 0.0, 0.0, 0.01, 1.0);

        vkCmdBindPipeline(cmd.buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        obdn_r_DrawPrim(cmd.buffer, &triangle);

    obdn_CmdEndRenderPass(cmd.buffer);

    obdn_EndCommandBuffer(cmd.buffer);

    obdn_SubmitGraphicsCommand(oInstance, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 1,
            &acquireSemaphore, 1, &drawSemaphore, drawFence, cmd.buffer);

    VkSemaphore uiSemaphore = obdn_RenderUI(ui, frameId, dim.width, dim.height, drawSemaphore);

    bool result = obdn_PresentFrame(swapchain, 1, &uiSemaphore);

    obdn_WaitForFence(device, &drawFence);

    frameCounter++;
}

int main(int argc, char *argv[])
{
    hell_Init(true, draw, 0);
    hell_c_SetVar("maxFps", "10000", 0);
    hell_Print("Starting hello triangle.\n");
    const Hell_Window* window = hell_OpenWindow(500, 500, 0);

    Obdn_V_MemorySizes memSizes = {
        .deviceGraphicsImageMemorySize = OBDN_100_MiB,
        .deviceGraphicsBufferMemorySize = OBDN_100_MiB,
        .hostGraphicsBufferMemorySize = OBDN_100_MiB,
    };

    oInstance = alloca(obdn_SizeOfInstance());
    oMemory   = alloca(obdn_SizeOfMemory());
    swapchain = alloca(obdn_SizeOfSwapchain());
    ui        = alloca(obdn_SizeOfUI());

    obdn_Init(oInstance);
    device = obdn_GetDevice(oInstance);
    obdn_InitSwapchain(oInstance, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, window, swapchain);
    obdn_InitMemory(oInstance, &memSizes, oMemory);

    obdn_CreateUI(oMemory, obdn_GetSwapchainFormat(swapchain), window->width,
                window->height, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                obdn_GetSwapchainImageCount(swapchain),
                obdn_GetSwapchainImageViews(swapchain), ui);
    uiBox = obdn_CreateSimpleBoxWidget(ui, 10, 10, 200, 100, NULL);
    uiText = obdn_CreateTextWidget(ui, 40, 40, "Blumpkin", uiBox);
    init();
    hell_Loop();
    return 0;
}