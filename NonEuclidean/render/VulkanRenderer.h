#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif
#include <volk.h>
#include "Vector.h"
#include <vector>
#include <memory>
#include <unordered_map>

class Mesh;
class Texture;
class Object;
class Portal;
struct World;
class Camera;

struct GPUInstanceInfo {
  float model[16];
  float normalMat[16];
  float emissive[4];       // xyz=emissive, w=unused
  uint32_t indices[4];     // mesh, isPortal, texIdx, portalWarpIndex
};
static_assert(sizeof(GPUInstanceInfo) == 160, "GPUInstanceInfo layout mismatch");

struct GPUPortalWarpInfo {
  float frontWarp[16];    // this portal front face -> connected portal (deltaInv)
  float frontReverse[16]; // connected portal -> this portal front face (delta)
  float backWarp[16];     // this portal back face -> connected portal (deltaInv)
  float backReverse[16];  // connected portal -> this portal back face (delta)
  float center[4];        // precomputed world-space center xyz, w=unused
  float forward[4];       // precomputed world-space forward dir xyz, w=unused
  float axisX[4];         // precomputed world-space X axis xyz (includes scale)
  float axisY[4];         // precomputed world-space Y axis xyz (includes scale)
  float aabbMin[4];       // influence volume min xyz, w=unused
  float aabbMax[4];       // influence volume max xyz, w=unused
};
static_assert(sizeof(GPUPortalWarpInfo) == 352, "GPUPortalWarpInfo layout mismatch");

class VulkanRenderer {
public:
  VulkanRenderer();
  ~VulkanRenderer();

  bool Init(HWND hwnd, HINSTANCE hInstance, int width, int height);
  void Shutdown();
  void OnResize(int width, int height);
  void UploadScene(const World& world);
  void Render(const Camera& cam, const World& world);
  void ReloadShaders();

private:
  // Vulkan core
  VkInstance instance = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkPhysicalDevice physDevice = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  VkQueue queue = VK_NULL_HANDLE;
  uint32_t queueFamily = 0;

  // Swapchain
  VkSwapchainKHR swapchain = VK_NULL_HANDLE;
  VkFormat swapFormat = VK_FORMAT_B8G8R8A8_SRGB;
  VkExtent2D swapExtent = {};
  std::vector<VkImage> swapImages;
  std::vector<VkImageView> swapImageViews;

  // Command
  VkCommandPool cmdPool = VK_NULL_HANDLE;
  VkCommandBuffer cmdBuf = VK_NULL_HANDLE;

  // Sync
  VkFence fence = VK_NULL_HANDLE;
  VkSemaphore semImageAvail = VK_NULL_HANDLE;
  VkSemaphore semRenderDone = VK_NULL_HANDLE;

  // Storage image (RT output)
  VkImage storageImage = VK_NULL_HANDLE;
  VkImageView storageView = VK_NULL_HANDLE;
  VkDeviceMemory storageMem = VK_NULL_HANDLE;

  // Descriptors
  VkDescriptorSetLayout descLayout = VK_NULL_HANDLE;
  VkDescriptorPool descPool = VK_NULL_HANDLE;
  VkDescriptorSet descSet = VK_NULL_HANDLE;

  // Pipeline
  VkPipelineLayout pipeLayout = VK_NULL_HANDLE;
  VkPipeline pipeline = VK_NULL_HANDLE;

  // SBT
  VkBuffer sbtBuffer = VK_NULL_HANDLE;
  VkDeviceMemory sbtMem = VK_NULL_HANDLE;
  VkStridedDeviceAddressRegionKHR rgenRegion = {};
  VkStridedDeviceAddressRegionKHR missRegion = {};
  VkStridedDeviceAddressRegionKHR hitRegion = {};
  VkStridedDeviceAddressRegionKHR callRegion = {};

  // UBO
  VkBuffer uboBuffer = VK_NULL_HANDLE;
  VkDeviceMemory uboMem = VK_NULL_HANDLE;
  void* uboMapped = nullptr;

  // Scene GPU data
  struct MeshGPU {
    VkAccelerationStructureKHR blas = VK_NULL_HANDLE;
    VkBuffer blasBuf = VK_NULL_HANDLE;
    VkDeviceMemory blasMem = VK_NULL_HANDLE;
    uint32_t vertexOffset = 0; // offset into global vertex arrays
    uint32_t vertexCount = 0;
  };
  std::vector<MeshGPU> meshGPUs;
  std::unordered_map<Mesh*, uint32_t> meshIndexMap;

  VkBuffer posBuffer = VK_NULL_HANDLE; VkDeviceMemory posMem = VK_NULL_HANDLE;
  VkBuffer nrmBuffer = VK_NULL_HANDLE; VkDeviceMemory nrmMem = VK_NULL_HANDLE;
  VkBuffer uvBuffer = VK_NULL_HANDLE;  VkDeviceMemory uvMem = VK_NULL_HANDLE;
  VkBuffer meshOffBuffer = VK_NULL_HANDLE; VkDeviceMemory meshOffMem = VK_NULL_HANDLE;
  VkBuffer instanceBuffer = VK_NULL_HANDLE; VkDeviceMemory instanceMem = VK_NULL_HANDLE;
  VkBuffer portalWarpBuffer = VK_NULL_HANDLE; VkDeviceMemory portalWarpMem = VK_NULL_HANDLE;
  VkBuffer portalIndexBuffer = VK_NULL_HANDLE; VkDeviceMemory portalIndexMem = VK_NULL_HANDLE;

  struct TexGPU {
    VkImage image = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkDeviceMemory mem = VK_NULL_HANDLE;
  };
  std::vector<TexGPU> texGPUs;
  std::unordered_map<Texture*, uint32_t> texIndexMap;
  VkSampler sampler = VK_NULL_HANDLE;

  VkAccelerationStructureKHR tlas = VK_NULL_HANDLE;
  VkBuffer tlasBuf = VK_NULL_HANDLE; VkDeviceMemory tlasMem = VK_NULL_HANDLE;
  VkBuffer tlasInstanceBuf = VK_NULL_HANDLE; VkDeviceMemory tlasInstMem = VK_NULL_HANDLE;

  int width = 0, height = 0;
  bool sceneUploaded = false;

  // Helpers
  uint32_t FindMemoryType(uint32_t filter, VkMemoryPropertyFlags props);
  void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags props, VkBuffer& buf, VkDeviceMemory& mem);
  void CreateImage(uint32_t w, uint32_t h, VkFormat fmt, VkImageUsageFlags usage,
                   VkImage& img, VkDeviceMemory& mem);
  VkImageView CreateImageView(VkImage img, VkFormat fmt);
  VkCommandBuffer BeginCmd();
  void EndCmd(VkCommandBuffer cmd);
  void TransitionImage(VkCommandBuffer cmd, VkImage img,
                       VkImageLayout oldL, VkImageLayout newL);
  VkShaderModule CreateShaderModule(const std::vector<uint32_t>& spirv);
  std::vector<uint32_t> CompileGLSL(const char* filename, int stage);
  VkDeviceAddress GetBufferAddress(VkBuffer buf);

  void CreateSwapchain();
  void DestroySwapchain();
  void CreateStorageImage();
  void DestroyStorageImage();
  void CreateDescriptorLayout();
  void CreatePipeline();
  void CreateSBT();
  void UpdateDescriptorSet();

  void DestroyScene();
};
