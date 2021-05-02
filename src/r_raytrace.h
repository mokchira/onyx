#ifndef R_RAYTRACE_H
#define R_RAYTRACE_H

#include "v_video.h"
#include "r_render.h"
#include "r_geo.h"

typedef struct {
    VkAccelerationStructureKHR handle;
    Obdn_V_BufferRegion        bufferRegion;
} Obdn_R_AccelerationStructure;

typedef struct {
    Obdn_V_BufferRegion             bufferRegion;
    uint32_t                        groupCount;
    VkStridedDeviceAddressRegionKHR raygenTable;
    VkStridedDeviceAddressRegionKHR missTable;
    VkStridedDeviceAddressRegionKHR hitTable;
    VkStridedDeviceAddressRegionKHR callableTable;
} Obdn_R_ShaderBindingTable;

void obdn_r_BuildBlas(const Obdn_R_Primitive* prim, Obdn_R_AccelerationStructure* blas);
void obdn_r_BuildTlas(const Obdn_R_AccelerationStructure* blas, Obdn_R_AccelerationStructure* tlas);
void obdn_r_BuildTlasNew(const uint32_t count, const Obdn_R_AccelerationStructure blasses[count],
        const Coal_Mat4 xforms[count],
        Obdn_R_AccelerationStructure* tlas);
void obdn_r_CreateShaderBindingTable(const uint32_t groupCount, const VkPipeline pipeline, Obdn_R_ShaderBindingTable* sbt);
void obdn_r_DestroyAccelerationStruct(Obdn_R_AccelerationStructure* as);
void obdn_r_DestroyShaderBindingTable(Obdn_R_ShaderBindingTable* sb);

#endif /* end of include guard: R_RAYTRACE_H */