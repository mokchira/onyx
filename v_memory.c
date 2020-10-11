#include "v_memory.h"
#include "v_video.h"
#include "t_def.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

// HVC = Host Visible and Coherent
#define MEMORY_SIZE_HOST 16777216
#define BUFFER_SIZE_HOST 16777216 // 2 MiB
// DL = Device Local    
#define MEMORY_SIZE_DEV_IMAGE   33554432 // 32 MiB
#define MEMORY_SIZE_DEV_BUFFER  33554432 // 32 MiB
#define MAX_BLOCKS 256

static VkDeviceMemory memoryDeviceLocal;

static VkPhysicalDeviceMemoryProperties memoryProperties;

static uint32_t curDevMemoryOffset;

struct Pool {
    int count;
    int bytesAvailable;
    int curOffset;
    VkDeviceMemory memory;
};

static struct HbPool {
    struct Pool pool;
    uint8_t* hostData;
    VkBuffer buffer;
    Tanto_V_BlockHostBuffer blocks[MAX_BLOCKS];
} hbPool;

struct BlockChain {
    size_t            totalSize;
    size_t            count;
    size_t            cur;
    VkDeviceMemory    memory;
    Tanto_V_MemBlock  blocks[MAX_BLOCKS];
};

static struct BlockChain diBlockChain;
static struct BlockChain dbBlockChain;

static void initPool(const VkDeviceSize size, const uint32_t memTypeIndex, struct Pool* pool)
{
    pool->count = 0;
    pool->bytesAvailable = size;
    pool->curOffset = 0;

    const VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = size,
        .memoryTypeIndex = memTypeIndex
    };

    V_ASSERT( vkAllocateMemory(device, &allocInfo, NULL, &pool->memory) );
}

static void initHbPool(const VkBufferUsageFlags usageFlags, const uint32_t memTypeIndex, struct HbPool* pool)
{
    VkBufferCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .usage = usageFlags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE, // queue determined by first use
        .size = BUFFER_SIZE_HOST
    };

    V_ASSERT( vkCreateBuffer(device, &ci, NULL, &pool->buffer) );
    //
    // VkMemoryRequirements reqs;
    // vkGetBufferMemoryRequirements(device, pool->buffer, &reqs);
    // we dont need to check the reqs. spec states that 
    // any buffer created without SPARSITY flags will 
    // support being bound to host visible | host coherent

    initPool(BUFFER_SIZE_HOST, memTypeIndex, &pool->pool);

    V_ASSERT( vkBindBufferMemory(device, pool->buffer, pool->pool.memory, 0) );

    V_ASSERT( vkMapMemory(device, pool->pool.memory, 0, BUFFER_SIZE_HOST, 0, (void**)&pool->hostData) );
}

static void printBufferMemoryReqs(const VkMemoryRequirements* reqs)
{
    printf("Size: %ld\tAlignment: %ld\n", reqs->size, reqs->alignment);
}

static void initBlockChain(const VkDeviceSize memorySize, const uint32_t memTypeIndex, struct BlockChain* chain)
{
    memset(chain->blocks, 0, MAX_BLOCKS * sizeof(Tanto_V_MemBlock));
    assert( memorySize % 0x40 == 0 ); // make sure memorysize is 64 byte aligned (arbitrary choice)
    chain->count = 1;
    chain->cur   = 0;
    chain->totalSize = memorySize;
    chain->blocks[0].inUse = false;
    chain->blocks[0].offset = 0;
    chain->blocks[0].size = memorySize;

    const VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memorySize,
        .memoryTypeIndex = memTypeIndex 
    };

    V_ASSERT( vkAllocateMemory(device, &allocInfo, NULL, &chain->memory) ); 
}

static const Tanto_V_MemBlock* requestBlock(const uint32_t size, const uint32_t alignment, struct BlockChain* chain)
{
    size_t cur  = chain->cur;
    const size_t init = chain->cur;
    const size_t count = chain->count;
    assert( size < chain->totalSize );
    assert( count > 0 );
    assert( cur < count );
    while (chain->blocks[cur].inUse || chain->blocks[cur].size < size)
    {
        cur = (cur + 1) % count;
        assert( cur != init ); // looped around. no blocks suitable.
    }
    // found a block not in use and with enough size
    // split the block
    size_t new = chain->count++;
    assert( new < MAX_BLOCKS );
    Tanto_V_MemBlock* curBlock = &chain->blocks[cur];
    Tanto_V_MemBlock* newBlock = &chain->blocks[new];
    assert( newBlock->inUse == false );
    VkDeviceSize alignedOffset = curBlock->offset;
    if (alignedOffset % alignment != 0) // not aligned
        alignedOffset = (alignedOffset / alignment + 1) * alignment;
    const VkDeviceSize offsetDiff = alignedOffset - curBlock->offset;
    // take away the size lost due to alignment and the new size
    newBlock->size   = curBlock->size - offsetDiff - size;
    newBlock->offset = alignedOffset + size;
    curBlock->size   = size;
    curBlock->offset = alignedOffset;
    curBlock->inUse = true;
    chain->cur = cur;
    return curBlock;
}

void tanto_v_InitMemory(void)
{
    curDevMemoryOffset = 0;

    uint32_t hostVisibleCoherentTypeIndex;
    int deviceLocalTypeIndex;

    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    V1_PRINT("Memory Heap Info:\n");
    for (int i = 0; i < memoryProperties.memoryHeapCount; i++) 
    {
        V1_PRINT("Heap %d: Size %ld: %s local\n", 
                i,
                memoryProperties.memoryHeaps[i].size, 
                memoryProperties.memoryHeaps[i].flags 
                & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT ? "Device" : "Host");
                // note there are other possible flags, but seem to only deal with multiple gpus
    }

    bool foundHvc = false;
    bool foundDl  = false;

    V1_PRINT("Memory Type Info:\n");
    for (int i = 0; i < memoryProperties.memoryTypeCount; i++) 
    {
        VkMemoryPropertyFlags flags = memoryProperties.memoryTypes[i].propertyFlags;
        V1_PRINT("Type %d: Heap Index: %d Flags: | %s%s%s%s%s%s\n", 
                i, 
                memoryProperties.memoryTypes[i].heapIndex,
                flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ?  "Device Local | " : "",
                flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ?  "Host Visible | " : "",
                flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ? "Host Coherent | " : "",
                flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT ?   "Host Cached | " : "",
                flags & VK_MEMORY_PROPERTY_PROTECTED_BIT ?     "Protected | "   : "",
                flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT ? "Lazily allocated | " : ""
                );   
        if ((flags & (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) &&
                !foundHvc)
        {
            hostVisibleCoherentTypeIndex = i;
            foundHvc = true;
        }
        if ((flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && !foundDl)
        {
            deviceLocalTypeIndex = i;
            foundDl = true;
        }
    }

    assert( foundHvc );
    assert( foundDl );

    VkBufferUsageFlags bhbFlags = 
         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | 
         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
         VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
         VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
         VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR;

    initHbPool(bhbFlags, hostVisibleCoherentTypeIndex, &hbPool);
    initBlockChain(MEMORY_SIZE_DEV_IMAGE, deviceLocalTypeIndex, &diBlockChain);
    initBlockChain(MEMORY_SIZE_DEV_BUFFER, deviceLocalTypeIndex, &dbBlockChain);
}

Tanto_V_BlockHostBuffer* tanto_v_RequestBlockHostAligned(const size_t size, const uint32_t alignment)
{
    assert( size % 4 == 0 ); // only allow for word-sized blocks
    assert( size < hbPool.pool.bytesAvailable);
    assert( hbPool.pool.count < MAX_BLOCKS );
    if (hbPool.pool.curOffset % alignment != 0)
        hbPool.pool.curOffset = (hbPool.pool.curOffset / alignment + 1) * alignment;
    Tanto_V_BlockHostBuffer* pBlock = &hbPool.blocks[hbPool.pool.count];
    pBlock->hostData = hbPool.hostData + hbPool.pool.curOffset;
    pBlock->vBuffer = &hbPool.buffer;
    pBlock->size = size;
    pBlock->vOffset = hbPool.pool.curOffset;
    pBlock->isMapped = true;

    hbPool.pool.bytesAvailable -= size;
    hbPool.pool.curOffset+= size;
    hbPool.pool.count++;
    // we really do need to be worrying about alignment here.
    // anything that is not a multiple of 4 bytes will have issues.
    // there is VERY GOOD CHANCE that there are other alignment
    // issues to consider.
    //
    // we should probably divide up the buffer into Chunks, where
    // all the blocks in a chunk contain the same kind of element
    // (a chunk for vertices, a chunk for indices, a chunk for 
    // uniform matrices, etc).

    return pBlock;
}

Tanto_V_BlockHostBuffer* tanto_v_RequestBlockHost(const size_t size, const VkBufferUsageFlags flags)
{
    uint32_t alignment;
    // the order of this if statements matters. it garuantees the maximum
    // alignment is chose if multiple flags are present
    if ( VK_BUFFER_USAGE_STORAGE_BUFFER_BIT & flags)
    {
        alignment = 0x10;
        // must satisfy alignment requirements for storage buffers
    }
    if ( VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT & flags)
    {
        // must satisfy alignment requirements for uniform buffers
        alignment = 0x40;
    }
    return tanto_v_RequestBlockHostAligned(size, alignment);
}

uint32_t tanto_v_GetMemoryType(uint32_t typeBits, const VkMemoryPropertyFlags properties)
{
    for(uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
      if(((typeBits & (1 << i)) > 0) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
      {
        return i;
      }
    }
    assert(0);
    return ~0u;
}

void tanto_v_BindImageToMemory(const VkImage image, const uint32_t size)
{
    //static bool imageBound = false;
    //assert (!imageBound);
    assert( curDevMemoryOffset < MEMORY_SIZE_DEV_IMAGE );
    vkBindImageMemory(device, image, memoryDeviceLocal, curDevMemoryOffset);
    curDevMemoryOffset += size;
}

Tanto_V_Image tanto_v_CreateImage(
        const uint32_t width, 
        const uint32_t height,
        const VkFormat format,
        const VkImageUsageFlags usageFlags,
        const VkImageAspectFlags aspectMask)
{
    assert( width * height < MEMORY_SIZE_DEV_IMAGE );

    VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {width, height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usageFlags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &graphicsQueueFamilyIndex,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    Tanto_V_Image image;

    V_ASSERT( vkCreateImage(device, &imageInfo, NULL, &image.handle) );

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device, image.handle, &memReqs);

    const Tanto_V_MemBlock* block = requestBlock(memReqs.size, memReqs.alignment, &diBlockChain);
    image.memBlock = block;

    vkBindImageMemory(device, image.handle, diBlockChain.memory, block->offset);

    VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image.handle,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .components = {0, 0, 0, 0}, // no swizzling
        .format = format,
        .subresourceRange = {
            .aspectMask = aspectMask,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    V_ASSERT( vkCreateImageView(device, &viewInfo, NULL, &image.view) );

    return image;
}

void tanto_v_CleanUpMemory()
{
    vkUnmapMemory(device, hbPool.pool.memory);
    vkDestroyBuffer(device, hbPool.buffer, NULL);
    vkFreeMemory(device, hbPool.pool.memory, NULL);
    vkFreeMemory(device, memoryDeviceLocal, NULL);
};
