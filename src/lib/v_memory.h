#ifndef OBDN_V_MEMORY_H
#define OBDN_V_MEMORY_H

#include "v_video.h"
#include <stdbool.h>


// TODO this is brittle. we should probably change V_MemoryType to a mask with
// flags for the different
// requirements. One bit for host or device, one bit for transfer capabale, one
// bit for external, one bit for image or buffer
typedef enum {
    OBDN_V_MEMORY_HOST_GRAPHICS_TYPE,
    OBDN_V_MEMORY_HOST_TRANSFER_TYPE,
    OBDN_V_MEMORY_DEVICE_TYPE,
    OBDN_V_MEMORY_EXTERNAL_DEVICE_TYPE
} Obdn_V_MemoryType;

typedef struct Obdn_Memory Obdn_Memory;

struct BlockChain;

typedef struct {
    VkDeviceSize       size;
    VkDeviceSize       offset;
    VkBuffer           buffer;
    uint8_t*           hostData; // if hostData not null, its mapped
    uint32_t           memBlockId;
    struct BlockChain* pChain;
} Obdn_V_BufferRegion;

typedef struct {
    VkImage            handle;
    VkImageView        view;
    VkSampler          sampler;
    VkDeviceSize       size; // size in bytes. taken from GetMemReqs
    VkDeviceSize       offset;
    VkExtent3D         extent;
    VkImageLayout      layout;
    uint32_t           mipLevels;
    uint32_t           queueFamily;
    uint32_t           memBlockId;
    struct BlockChain* pChain;
} Obdn_V_Image;

#define OBDN_1_MiB (VkDeviceSize)0x100000
#define OBDN_100_MiB (VkDeviceSize)0x6400000
#define OBDN_256_MiB (VkDeviceSize)0x10000000
#define OBDN_512_MiB (VkDeviceSize)0x20000000
#define OBDN_1_GiB (VkDeviceSize)0x40000000

uint64_t obdn_SizeOfMemory(void);
void     obdn_InitMemory(const Obdn_Instance*, const Obdn_V_MemorySizes*, Obdn_Memory*);

Obdn_V_BufferRegion obdn_RequestBufferRegion(Obdn_Memory*, size_t size,
                                             const VkBufferUsageFlags,
                                             const Obdn_V_MemoryType);

Obdn_V_BufferRegion obdn_RequestBufferRegionAligned(Obdn_Memory*, const size_t size,
                                                    uint32_t     alignment,
                                                    const Obdn_V_MemoryType);

uint32_t obdn_GetMemoryType(const Obdn_Memory*, uint32_t                    typeBits,
                            const VkMemoryPropertyFlags properties);

Obdn_V_Image obdn_CreateImage(Obdn_Memory*, const uint32_t width, const uint32_t height,
                              const VkFormat           format,
                              const VkImageUsageFlags  usageFlags,
                              const VkImageAspectFlags aspectMask,
                              const VkSampleCountFlags sampleCount,
                              const uint32_t           mipLevels,
                              const Obdn_V_MemoryType);

void obdn_CopyBufferRegion(const Obdn_V_BufferRegion* src,
                        Obdn_V_BufferRegion*       dst);

void obdn_CopyImageToBufferRegion(const Obdn_V_Image*  image,
                                  Obdn_V_BufferRegion* bufferRegion);

void obdn_TransferToDevice(Obdn_Memory* memory, Obdn_V_BufferRegion* pRegion);

void obdn_FreeImage(Obdn_V_Image* image);

void obdn_FreeBufferRegion(Obdn_V_BufferRegion* pRegion);

VkDeviceAddress obdn_GetBufferRegionAddress(const Obdn_V_BufferRegion* region);

void obdn_CleanUpMemory(void);

// application's job to destroy this buffer and free the memory
void obdn_CreateUnmanagedBuffer(Obdn_Memory* memory, const VkBufferUsageFlags bufferUsageFlags,
                                const uint32_t           memorySize,
                                const Obdn_V_MemoryType  type,
                                VkDeviceMemory* pMemory, VkBuffer* pBuffer);

const VkDeviceMemory obdn_GetDeviceMemory(const Obdn_Memory* memory, const Obdn_V_MemoryType memType);
const VkDeviceSize   obdn_GetMemorySize(const Obdn_Memory* memory, const Obdn_V_MemoryType memType);

#endif /* end of include guard: V_MEMORY_H */