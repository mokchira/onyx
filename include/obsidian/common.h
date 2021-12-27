#ifndef OBSIDIAN_COMMON_H
#define OBSIDIAN_COMMON_H

#include "vulkan.h"
#include "video.h"

struct Hell_Window;

#define OBDN_CVAR_NAME_HOST_GRAPHICS_BUFFER_MEMORY_SIZE                        \
    "hstGraphicsBufferMemorySize"
#define OBDN_CVAR_NAME_DEV_GRAPHICS_BUFFER_MEMORY_SIZE                         \
    "devGraphicsBufferMemorySize"
#define OBDN_CVAR_NAME_DEV_GRAPHICS_IMAGE_MEMORY_SIZE                          \
    "devGraphicsImageMemorySize"
#define OBDN_CVAR_NAME_HOST_TRANSFER_BUFFER_MEMORY_SIZE                        \
    "hstTransferBufferMemorySize"
#define OBDN_CVAR_NAME_EXTERNAL_GRAPHICS_IMAGE_MEMORY_SIZE                     \
    "extGraphicsImageMemorySize"

void obdn_Announce(const char *fmt, ...);

// if null is passed it will be an offscreen swapchain.
// returns the swapchain id. currently not used for anything.

void obdn_CreateFramebuffer(const VkDevice, const unsigned attachmentCount, const VkImageView* attachments, 
        const unsigned width, const unsigned height, 
        const VkRenderPass renderpass,
        VkFramebuffer* framebuffer);

void obdn_DestroyFramebuffer(const VkDevice, VkFramebuffer fb);

#endif /* end of include guard: OBSIDIAN_COMMON_H */
