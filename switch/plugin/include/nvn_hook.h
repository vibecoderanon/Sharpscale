#ifndef SHARPSCALE_NVN_HOOK_H
#define SHARPSCALE_NVN_HOOK_H

#include <stdint.h>
#include <stdbool.h>
#include "sharpscale_nx.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Hook signatures for NVN (Nvidia Graphics API for Switch)
 */
typedef void* (*PFN_nvnDeviceGetProcAddress)(void* device, const char* name);
typedef void (*PFN_nvnQueuePresentTexture)(void* queue, void* window, int texture_idx);
typedef void (*PFN_nvnWindowAcquireTexture)(void* window, void* fence, int* texture_idx);
typedef void (*PFN_nvnWindowBuilderSetTextures)(void* builder, int numTextures, void** textures);
typedef uint16_t (*PFN_nvnTextureGetWidth)(const void* texture);
typedef uint16_t (*PFN_nvnTextureGetHeight)(const void* texture);
typedef void (*PFN_nvnCommandBufferSetViewport)(void* cmdBuf, int x, int y, int width, int height);
typedef void (*PFN_nvnCommandBufferSetViewports)(void* cmdBuf, int start, int count, const void* viewports);
typedef void (*PFN_nvnCommandBufferSetScissor)(void* cmdBuf, int x, int y, int width, int height);
typedef void (*PFN_nvnCommandBufferSetScissors)(void* cmdBuf, int start, int count, const void* scissors);

typedef struct {
    float x;
    float y;
    float width;
    float height;
} NVNviewport;

typedef struct {
    int x;
    int y;
    int width;
    int height;
} NVNscissor;

/**
 * Function pointers and state for NVN hooks
 */
typedef struct {
    bool is_installed;
    void* nvn_device;
    void* nvn_queue;
    void* nvn_window;
    PFN_nvnDeviceGetProcAddress orig_nvnDeviceGetProcAddress;
    PFN_nvnQueuePresentTexture orig_nvnQueuePresentTexture;
    PFN_nvnWindowAcquireTexture orig_nvnWindowAcquireTexture;
    PFN_nvnWindowBuilderSetTextures orig_nvnWindowBuilderSetTextures;
    PFN_nvnTextureGetWidth orig_nvnTextureGetWidth;
    PFN_nvnTextureGetHeight orig_nvnTextureGetHeight;
    PFN_nvnCommandBufferSetViewport orig_nvnCommandBufferSetViewport;
    PFN_nvnCommandBufferSetViewports orig_nvnCommandBufferSetViewports;
    PFN_nvnCommandBufferSetScissor orig_nvnCommandBufferSetScissor;
    PFN_nvnCommandBufferSetScissors orig_nvnCommandBufferSetScissors;
} NvnHookState;

bool nvn_hook_init(void);
void nvn_hook_exit(void);
void* nvn_hook_get_proc_address(void* device, const char* name);
void nvn_hook_queue_present_texture(void* queue, void* window, int texture_idx);
void nvn_hook_apply_scaling(void* window, uint32_t src_w, uint32_t src_h);

#ifdef __cplusplus
}
#endif

#endif /* SHARPSCALE_NVN_HOOK_H */
