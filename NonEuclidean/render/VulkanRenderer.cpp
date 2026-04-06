#include "VulkanRenderer.h"
#include "Camera.h"
#include "world.h"
#include "Object.h"
#include "Portal.h"
#include "Mesh.h"
#include "Texture.h"
#include "GameHeader.h"
#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>
#include <fstream>
#include <sstream>
#include <cassert>
#include <algorithm>
#include <iostream>
#include "stb_image.h"

// ============================================================
// Helpers
// ============================================================

uint32_t VulkanRenderer::FindMemoryType(uint32_t filter, VkMemoryPropertyFlags props) {
  VkPhysicalDeviceMemoryProperties memProps;
  vkGetPhysicalDeviceMemoryProperties(physDevice, &memProps);
  for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
    if ((filter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & props) == props)
      return i;
  }
  assert(false && "Failed to find memory type");
  return 0;
}

void VulkanRenderer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                  VkMemoryPropertyFlags props, VkBuffer& buf, VkDeviceMemory& mem) {
  VkBufferCreateInfo ci = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  ci.size = size;
  ci.usage = usage;
  ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  vkCreateBuffer(device, &ci, nullptr, &buf);

  VkMemoryRequirements req;
  vkGetBufferMemoryRequirements(device, buf, &req);
  VkMemoryAllocateInfo ai = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  ai.allocationSize = req.size;
  ai.memoryTypeIndex = FindMemoryType(req.memoryTypeBits, props);

  VkMemoryAllocateFlagsInfo flagsInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
  if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
    flagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
    ai.pNext = &flagsInfo;
  }
  vkAllocateMemory(device, &ai, nullptr, &mem);
  vkBindBufferMemory(device, buf, mem, 0);
}

void VulkanRenderer::CreateImage(uint32_t w, uint32_t h, VkFormat fmt,
                                 VkImageUsageFlags usage, VkImage& img, VkDeviceMemory& mem) {
  VkImageCreateInfo ci = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  ci.imageType = VK_IMAGE_TYPE_2D;
  ci.format = fmt;
  ci.extent = {w, h, 1};
  ci.mipLevels = 1;
  ci.arrayLayers = 1;
  ci.samples = VK_SAMPLE_COUNT_1_BIT;
  ci.tiling = VK_IMAGE_TILING_OPTIMAL;
  ci.usage = usage;
  ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  vkCreateImage(device, &ci, nullptr, &img);

  VkMemoryRequirements req;
  vkGetImageMemoryRequirements(device, img, &req);
  VkMemoryAllocateInfo ai = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  ai.allocationSize = req.size;
  ai.memoryTypeIndex = FindMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  vkAllocateMemory(device, &ai, nullptr, &mem);
  vkBindImageMemory(device, img, mem, 0);
}

VkImageView VulkanRenderer::CreateImageView(VkImage img, VkFormat fmt) {
  VkImageViewCreateInfo ci = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  ci.image = img;
  ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
  ci.format = fmt;
  ci.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  VkImageView view;
  vkCreateImageView(device, &ci, nullptr, &view);
  return view;
}

VkCommandBuffer VulkanRenderer::BeginCmd() {
  VkCommandBufferAllocateInfo ai = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  ai.commandPool = cmdPool;
  ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  ai.commandBufferCount = 1;
  VkCommandBuffer cmd;
  vkAllocateCommandBuffers(device, &ai, &cmd);
  VkCommandBufferBeginInfo bi = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cmd, &bi);
  return cmd;
}

void VulkanRenderer::EndCmd(VkCommandBuffer cmd) {
  vkEndCommandBuffer(cmd);
  VkSubmitInfo si = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
  si.commandBufferCount = 1;
  si.pCommandBuffers = &cmd;
  vkQueueSubmit(queue, 1, &si, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);
  vkFreeCommandBuffers(device, cmdPool, 1, &cmd);
}

void VulkanRenderer::TransitionImage(VkCommandBuffer cmd, VkImage img,
                                     VkImageLayout oldL, VkImageLayout newL) {
  VkImageMemoryBarrier b = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
  b.oldLayout = oldL;
  b.newLayout = newL;
  b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  b.image = img;
  b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  b.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
  b.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                       VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &b);
}

VkDeviceAddress VulkanRenderer::GetBufferAddress(VkBuffer buf) {
  VkBufferDeviceAddressInfo info = {VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
  info.buffer = buf;
  return vkGetBufferDeviceAddress(device, &info);
}

// ============================================================
// GLSL -> SPIR-V compilation via glslang
// ============================================================
std::vector<uint32_t> VulkanRenderer::CompileGLSL(const char* filename, int stage) {
  std::ifstream f(std::string("RTShaders/") + filename);
  std::stringstream ss;
  ss << f.rdbuf();
  std::string src = ss.str();
  const char* srcPtr = src.c_str();

  EShLanguage lang = (EShLanguage)stage;
  glslang::TShader shader(lang);
  shader.setStrings(&srcPtr, 1);
  shader.setEnvInput(glslang::EShSourceGlsl, lang, glslang::EShClientVulkan, 460);
  shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
  shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_4);

  if (!shader.parse(GetDefaultResources(), 460, ECoreProfile, false, false, EShMsgDefault)) {
    OutputDebugStringA("Shader compile error: ");
    OutputDebugStringA(filename);
    OutputDebugStringA("\n");
    OutputDebugStringA(shader.getInfoLog());
    MessageBoxA(nullptr, shader.getInfoLog(), filename, MB_OK);
  }
  glslang::TProgram program;
  program.addShader(&shader);
  if (!program.link(EShMsgDefault)) {
    OutputDebugStringA("Shader link error: ");
    OutputDebugStringA(filename);
    OutputDebugStringA("\n");
    OutputDebugStringA(program.getInfoLog());
    MessageBoxA(nullptr, program.getInfoLog(), filename, MB_OK);
  }
  std::vector<uint32_t> spirv;
  glslang::GlslangToSpv(*program.getIntermediate(lang), spirv);
  return spirv;
}

VkShaderModule VulkanRenderer::CreateShaderModule(const std::vector<uint32_t>& spirv) {
  VkShaderModuleCreateInfo ci = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  ci.codeSize = spirv.size() * sizeof(uint32_t);
  ci.pCode = spirv.data();
  VkShaderModule mod;
  vkCreateShaderModule(device, &ci, nullptr, &mod);
  return mod;
}

// ============================================================
// Constructor / Destructor
// ============================================================
VulkanRenderer::VulkanRenderer() {}
VulkanRenderer::~VulkanRenderer() { Shutdown(); }

// ============================================================
// Init
// ============================================================
bool VulkanRenderer::Init(HWND hwnd, HINSTANCE hInstance, int w, int h) {
  width = w; height = h;
  if (volkInitialize() != VK_SUCCESS) return false;

  // Instance
  {
    auto hasInstanceLayer = [](const char* layerName) {
      uint32_t count = 0;
      vkEnumerateInstanceLayerProperties(&count, nullptr);
      std::vector<VkLayerProperties> layers(count);
      if (count > 0) {
        vkEnumerateInstanceLayerProperties(&count, layers.data());
      }
      for (const auto& layer : layers) {
        if (strcmp(layer.layerName, layerName) == 0) {
          return true;
        }
      }
      return false;
    };

    VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.pApplicationName = "NonEuclidean";
    appInfo.apiVersion = VK_API_VERSION_1_2;

    std::vector<const char*> exts = {
      VK_KHR_SURFACE_EXTENSION_NAME,
      VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    };
    std::vector<const char*> layers;
#ifdef _DEBUG
    if (hasInstanceLayer("VK_LAYER_KHRONOS_validation")) {
      layers.push_back("VK_LAYER_KHRONOS_validation");
    } else {
      OutputDebugStringA("VK_LAYER_KHRONOS_validation not available; continuing without validation layers.\n");
    }
#endif

    VkInstanceCreateInfo ci = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ci.pApplicationInfo = &appInfo;
    ci.enabledExtensionCount = static_cast<uint32_t>(exts.size());
    ci.ppEnabledExtensionNames = exts.data();
    ci.enabledLayerCount = static_cast<uint32_t>(layers.size());
    ci.ppEnabledLayerNames = layers.empty() ? nullptr : layers.data();

    VkResult createResult = vkCreateInstance(&ci, nullptr, &instance);
#ifdef _DEBUG
    if (createResult == VK_ERROR_LAYER_NOT_PRESENT && !layers.empty()) {
      OutputDebugStringA("Debug Vulkan instance creation failed due to missing layers; retrying without validation layers.\n");
      layers.clear();
      ci.enabledLayerCount = 0;
      ci.ppEnabledLayerNames = nullptr;
      createResult = vkCreateInstance(&ci, nullptr, &instance);
    }
#endif
    if (createResult != VK_SUCCESS) return false;
    volkLoadInstance(instance);
  }

  // Surface
  {
    VkWin32SurfaceCreateInfoKHR ci = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    ci.hinstance = hInstance;
    ci.hwnd = hwnd;
    if (vkCreateWin32SurfaceKHR(instance, &ci, nullptr, &surface) != VK_SUCCESS) return false;
  }

  // Physical device
  {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    std::vector<VkPhysicalDevice> devs(count);
    vkEnumeratePhysicalDevices(instance, &count, devs.data());

    for (auto& pd : devs) {
      // Check for RT support
      uint32_t extCount = 0;
      vkEnumerateDeviceExtensionProperties(pd, nullptr, &extCount, nullptr);
      std::vector<VkExtensionProperties> available(extCount);
      vkEnumerateDeviceExtensionProperties(pd, nullptr, &extCount, available.data());

      bool hasAS = false, hasRT = false, hasDeferred = false;
      for (auto& e : available) {
        if (strcmp(e.extensionName, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) == 0) hasAS = true;
        if (strcmp(e.extensionName, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) == 0) hasRT = true;
        if (strcmp(e.extensionName, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME) == 0) hasDeferred = true;
      }
      if (hasAS && hasRT && hasDeferred) {
        physDevice = pd;
        break;
      }
    }
    if (physDevice == VK_NULL_HANDLE) {
      MessageBoxA(nullptr, "No GPU with Vulkan Ray Tracing support found.", "Error", MB_OK);
      return false;
    }
  }

  // Queue family
  {
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &count, families.data());
    for (uint32_t i = 0; i < count; i++) {
      VkBool32 present = VK_FALSE;
      vkGetPhysicalDeviceSurfaceSupportKHR(physDevice, i, surface, &present);
      if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present) {
        queueFamily = i;
        break;
      }
    }
  }

  // Logical device
  {
    float priority = 1.0f;
    VkDeviceQueueCreateInfo qci = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    qci.queueFamilyIndex = queueFamily;
    qci.queueCount = 1;
    qci.pQueuePriorities = &priority;

    const char* devExts[] = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
      VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
      VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
      VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
    };

    // Feature chain
    VkPhysicalDeviceVulkan12Features features12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.bufferDeviceAddress = VK_TRUE;
    features12.descriptorIndexing = VK_TRUE;
    features12.runtimeDescriptorArray = VK_TRUE;
    features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    features12.descriptorBindingPartiallyBound = VK_TRUE;
    features12.descriptorBindingVariableDescriptorCount = VK_TRUE;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR asFeatures =
      {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
    asFeatures.accelerationStructure = VK_TRUE;
    asFeatures.pNext = &features12;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtFeatures =
      {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
    rtFeatures.rayTracingPipeline = VK_TRUE;
    rtFeatures.pNext = &asFeatures;

    VkPhysicalDeviceFeatures2 features2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    features2.pNext = &rtFeatures;

    VkDeviceCreateInfo dci = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    dci.enabledExtensionCount = sizeof(devExts) / sizeof(devExts[0]);
    dci.ppEnabledExtensionNames = devExts;
    dci.pNext = &features2;

    if (vkCreateDevice(physDevice, &dci, nullptr, &device) != VK_SUCCESS) return false;
    volkLoadDevice(device);
    vkGetDeviceQueue(device, queueFamily, 0, &queue);
  }

  // Command pool
  {
    VkCommandPoolCreateInfo ci = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    ci.queueFamilyIndex = queueFamily;
    vkCreateCommandPool(device, &ci, nullptr, &cmdPool);

    VkCommandBufferAllocateInfo ai = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    ai.commandPool = cmdPool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 1;
    vkAllocateCommandBuffers(device, &ai, &cmdBuf);
  }

  // Sync
  {
    VkFenceCreateInfo fci = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(device, &fci, nullptr, &fence);
    VkSemaphoreCreateInfo sci = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    vkCreateSemaphore(device, &sci, nullptr, &semImageAvail);
    vkCreateSemaphore(device, &sci, nullptr, &semRenderDone);
  }

  CreateSwapchain();
  CreateStorageImage();

  // UBO
  CreateBuffer(256, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               uboBuffer, uboMem);
  vkMapMemory(device, uboMem, 0, 512, 0, &uboMapped);

  // Sampler
  {
    VkSamplerCreateInfo ci = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    ci.magFilter = VK_FILTER_LINEAR;
    ci.minFilter = VK_FILTER_LINEAR;
    ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    ci.maxAnisotropy = 16.0f;
    ci.anisotropyEnable = VK_TRUE;
    ci.maxLod = 1.0f;
    vkCreateSampler(device, &ci, nullptr, &sampler);
  }

  // Init glslang
  glslang::InitializeProcess();

  CreateDescriptorLayout();
  CreatePipeline();
  CreateSBT();

  return true;
}

// ============================================================
// Swapchain
// ============================================================
void VulkanRenderer::CreateSwapchain() {
  VkSurfaceCapabilitiesKHR caps;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &caps);

  swapExtent = caps.currentExtent;
  if (swapExtent.width == UINT32_MAX) {
    swapExtent = {(uint32_t)width, (uint32_t)height};
  }

  uint32_t fmtCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &fmtCount, nullptr);
  std::vector<VkSurfaceFormatKHR> fmts(fmtCount);
  vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &fmtCount, fmts.data());
  swapFormat = fmts[0].format;
  VkColorSpaceKHR colorSpace = fmts[0].colorSpace;
  for (auto& f : fmts) {
    if (f.format == VK_FORMAT_B8G8R8A8_UNORM) {
      swapFormat = f.format; colorSpace = f.colorSpace; break;
    }
  }

  VkSwapchainCreateInfoKHR ci = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
  ci.surface = surface;
  ci.minImageCount = std::max(caps.minImageCount + 1,
                              caps.maxImageCount > 0 ? std::min(caps.minImageCount + 1, caps.maxImageCount) : caps.minImageCount + 1);
  ci.imageFormat = swapFormat;
  ci.imageColorSpace = colorSpace;
  ci.imageExtent = swapExtent;
  ci.imageArrayLayers = 1;
  ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  ci.preTransform = caps.currentTransform;
  ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  ci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  ci.clipped = VK_TRUE;
  vkCreateSwapchainKHR(device, &ci, nullptr, &swapchain);

  uint32_t imgCount = 0;
  vkGetSwapchainImagesKHR(device, swapchain, &imgCount, nullptr);
  swapImages.resize(imgCount);
  vkGetSwapchainImagesKHR(device, swapchain, &imgCount, swapImages.data());
  swapImageViews.resize(imgCount);
  for (uint32_t i = 0; i < imgCount; i++) {
    swapImageViews[i] = CreateImageView(swapImages[i], swapFormat);
  }
}

void VulkanRenderer::DestroySwapchain() {
  for (auto v : swapImageViews) vkDestroyImageView(device, v, nullptr);
  swapImageViews.clear();
  swapImages.clear();
  if (swapchain) { vkDestroySwapchainKHR(device, swapchain, nullptr); swapchain = VK_NULL_HANDLE; }
}

// ============================================================
// Storage image
// ============================================================
void VulkanRenderer::CreateStorageImage() {
  CreateImage(swapExtent.width * GH_SSAA_FACTOR, swapExtent.height * GH_SSAA_FACTOR, VK_FORMAT_R8G8B8A8_UNORM,
              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
              storageImage, storageMem);
  storageView = CreateImageView(storageImage, VK_FORMAT_R8G8B8A8_UNORM);

  auto cmd = BeginCmd();
  TransitionImage(cmd, storageImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
  EndCmd(cmd);
}

void VulkanRenderer::DestroyStorageImage() {
  if (storageView) { vkDestroyImageView(device, storageView, nullptr); storageView = VK_NULL_HANDLE; }
  if (storageImage) { vkDestroyImage(device, storageImage, nullptr); storageImage = VK_NULL_HANDLE; }
  if (storageMem) { vkFreeMemory(device, storageMem, nullptr); storageMem = VK_NULL_HANDLE; }
}

// ============================================================
// Descriptors
// ============================================================
void VulkanRenderer::CreateDescriptorLayout() {
  VkDescriptorSetLayoutBinding bindings[14] = {};
  // 0: TLAS
  bindings[0].binding = 0;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
  // 1: Storage image
  bindings[1].binding = 1;
  bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  bindings[1].descriptorCount = 1;
  bindings[1].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
  // 2: UBO
  bindings[2].binding = 2;
  bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[2].descriptorCount = 1;
  bindings[2].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;
  // 3-7: Storage buffers (instances, positions, normals, uvs, meshOffsets)
  for (int i = 3; i <= 7; i++) {
    bindings[i].binding = i;
    bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[i].descriptorCount = 1;
    bindings[i].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
  }
  // 8: Portal warp buffer
  bindings[8].binding = 8;
  bindings[8].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[8].descriptorCount = 1;
  bindings[8].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
  // 9: Portal index buffer
  bindings[9].binding = 9;
  bindings[9].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[9].descriptorCount = 1;
  bindings[9].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
  // 10: Texture array (partially bound, fixed 128 max)
  bindings[10].binding = 10;
  bindings[10].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[10].descriptorCount = 128; // max
  bindings[10].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
  // 11: Vertex colors buffer
  bindings[11].binding = 11;
  bindings[11].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[11].descriptorCount = 1;
  bindings[11].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
  // 12: Per-vertex texture index buffer
  bindings[12].binding = 12;
  bindings[12].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[12].descriptorCount = 1;
  bindings[12].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
  // 13: Lights SSBO
  bindings[13].binding = 13;
  bindings[13].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[13].descriptorCount = 1;
  bindings[13].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

  VkDescriptorBindingFlags bindFlags[14] = {};
  bindFlags[10] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

  VkDescriptorSetLayoutBindingFlagsCreateInfo flagsCI =
    {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO};
  flagsCI.bindingCount = 14;
  flagsCI.pBindingFlags = bindFlags;

  VkDescriptorSetLayoutCreateInfo ci = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  ci.bindingCount = 14;
  ci.pBindings = bindings;
  ci.pNext = &flagsCI;
  vkCreateDescriptorSetLayout(device, &ci, nullptr, &descLayout);
}

// ============================================================
// Pipeline
// ============================================================
void VulkanRenderer::ReloadShaders() {
  if (!device) return;
  vkDeviceWaitIdle(device);

  // Tear down SBT and pipeline, then rebuild
  if (sbtBuffer) { vkDestroyBuffer(device, sbtBuffer, nullptr); sbtBuffer = VK_NULL_HANDLE; }
  if (sbtMem)    { vkFreeMemory(device, sbtMem, nullptr);       sbtMem    = VK_NULL_HANDLE; }
  if (pipeline)  { vkDestroyPipeline(device, pipeline, nullptr);             pipeline  = VK_NULL_HANDLE; }
  if (pipeLayout){ vkDestroyPipelineLayout(device, pipeLayout, nullptr);     pipeLayout = VK_NULL_HANDLE; }

  std::cout << "[ReloadShaders] recompiling..." << std::flush;
  CreatePipeline();
  CreateSBT();
  std::cout << " done." << std::endl;
}

void VulkanRenderer::CreatePipeline() {
  VkPipelineLayoutCreateInfo plci = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  plci.setLayoutCount = 1;
  plci.pSetLayouts = &descLayout;
  vkCreatePipelineLayout(device, &plci, nullptr, &pipeLayout);

  // Compile shaders
  auto rgenSpv = CompileGLSL("raygen.rgen", EShLangRayGen);
  auto chitSpv = CompileGLSL("closesthit.rchit", EShLangClosestHit);
  auto portSpv = CompileGLSL("portalhit.rchit", EShLangClosestHit);
  auto shitSpv = CompileGLSL("shadowhit.rchit", EShLangClosestHit);
  auto pshitSpv = CompileGLSL("portalshadow.rchit", EShLangClosestHit);
  auto missSpv = CompileGLSL("miss.rmiss", EShLangMiss);
  auto smisSpv = CompileGLSL("shadowmiss.rmiss", EShLangMiss);

  VkShaderModule rgenMod = CreateShaderModule(rgenSpv);
  VkShaderModule chitMod = CreateShaderModule(chitSpv);
  VkShaderModule portMod = CreateShaderModule(portSpv);
  VkShaderModule shitMod = CreateShaderModule(shitSpv);
  VkShaderModule pshitMod = CreateShaderModule(pshitSpv);
  VkShaderModule missMod = CreateShaderModule(missSpv);
  VkShaderModule smisMod = CreateShaderModule(smisSpv);

  // Shader stages: 0=raygen, 1=closesthit, 2=portalhit, 3=shadowhit, 4=portalshadow, 5=miss, 6=shadowmiss
  VkPipelineShaderStageCreateInfo stages[7] = {};
  for (auto& s : stages) s.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[0].stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;       stages[0].module = rgenMod;  stages[0].pName = "main";
  stages[1].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;  stages[1].module = chitMod;  stages[1].pName = "main";
  stages[2].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;  stages[2].module = portMod;  stages[2].pName = "main";
  stages[3].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;  stages[3].module = shitMod;  stages[3].pName = "main";
  stages[4].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;  stages[4].module = pshitMod; stages[4].pName = "main";
  stages[5].stage = VK_SHADER_STAGE_MISS_BIT_KHR;         stages[5].module = missMod;  stages[5].pName = "main";
  stages[6].stage = VK_SHADER_STAGE_MISS_BIT_KHR;         stages[6].module = smisMod;  stages[6].pName = "main";

  // Groups: raygen(0), sky_miss(1), shadow_miss(2),
  //   closesthit(3), shadowhit(4), portalhit(5), portalshadow(6)
  // Hit groups are paired: stride=2 so primary=offset0, shadow=offset1
  VkRayTracingShaderGroupCreateInfoKHR groups[7] = {};
  for (auto& g : groups) {
    g.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    g.generalShader = VK_SHADER_UNUSED_KHR;
    g.closestHitShader = VK_SHADER_UNUSED_KHR;
    g.anyHitShader = VK_SHADER_UNUSED_KHR;
    g.intersectionShader = VK_SHADER_UNUSED_KHR;
  }
  // Raygen
  groups[0].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
  groups[0].generalShader = 0;
  // Miss (sky)
  groups[1].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
  groups[1].generalShader = 5;
  // Shadow miss
  groups[2].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
  groups[2].generalShader = 6;
  // Closest hit (material) — primary, sbtOffset 0
  groups[3].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
  groups[3].closestHitShader = 1;
  // Shadow hit (material) — shadow, sbtOffset 0+1
  groups[4].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
  groups[4].closestHitShader = 3;
  // Portal hit — primary, sbtOffset 2
  groups[5].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
  groups[5].closestHitShader = 2;
  // Portal shadow — shadow, sbtOffset 2+1
  groups[6].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
  groups[6].closestHitShader = 4;

  VkRayTracingPipelineCreateInfoKHR ci = {VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR};
  ci.stageCount = 7;
  ci.pStages = stages;
  ci.groupCount = 7;
  ci.pGroups = groups;
  ci.maxPipelineRayRecursionDepth = GH_MAX_RECURSION + 1; // +1 for shadow rays
  ci.layout = pipeLayout;
  vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &ci, nullptr, &pipeline);

  vkDestroyShaderModule(device, rgenMod, nullptr);
  vkDestroyShaderModule(device, chitMod, nullptr);
  vkDestroyShaderModule(device, portMod, nullptr);
  vkDestroyShaderModule(device, shitMod, nullptr);
  vkDestroyShaderModule(device, pshitMod, nullptr);
  vkDestroyShaderModule(device, missMod, nullptr);
  vkDestroyShaderModule(device, smisMod, nullptr);
}

// ============================================================
// Shader Binding Table
// ============================================================
void VulkanRenderer::CreateSBT() {
  VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProps =
    {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
  VkPhysicalDeviceProperties2 props2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
  props2.pNext = &rtProps;
  vkGetPhysicalDeviceProperties2(physDevice, &props2);

  const uint32_t handleSize = rtProps.shaderGroupHandleSize;
  const uint32_t handleAlign = rtProps.shaderGroupHandleAlignment;
  const uint32_t baseAlign = rtProps.shaderGroupBaseAlignment;
  const uint32_t handleStride = (handleSize + handleAlign - 1) & ~(handleAlign - 1);

  // 7 groups: raygen(1) + miss(2) + hit(4)
  uint32_t totalHandles = 7;
  std::vector<uint8_t> handles(totalHandles * handleSize);
  vkGetRayTracingShaderGroupHandlesKHR(device, pipeline, 0, totalHandles, handles.size(), handles.data());

  // SBT regions
  auto alignUp = [](uint32_t v, uint32_t a) { return (v + a - 1) & ~(a - 1); };
  uint32_t rgenSize = alignUp(handleStride, baseAlign);
  uint32_t missSize = alignUp(2 * handleStride, baseAlign);
  uint32_t hitSize  = alignUp(4 * handleStride, baseAlign);

  CreateBuffer(rgenSize + missSize + hitSize,
               VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               sbtBuffer, sbtMem);

  uint8_t* mapped = nullptr;
  vkMapMemory(device, sbtMem, 0, VK_WHOLE_SIZE, 0, (void**)&mapped);
  memset(mapped, 0, rgenSize + missSize + hitSize);

  // Group 0 = raygen
  memcpy(mapped, handles.data() + 0 * handleSize, handleSize);
  // Group 1,2 = miss
  memcpy(mapped + rgenSize + 0 * handleStride, handles.data() + 1 * handleSize, handleSize);
  memcpy(mapped + rgenSize + 1 * handleStride, handles.data() + 2 * handleSize, handleSize);
  // Group 3,4,5,6 = hit (closesthit, shadowhit, portalhit, portalshadow)
  memcpy(mapped + rgenSize + missSize + 0 * handleStride, handles.data() + 3 * handleSize, handleSize);
  memcpy(mapped + rgenSize + missSize + 1 * handleStride, handles.data() + 4 * handleSize, handleSize);
  memcpy(mapped + rgenSize + missSize + 2 * handleStride, handles.data() + 5 * handleSize, handleSize);
  memcpy(mapped + rgenSize + missSize + 3 * handleStride, handles.data() + 6 * handleSize, handleSize);

  vkUnmapMemory(device, sbtMem);

  VkDeviceAddress base = GetBufferAddress(sbtBuffer);
  rgenRegion = {base, rgenSize, rgenSize};  // raygen stride must equal size
  missRegion = {base + rgenSize, handleStride, missSize};
  hitRegion  = {base + rgenSize + missSize, handleStride, hitSize};
  callRegion = {0, 0, 0};
}

// ============================================================
// Scene upload
// ============================================================
void VulkanRenderer::UploadScene(const World& world) {
  DestroyScene();

  // Collect unique meshes and textures
  std::vector<float> allPos, allNrm, allUV, allCol;
  std::vector<uint32_t> allTexIdx; // per-vertex global texture index (0 = no per-vertex tex)
  std::vector<uint32_t> meshOffsets;
  uint32_t vertexOffset = 0;

  auto ensureTex = [&](Texture* t) -> uint32_t {
    if (!t) return 0;
    auto it = texIndexMap.find(t);
    if (it != texIndexMap.end()) return it->second;
    uint32_t idx = (uint32_t)texGPUs.size();
    texIndexMap[t] = idx;

    TexGPU tg;
    // Create VkImage from texture CPU data
    CreateImage(t->width, t->height, VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                tg.image, tg.mem);
    tg.view = CreateImageView(tg.image, VK_FORMAT_R8G8B8A8_UNORM);

    // Upload via staging buffer
    VkDeviceSize imgSize = t->width * t->height * 4;
    VkBuffer staging; VkDeviceMemory stagingMem;
    CreateBuffer(imgSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 staging, stagingMem);
    void* data;
    vkMapMemory(device, stagingMem, 0, imgSize, 0, &data);
    // Convert to RGBA
    uint8_t* dst = (uint8_t*)data;
    if (t->isRGBA) {
      // Already RGBA, 4 bytes per pixel
      memcpy(dst, t->pixels.data(), t->width * t->height * 4);
    } else {
      // Convert BGR to RGBA
      for (int i = 0; i < t->width * t->height; i++) {
        dst[i * 4 + 0] = t->pixels[i * 3 + 2]; // R
        dst[i * 4 + 1] = t->pixels[i * 3 + 1]; // G
        dst[i * 4 + 2] = t->pixels[i * 3 + 0]; // B
        dst[i * 4 + 3] = 255;                   // A
      }
    }
    vkUnmapMemory(device, stagingMem);

    auto cmd = BeginCmd();
    TransitionImage(cmd, tg.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkBufferImageCopy region = {};
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = {(uint32_t)t->width, (uint32_t)t->height, 1};
    vkCmdCopyBufferToImage(cmd, staging, tg.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    TransitionImage(cmd, tg.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    EndCmd(cmd);

    vkDestroyBuffer(device, staging, nullptr);
    vkFreeMemory(device, stagingMem, nullptr);

    texGPUs.push_back(tg);
    return idx;
  };

  // Ensure a default white texture exists at index 0
  {
    TexGPU tg;
    CreateImage(1, 1, VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                tg.image, tg.mem);
    tg.view = CreateImageView(tg.image, VK_FORMAT_R8G8B8A8_UNORM);

    VkBuffer staging; VkDeviceMemory stagingMem;
    CreateBuffer(4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 staging, stagingMem);
    void* data;
    vkMapMemory(device, stagingMem, 0, 4, 0, &data);
    uint8_t white[] = {255, 255, 255, 255};
    memcpy(data, white, 4);
    vkUnmapMemory(device, stagingMem);

    auto cmd = BeginCmd();
    TransitionImage(cmd, tg.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkBufferImageCopy region = {};
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = {1, 1, 1};
    vkCmdCopyBufferToImage(cmd, staging, tg.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    TransitionImage(cmd, tg.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    EndCmd(cmd);

    vkDestroyBuffer(device, staging, nullptr);
    vkFreeMemory(device, stagingMem, nullptr);
    texGPUs.push_back(tg);
  }

  auto ensureMesh = [&](Mesh* m) -> uint32_t {
    auto it = meshIndexMap.find(m);
    if (it != meshIndexMap.end()) return it->second;
    uint32_t idx = (uint32_t)meshGPUs.size();
    meshIndexMap[m] = idx;
    MeshGPU mg;
    mg.vertexOffset = vertexOffset;
    mg.vertexCount = (uint32_t)m->verts.size() / 3;

    // Append vertex data
    allPos.insert(allPos.end(), m->verts.begin(), m->verts.end());
    allNrm.insert(allNrm.end(), m->normals.begin(), m->normals.end());
    // UVs: always store 2 components
    if (m->is3DTex) {
      for (size_t i = 0; i < m->uvs.size(); i += 3) {
        allUV.push_back(m->uvs[i]);
        allUV.push_back(m->uvs[i + 1]);
      }
    } else {
      allUV.insert(allUV.end(), m->uvs.begin(), m->uvs.end());
    }

    // Vertex colors: 3 floats per vertex; fill with (1,1,1) if mesh has none
    if (!m->colors.empty()) {
      allCol.insert(allCol.end(), m->colors.begin(), m->colors.end());
    } else {
      allCol.resize(allCol.size() + mg.vertexCount * 3, 1.0f);
    }

    // Per-vertex texture indices: upload embedded textures, map local → global
    if (!m->texIndices.empty() && !m->embeddedTextures.empty()) {
      std::vector<uint32_t> localToGlobal;
      for (size_t ti = 0; ti < m->embeddedTextures.size(); ++ti) {
        localToGlobal.push_back(ensureTex(m->embeddedTextures[ti].get()));
      }
      for (size_t vi = 0; vi < mg.vertexCount; vi++) {
        uint32_t li = m->texIndices[vi];
        uint32_t gi = (li < (uint32_t)localToGlobal.size()) ? localToGlobal[li] : 0;
        allTexIdx.push_back(gi);
      }
    } else {
      allTexIdx.resize(allTexIdx.size() + mg.vertexCount, 0);
    }

    meshOffsets.push_back(vertexOffset);
    vertexOffset += mg.vertexCount;
    meshGPUs.push_back(mg);
    return idx;
  };

  // Process objects and portals
  std::vector<GPUInstanceInfo> instanceInfos;
  std::vector<GPUPortalWarpInfo> portalWarpInfos;
  std::vector<uint32_t> portalInstanceIndices; // compact list of portal instance indices
  struct TLASInstance {
    Matrix4 transform;
    uint32_t customIndex;
    uint32_t blasIdx;
    uint32_t sbtOffset; // 0=material, 1=portal
    uint32_t flags;
    uint32_t mask = 0xFF;
  };
  std::vector<TLASInstance> tlasInstances;

  // Regular objects
  for (size_t i = 0; i < world.objects.size(); i++) {
    Object* obj = world.objects[i].get();
    if (!obj->mesh || !obj->visible) continue;

    uint32_t mi = ensureMesh(obj->mesh.get());
    uint32_t ti = obj->texture ? ensureTex(obj->texture.get()) : 0;

    GPUInstanceInfo info = {};
    Matrix4 m = obj->LocalToWorld();
    Matrix4 mInv = obj->WorldToLocal().Transposed(); // transpose(inverse(model)) for normals
    memcpy(info.model, m.m, 64);
    memcpy(info.normalMat, mInv.m, 64);
    info.emissive[0] = obj->emissive.x;
    info.emissive[1] = obj->emissive.y;
    info.emissive[2] = obj->emissive.z;
    info.indices[0] = mi;
    info.indices[1] = 0; // not portal
    info.indices[2] = ti;

    TLASInstance ti2;
    ti2.transform = m;
    ti2.customIndex = (uint32_t)instanceInfos.size();
    ti2.blasIdx = mi;
    ti2.sbtOffset = 0; // hit groups 0,1 (closesthit, shadowhit)
    ti2.flags = 0;
    ti2.mask = obj->castShadow ? 0xFF : 0xFE;

    instanceInfos.push_back(info);
    tlasInstances.push_back(ti2);
  }

  // Portals
  for (size_t i = 0; i < world.portals.size(); i++) {
    Portal* p = world.portals[i].get();
    if (!p->mesh) continue;

    uint32_t mi = ensureMesh(p->mesh.get());

    GPUInstanceInfo info = {};
    Matrix4 m = p->LocalToWorld();
    Matrix4 mInv = p->WorldToLocal().Transposed();
    memcpy(info.model, m.m, 64);
    memcpy(info.normalMat, mInv.m, 64);
    info.indices[0] = mi;
    info.indices[1] = 1; // is portal
    info.indices[2] = 0; // no texture for portals
    info.indices[3] = (uint32_t)portalWarpInfos.size(); // portal warp index

    // Portal warp data stored in separate buffer for cache efficiency
    GPUPortalWarpInfo warpInfo = {};
    memcpy(warpInfo.frontWarp, p->front.deltaInv.m, 64);
    memcpy(warpInfo.frontReverse, p->front.delta.m, 64);
    memcpy(warpInfo.backWarp, p->back.deltaInv.m, 64);
    memcpy(warpInfo.backReverse, p->back.delta.m, 64);

    // Precompute world-space portal geometry to avoid shader-side matrix multiplies
    Vector3 portalCenter = m.MulPoint(Vector3(0, 0, 0));
    Matrix4 nMat = p->WorldToLocal().Transposed();
    Vector3 portalFwd = (nMat * Vector4(0, 0, -1, 0)).XYZ().Normalized();
    Vector3 portalX = (m * Vector4(1, 0, 0, 0)).XYZ();
    Vector3 portalY = (m * Vector4(0, 1, 0, 0)).XYZ();
    warpInfo.center[0] = portalCenter.x; warpInfo.center[1] = portalCenter.y; warpInfo.center[2] = portalCenter.z;
    warpInfo.forward[0] = portalFwd.x; warpInfo.forward[1] = portalFwd.y; warpInfo.forward[2] = portalFwd.z;
    warpInfo.axisX[0] = portalX.x; warpInfo.axisX[1] = portalX.y; warpInfo.axisX[2] = portalX.z;
    warpInfo.axisY[0] = portalY.x; warpInfo.axisY[1] = portalY.y; warpInfo.axisY[2] = portalY.z;

    // AABB for portal influence zone: conservative box around portal center.
    // At radius ~54 units the quadratic attenuation drops below 1%, so use 50.
    const float influenceRadius = 50.0f;
    warpInfo.aabbMin[0] = portalCenter.x - influenceRadius;
    warpInfo.aabbMin[1] = portalCenter.y - influenceRadius;
    warpInfo.aabbMin[2] = portalCenter.z - influenceRadius;
    warpInfo.aabbMax[0] = portalCenter.x + influenceRadius;
    warpInfo.aabbMax[1] = portalCenter.y + influenceRadius;
    warpInfo.aabbMax[2] = portalCenter.z + influenceRadius;

    portalWarpInfos.push_back(warpInfo);

    // Track this portal's instance index for the portal index buffer
    portalInstanceIndices.push_back((uint32_t)instanceInfos.size());

    TLASInstance ti2;
    ti2.transform = m;
    ti2.customIndex = (uint32_t)instanceInfos.size();
    ti2.blasIdx = mi;
    ti2.sbtOffset = 2; // hit groups 2,3 (portalhit, portalshadow)
    ti2.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    // Reserve bit 0 for regular shadow-casting geometry. Portals use bit 1 so
    // primary rays (cull mask 0xFF) still hit them, while shadow/path
    // validation rays using cull mask 0x01 ignore portal quads.
    ti2.mask = 0x02;

    instanceInfos.push_back(info);
    tlasInstances.push_back(ti2);
  }

  // Upload vertex data to GPU
  auto uploadBuffer = [&](const void* data, VkDeviceSize size, VkBufferUsageFlags extraFlags,
                          VkBuffer& buf, VkDeviceMemory& mem) {
    if (size == 0) size = 4; // minimum
    VkBuffer staging; VkDeviceMemory stagingMem;
    CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 staging, stagingMem);
    void* mapped;
    vkMapMemory(device, stagingMem, 0, size, 0, &mapped);
    if (data) memcpy(mapped, data, size);
    vkUnmapMemory(device, stagingMem);

    CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | extraFlags,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buf, mem);
    auto cmd = BeginCmd();
    VkBufferCopy copy = {0, 0, size};
    vkCmdCopyBuffer(cmd, staging, buf, 1, &copy);
    EndCmd(cmd);

    vkDestroyBuffer(device, staging, nullptr);
    vkFreeMemory(device, stagingMem, nullptr);
  };

  VkBufferUsageFlags vertUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

  uploadBuffer(allPos.data(), allPos.size() * sizeof(float), vertUsage, posBuffer, posMem);
  uploadBuffer(allNrm.data(), allNrm.size() * sizeof(float),
               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, nrmBuffer, nrmMem);
  uploadBuffer(allUV.data(), allUV.size() * sizeof(float),
               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, uvBuffer, uvMem);
  uploadBuffer(allCol.data(), allCol.size() * sizeof(float),
               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, colBuffer, colMem);
  uploadBuffer(allTexIdx.data(), allTexIdx.size() * sizeof(uint32_t),
               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, texIdxBuffer, texIdxMem);
  uploadBuffer(meshOffsets.data(), meshOffsets.size() * sizeof(uint32_t),
               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, meshOffBuffer, meshOffMem);
  uploadBuffer(instanceInfos.data(), instanceInfos.size() * sizeof(GPUInstanceInfo),
               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, instanceBuffer, instanceMem);
  uploadBuffer(portalWarpInfos.data(), portalWarpInfos.size() * sizeof(GPUPortalWarpInfo),
               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, portalWarpBuffer, portalWarpMem);
  uploadBuffer(portalInstanceIndices.data(), portalInstanceIndices.size() * sizeof(uint32_t),
               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, portalIndexBuffer, portalIndexMem);

  // Build BLAS per unique mesh
  for (size_t i = 0; i < meshGPUs.size(); i++) {
    auto& mg = meshGPUs[i];

    VkAccelerationStructureGeometryTrianglesDataKHR triangles =
      {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR};
    triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    triangles.vertexData.deviceAddress = GetBufferAddress(posBuffer) + mg.vertexOffset * 3 * sizeof(float);
    triangles.vertexStride = 3 * sizeof(float);
    triangles.maxVertex = mg.vertexCount - 1;
    triangles.indexType = VK_INDEX_TYPE_NONE_KHR;

    VkAccelerationStructureGeometryKHR geom =
      {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    geom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geom.geometry.triangles = triangles;
    geom.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo =
      {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries = &geom;

    uint32_t primCount = mg.vertexCount / 3;
    VkAccelerationStructureBuildSizesInfoKHR sizeInfo =
      {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                            &buildInfo, &primCount, &sizeInfo);

    CreateBuffer(sizeInfo.accelerationStructureSize,
                 VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mg.blasBuf, mg.blasMem);

    VkAccelerationStructureCreateInfoKHR asCI =
      {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    asCI.buffer = mg.blasBuf;
    asCI.size = sizeInfo.accelerationStructureSize;
    asCI.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    vkCreateAccelerationStructureKHR(device, &asCI, nullptr, &mg.blas);

    // Scratch buffer
    VkBuffer scratchBuf; VkDeviceMemory scratchMem;
    CreateBuffer(sizeInfo.buildScratchSize,
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, scratchBuf, scratchMem);

    buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.dstAccelerationStructure = mg.blas;
    buildInfo.scratchData.deviceAddress = GetBufferAddress(scratchBuf);

    VkAccelerationStructureBuildRangeInfoKHR rangeInfo = {};
    rangeInfo.primitiveCount = primCount;
    const VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &rangeInfo;

    auto cmd = BeginCmd();
    vkCmdBuildAccelerationStructuresKHR(cmd, 1, &buildInfo, &pRangeInfo);
    EndCmd(cmd);

    vkDestroyBuffer(device, scratchBuf, nullptr);
    vkFreeMemory(device, scratchMem, nullptr);
  }

  // Build TLAS
  {
    std::vector<VkAccelerationStructureInstanceKHR> vkInstances(tlasInstances.size());
    for (size_t i = 0; i < tlasInstances.size(); i++) {
      auto& src = tlasInstances[i];
      auto& dst = vkInstances[i];
      memset(&dst, 0, sizeof(dst));

      // VkTransformMatrixKHR is 3x4 row-major
      const float* m = src.transform.m;
      dst.transform.matrix[0][0] = m[0];  dst.transform.matrix[0][1] = m[1];
      dst.transform.matrix[0][2] = m[2];  dst.transform.matrix[0][3] = m[3];
      dst.transform.matrix[1][0] = m[4];  dst.transform.matrix[1][1] = m[5];
      dst.transform.matrix[1][2] = m[6];  dst.transform.matrix[1][3] = m[7];
      dst.transform.matrix[2][0] = m[8];  dst.transform.matrix[2][1] = m[9];
      dst.transform.matrix[2][2] = m[10]; dst.transform.matrix[2][3] = m[11];

      dst.instanceCustomIndex = src.customIndex;
      dst.mask = src.mask;
      dst.instanceShaderBindingTableRecordOffset = src.sbtOffset;
      dst.flags = src.flags;

      VkAccelerationStructureDeviceAddressInfoKHR addrInfo =
        {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
      addrInfo.accelerationStructure = meshGPUs[src.blasIdx].blas;
      dst.accelerationStructureReference = vkGetAccelerationStructureDeviceAddressKHR(device, &addrInfo);
    }

    VkDeviceSize instSize = vkInstances.size() * sizeof(VkAccelerationStructureInstanceKHR);
    uploadBuffer(vkInstances.data(), instSize,
                 VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                 tlasInstanceBuf, tlasInstMem);

    VkAccelerationStructureGeometryInstancesDataKHR instData =
      {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR};
    instData.data.deviceAddress = GetBufferAddress(tlasInstanceBuf);

    VkAccelerationStructureGeometryKHR geom =
      {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    geom.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geom.geometry.instances = instData;

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo =
      {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries = &geom;

    uint32_t instCount = (uint32_t)tlasInstances.size();
    VkAccelerationStructureBuildSizesInfoKHR sizeInfo =
      {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                            &buildInfo, &instCount, &sizeInfo);

    CreateBuffer(sizeInfo.accelerationStructureSize,
                 VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tlasBuf, tlasMem);

    VkAccelerationStructureCreateInfoKHR asCI =
      {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    asCI.buffer = tlasBuf;
    asCI.size = sizeInfo.accelerationStructureSize;
    asCI.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    vkCreateAccelerationStructureKHR(device, &asCI, nullptr, &tlas);

    VkBuffer scratchBuf; VkDeviceMemory scratchMem;
    CreateBuffer(sizeInfo.buildScratchSize,
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, scratchBuf, scratchMem);

    buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.dstAccelerationStructure = tlas;
    buildInfo.scratchData.deviceAddress = GetBufferAddress(scratchBuf);

    VkAccelerationStructureBuildRangeInfoKHR rangeInfo = {};
    rangeInfo.primitiveCount = instCount;
    const VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &rangeInfo;

    auto cmd = BeginCmd();
    vkCmdBuildAccelerationStructuresKHR(cmd, 1, &buildInfo, &pRangeInfo);
    EndCmd(cmd);

    vkDestroyBuffer(device, scratchBuf, nullptr);
    vkFreeMemory(device, scratchMem, nullptr);
  }

  // Create light SSBO (host-visible, updated per-frame in Render)
  {
    const VkDeviceSize maxLights = 32;
    const VkDeviceSize lightBufSize = maxLights * 2 * sizeof(float) * 4; // GPULight = 2 x vec4
    CreateBuffer(lightBufSize,
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 lightBuffer, lightMem);
    vkMapMemory(device, lightMem, 0, lightBufSize, 0, &lightMapped);
    memset(lightMapped, 0, (size_t)lightBufSize);
  }

  // Load HUD overlay image (phone_teto.png)
  {
    int ow = 0, oh = 0, oc = 0;
    unsigned char* pixels = stbi_load("Textures/phone_teto.png", &ow, &oh, &oc, 4);
    if (pixels && ow > 0 && oh > 0) {
      overlayWidth = (uint32_t)ow;
      overlayHeight = (uint32_t)oh;

      CreateImage(overlayWidth, overlayHeight, VK_FORMAT_R8G8B8A8_UNORM,
                  VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                  overlayImage, overlayMem);
      overlayView = CreateImageView(overlayImage, VK_FORMAT_R8G8B8A8_UNORM);

      VkDeviceSize imgSize = overlayWidth * overlayHeight * 4;
      VkBuffer staging; VkDeviceMemory stagingMem;
      CreateBuffer(imgSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                   staging, stagingMem);
      void* data;
      vkMapMemory(device, stagingMem, 0, imgSize, 0, &data);
      memcpy(data, pixels, imgSize);
      vkUnmapMemory(device, stagingMem);

      auto cmd = BeginCmd();
      TransitionImage(cmd, overlayImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
      VkBufferImageCopy region = {};
      region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
      region.imageExtent = {overlayWidth, overlayHeight, 1};
      vkCmdCopyBufferToImage(cmd, staging, overlayImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
      TransitionImage(cmd, overlayImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
      EndCmd(cmd);

      vkDestroyBuffer(device, staging, nullptr);
      vkFreeMemory(device, stagingMem, nullptr);
      stbi_image_free(pixels);

      // Create overlay composite compute pipeline
      {
        VkDescriptorSetLayoutBinding bindings[2] = {};
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo dslci = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        dslci.bindingCount = 2;
        dslci.pBindings = bindings;
        vkCreateDescriptorSetLayout(device, &dslci, nullptr, &overlayDescLayout);

        VkPushConstantRange pcRange = {};
        pcRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pcRange.offset = 0;
        pcRange.size = 24; // 3 x ivec2

        VkPipelineLayoutCreateInfo plci = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        plci.setLayoutCount = 1;
        plci.pSetLayouts = &overlayDescLayout;
        plci.pushConstantRangeCount = 1;
        plci.pPushConstantRanges = &pcRange;
        vkCreatePipelineLayout(device, &plci, nullptr, &overlayPipeLayout);

        auto compSpirv = CompileGLSL("overlay_composite.comp", EShLangCompute);
        VkShaderModule compModule = CreateShaderModule(compSpirv);

        VkComputePipelineCreateInfo cpci = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        cpci.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        cpci.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        cpci.stage.module = compModule;
        cpci.stage.pName = "main";
        cpci.layout = overlayPipeLayout;
        vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &cpci, nullptr, &overlayPipeline);
        vkDestroyShaderModule(device, compModule, nullptr);
      }

      // Allocate overlay descriptor set
      {
        VkDescriptorPoolSize poolSize = {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2};
        VkDescriptorPoolCreateInfo dpci = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        dpci.maxSets = 1;
        dpci.poolSizeCount = 1;
        dpci.pPoolSizes = &poolSize;
        vkCreateDescriptorPool(device, &dpci, nullptr, &overlayDescPool);

        VkDescriptorSetAllocateInfo dsai = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        dsai.descriptorPool = overlayDescPool;
        dsai.descriptorSetCount = 1;
        dsai.pSetLayouts = &overlayDescLayout;
        vkAllocateDescriptorSets(device, &dsai, &overlayDescSet);

        VkDescriptorImageInfo imgInfos[2] = {};
        imgInfos[0].imageView = storageView;
        imgInfos[0].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        imgInfos[1].imageView = overlayView;
        imgInfos[1].imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkWriteDescriptorSet writes[2] = {};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = overlayDescSet;
        writes[0].dstBinding = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[0].pImageInfo = &imgInfos[0];
        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = overlayDescSet;
        writes[1].dstBinding = 1;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[1].pImageInfo = &imgInfos[1];
        vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);
      }
    }
  }

  // Allocate and update descriptor set
  {
    VkDescriptorPoolSize poolSizes[] = {
      {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 128},
    };
    VkDescriptorPoolCreateInfo dpci = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    dpci.maxSets = 1;
    dpci.poolSizeCount = 5;
    dpci.pPoolSizes = poolSizes;
    if (descPool) vkDestroyDescriptorPool(device, descPool, nullptr);
    vkCreateDescriptorPool(device, &dpci, nullptr, &descPool);

    VkDescriptorSetAllocateInfo dsai = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    dsai.descriptorPool = descPool;
    dsai.descriptorSetCount = 1;
    dsai.pSetLayouts = &descLayout;
    vkAllocateDescriptorSets(device, &dsai, &descSet);
  }

  UpdateDescriptorSet();
  sceneUploaded = true;
}

void VulkanRenderer::UpdateDescriptorSet() {
  // TLAS
  VkWriteDescriptorSetAccelerationStructureKHR asWrite =
    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR};
  asWrite.accelerationStructureCount = 1;
  asWrite.pAccelerationStructures = &tlas;

  // Storage image
  VkDescriptorImageInfo imgInfo = {};
  imgInfo.imageView = storageView;
  imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

  // UBO
  VkDescriptorBufferInfo uboInfo = {uboBuffer, 0, 256};

  // SSBOs
  VkDescriptorBufferInfo ssbos[5] = {};
  ssbos[0] = {instanceBuffer, 0, VK_WHOLE_SIZE};
  ssbos[1] = {posBuffer, 0, VK_WHOLE_SIZE};
  ssbos[2] = {nrmBuffer, 0, VK_WHOLE_SIZE};
  ssbos[3] = {uvBuffer, 0, VK_WHOLE_SIZE};
  ssbos[4] = {meshOffBuffer, 0, VK_WHOLE_SIZE};

  // Portal warp SSBO (binding 8)
  VkDescriptorBufferInfo portalWarpInfo = {portalWarpBuffer, 0, VK_WHOLE_SIZE};

  // Portal index SSBO (binding 9)
  VkDescriptorBufferInfo portalIndexInfo = {portalIndexBuffer, 0, VK_WHOLE_SIZE};

  // Vertex colors SSBO (binding 11)
  VkDescriptorBufferInfo colInfo = {colBuffer, 0, VK_WHOLE_SIZE};

  // Per-vertex texture index SSBO (binding 12)
  VkDescriptorBufferInfo texIdxInfo = {texIdxBuffer, 0, VK_WHOLE_SIZE};

  // Lights SSBO (binding 13)
  VkDescriptorBufferInfo lightInfo = {lightBuffer, 0, VK_WHOLE_SIZE};

  // Textures
  std::vector<VkDescriptorImageInfo> texInfos(texGPUs.size());
  for (size_t i = 0; i < texGPUs.size(); i++) {
    texInfos[i].sampler = sampler;
    texInfos[i].imageView = texGPUs[i].view;
    texInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  }

  VkWriteDescriptorSet writes[14] = {};
  for (auto& w : writes) w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

  writes[0].dstSet = descSet; writes[0].dstBinding = 0;
  writes[0].descriptorCount = 1; writes[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
  writes[0].pNext = &asWrite;

  writes[1].dstSet = descSet; writes[1].dstBinding = 1;
  writes[1].descriptorCount = 1; writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  writes[1].pImageInfo = &imgInfo;

  writes[2].dstSet = descSet; writes[2].dstBinding = 2;
  writes[2].descriptorCount = 1; writes[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writes[2].pBufferInfo = &uboInfo;

  for (int i = 0; i < 5; i++) {
    writes[3 + i].dstSet = descSet; writes[3 + i].dstBinding = 3 + i;
    writes[3 + i].descriptorCount = 1; writes[3 + i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[3 + i].pBufferInfo = &ssbos[i];
  }

  writes[8].dstSet = descSet; writes[8].dstBinding = 8;
  writes[8].descriptorCount = 1; writes[8].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writes[8].pBufferInfo = &portalWarpInfo;

  writes[9].dstSet = descSet; writes[9].dstBinding = 9;
  writes[9].descriptorCount = 1; writes[9].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writes[9].pBufferInfo = &portalIndexInfo;

  writes[10].dstSet = descSet; writes[10].dstBinding = 10;
  writes[10].descriptorCount = (uint32_t)texInfos.size();
  writes[10].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writes[10].pImageInfo = texInfos.data();

  writes[11].dstSet = descSet; writes[11].dstBinding = 11;
  writes[11].descriptorCount = 1; writes[11].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writes[11].pBufferInfo = &colInfo;

  writes[12].dstSet = descSet; writes[12].dstBinding = 12;
  writes[12].descriptorCount = 1; writes[12].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writes[12].pBufferInfo = &texIdxInfo;

  writes[13].dstSet = descSet; writes[13].dstBinding = 13;
  writes[13].descriptorCount = 1; writes[13].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writes[13].pBufferInfo = &lightInfo;

  vkUpdateDescriptorSets(device, 14, writes, 0, nullptr);
}

// ============================================================
// Render
// ============================================================
void VulkanRenderer::Render(const Camera& cam, const World& world) {
  if (!sceneUploaded) return;

  vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
  vkResetFences(device, 1, &fence);

  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, semImageAvail, VK_NULL_HANDLE, &imageIndex);
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    OnResize(width, height);
    return;
  }

  // Update UBO
  {
    Matrix4 viewInv = cam.worldView.Inverse();
    Matrix4 projInv = cam.InverseProjection();

    // Must match GLSL std140 layout exactly
    struct alignas(16) UBOData {
      float viewInv[16];  // offset 0
      float projInv[16];  // offset 64
      float camPos[4];    // offset 128, w = time_of_day
      float params[4];    // offset 144, x = ambientScale, y = lightCount
    };
    static_assert(sizeof(UBOData) <= 256, "UBO too large");

    UBOData ubo;
    memset(&ubo, 0, sizeof(ubo));
    memcpy(ubo.viewInv, viewInv.m, 64);
    memcpy(ubo.projInv, projInv.m, 64);
    ubo.camPos[0] = cam.worldPos.x; ubo.camPos[1] = cam.worldPos.y;
    ubo.camPos[2] = cam.worldPos.z; ubo.camPos[3] = world.time_of_day;
    ubo.params[0] = world.ambientScale;
    ubo.params[1] = (float)world.lights.size();

    memcpy(uboMapped, &ubo, sizeof(ubo));
  }

  // Update lights SSBO
  if (lightMapped) {
    struct GPULight { float pos[4]; float color[4]; };
    const size_t maxLights = 32;
    size_t count = world.lights.size() < maxLights ? world.lights.size() : maxLights;
    GPULight gpuLights[32];
    memset(gpuLights, 0, sizeof(gpuLights));
    for (size_t i = 0; i < count; ++i) {
      gpuLights[i].pos[0] = world.lights[i].pos.x;
      gpuLights[i].pos[1] = world.lights[i].pos.y;
      gpuLights[i].pos[2] = world.lights[i].pos.z;
      gpuLights[i].pos[3] = world.lights[i].radius;
      gpuLights[i].color[0] = world.lights[i].color.x;
      gpuLights[i].color[1] = world.lights[i].color.y;
      gpuLights[i].color[2] = world.lights[i].color.z;
    }
    memcpy(lightMapped, gpuLights, sizeof(gpuLights));
  }

  // Record command buffer
  vkResetCommandBuffer(cmdBuf, 0);
  VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  vkBeginCommandBuffer(cmdBuf, &beginInfo);

  // Ensure storage image is GENERAL
  TransitionImage(cmdBuf, storageImage, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

  vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);
  vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeLayout,
                          0, 1, &descSet, 0, nullptr);

  vkCmdTraceRaysKHR(cmdBuf, &rgenRegion, &missRegion, &hitRegion, &callRegion,
                    swapExtent.width * GH_SSAA_FACTOR, swapExtent.height * GH_SSAA_FACTOR, 1);

  // Composite overlay onto storage image via compute shader (alpha-blended)
  if (overlayImage && overlayPipeline) {
    // Barrier: RT writes -> compute reads on storage image
    VkMemoryBarrier rtToCompute = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    rtToCompute.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    rtToCompute.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &rtToCompute, 0, nullptr, 0, nullptr);

    // 60% of storage image height
    int32_t stH = (int32_t)(swapExtent.height * GH_SSAA_FACTOR);
    int32_t dstH = stH * 3 / 7;
    int32_t dstW = dstH * (int32_t)overlayWidth / (int32_t)overlayHeight;
    int32_t dstX = 0;
    int32_t dstY = stH - dstH;

    int32_t pc[6] = {dstX, dstY, dstW, dstH, (int32_t)overlayWidth, (int32_t)overlayHeight};
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, overlayPipeline);
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, overlayPipeLayout,
                            0, 1, &overlayDescSet, 0, nullptr);
    vkCmdPushConstants(cmdBuf, overlayPipeLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 24, pc);
    vkCmdDispatch(cmdBuf, ((uint32_t)dstW + 15) / 16, ((uint32_t)dstH + 15) / 16, 1);

    // Barrier: compute writes -> transfer read
    VkMemoryBarrier barrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
  }

  // Copy storage image to swapchain image
  TransitionImage(cmdBuf, storageImage, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  TransitionImage(cmdBuf, swapImages[imageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  VkImageBlit blit = {};
  blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
  blit.srcOffsets[1] = {(int32_t)swapExtent.width * GH_SSAA_FACTOR, (int32_t)swapExtent.height * GH_SSAA_FACTOR, 1};
  blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
  blit.dstOffsets[1] = {(int32_t)swapExtent.width, (int32_t)swapExtent.height, 1};
  vkCmdBlitImage(cmdBuf, storageImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                 swapImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                 1, &blit, VK_FILTER_LINEAR);

  TransitionImage(cmdBuf, swapImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
  TransitionImage(cmdBuf, storageImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

  vkEndCommandBuffer(cmdBuf);

  VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
  VkSubmitInfo si = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
  si.waitSemaphoreCount = 1;
  si.pWaitSemaphores = &semImageAvail;
  si.pWaitDstStageMask = &waitStage;
  si.commandBufferCount = 1;
  si.pCommandBuffers = &cmdBuf;
  si.signalSemaphoreCount = 1;
  si.pSignalSemaphores = &semRenderDone;
  vkQueueSubmit(queue, 1, &si, fence);

  VkPresentInfoKHR pi = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  pi.waitSemaphoreCount = 1;
  pi.pWaitSemaphores = &semRenderDone;
  pi.swapchainCount = 1;
  pi.pSwapchains = &swapchain;
  pi.pImageIndices = &imageIndex;
  vkQueuePresentKHR(queue, &pi);
}

// ============================================================
// Resize
// ============================================================
void VulkanRenderer::OnResize(int w, int h) {
  if (w == 0 || h == 0) return;
  width = w; height = h;
  vkDeviceWaitIdle(device);
  DestroyStorageImage();
  DestroySwapchain();
  CreateSwapchain();
  CreateStorageImage();
  if (sceneUploaded) UpdateDescriptorSet();
}

// ============================================================
// Cleanup
// ============================================================
void VulkanRenderer::DestroyScene() {
  if (!device) return;
  vkDeviceWaitIdle(device);

  auto destroyBuf = [&](VkBuffer& b, VkDeviceMemory& m) {
    if (b) { vkDestroyBuffer(device, b, nullptr); b = VK_NULL_HANDLE; }
    if (m) { vkFreeMemory(device, m, nullptr); m = VK_NULL_HANDLE; }
  };

  if (tlas) { vkDestroyAccelerationStructureKHR(device, tlas, nullptr); tlas = VK_NULL_HANDLE; }
  destroyBuf(tlasBuf, tlasMem);
  destroyBuf(tlasInstanceBuf, tlasInstMem);

  for (auto& mg : meshGPUs) {
    if (mg.blas) vkDestroyAccelerationStructureKHR(device, mg.blas, nullptr);
    destroyBuf(mg.blasBuf, mg.blasMem);
  }
  meshGPUs.clear();
  meshIndexMap.clear();

  for (auto& tg : texGPUs) {
    if (tg.view) vkDestroyImageView(device, tg.view, nullptr);
    if (tg.image) vkDestroyImage(device, tg.image, nullptr);
    if (tg.mem) vkFreeMemory(device, tg.mem, nullptr);
  }
  texGPUs.clear();
  texIndexMap.clear();

  destroyBuf(posBuffer, posMem);
  destroyBuf(nrmBuffer, nrmMem);
  destroyBuf(uvBuffer, uvMem);
  destroyBuf(colBuffer, colMem);
  destroyBuf(texIdxBuffer, texIdxMem);
  destroyBuf(meshOffBuffer, meshOffMem);
  destroyBuf(instanceBuffer, instanceMem);
  destroyBuf(portalWarpBuffer, portalWarpMem);
  destroyBuf(portalIndexBuffer, portalIndexMem);
  if (lightMapped) { vkUnmapMemory(device, lightMem); lightMapped = nullptr; }
  destroyBuf(lightBuffer, lightMem);

  if (overlayImage) { vkDestroyImage(device, overlayImage, nullptr); overlayImage = VK_NULL_HANDLE; }
  if (overlayView) { vkDestroyImageView(device, overlayView, nullptr); overlayView = VK_NULL_HANDLE; }
  if (overlayMem) { vkFreeMemory(device, overlayMem, nullptr); overlayMem = VK_NULL_HANDLE; }
  if (overlayPipeline) { vkDestroyPipeline(device, overlayPipeline, nullptr); overlayPipeline = VK_NULL_HANDLE; }
  if (overlayPipeLayout) { vkDestroyPipelineLayout(device, overlayPipeLayout, nullptr); overlayPipeLayout = VK_NULL_HANDLE; }
  if (overlayDescLayout) { vkDestroyDescriptorSetLayout(device, overlayDescLayout, nullptr); overlayDescLayout = VK_NULL_HANDLE; }
  if (overlayDescPool) { vkDestroyDescriptorPool(device, overlayDescPool, nullptr); overlayDescPool = VK_NULL_HANDLE; }
  overlayDescSet = VK_NULL_HANDLE;
  overlayWidth = 0; overlayHeight = 0;

  if (descPool) { vkDestroyDescriptorPool(device, descPool, nullptr); descPool = VK_NULL_HANDLE; }
  descSet = VK_NULL_HANDLE;
  sceneUploaded = false;
}

void VulkanRenderer::Shutdown() {
  if (!device) return;
  vkDeviceWaitIdle(device);

  glslang::FinalizeProcess();

  DestroyScene();

  auto destroyBuf = [&](VkBuffer& b, VkDeviceMemory& m) {
    if (b) { vkDestroyBuffer(device, b, nullptr); b = VK_NULL_HANDLE; }
    if (m) { vkFreeMemory(device, m, nullptr); m = VK_NULL_HANDLE; }
  };

  destroyBuf(sbtBuffer, sbtMem);
  if (uboMapped) { vkUnmapMemory(device, uboMem); uboMapped = nullptr; }
  destroyBuf(uboBuffer, uboMem);

  if (sampler) { vkDestroySampler(device, sampler, nullptr); sampler = VK_NULL_HANDLE; }
  if (pipeline) { vkDestroyPipeline(device, pipeline, nullptr); pipeline = VK_NULL_HANDLE; }
  if (pipeLayout) { vkDestroyPipelineLayout(device, pipeLayout, nullptr); pipeLayout = VK_NULL_HANDLE; }
  if (descLayout) { vkDestroyDescriptorSetLayout(device, descLayout, nullptr); descLayout = VK_NULL_HANDLE; }

  DestroyStorageImage();
  DestroySwapchain();

  if (fence) { vkDestroyFence(device, fence, nullptr); fence = VK_NULL_HANDLE; }
  if (semImageAvail) { vkDestroySemaphore(device, semImageAvail, nullptr); semImageAvail = VK_NULL_HANDLE; }
  if (semRenderDone) { vkDestroySemaphore(device, semRenderDone, nullptr); semRenderDone = VK_NULL_HANDLE; }
  if (cmdPool) { vkDestroyCommandPool(device, cmdPool, nullptr); cmdPool = VK_NULL_HANDLE; }
  if (device) { vkDestroyDevice(device, nullptr); device = VK_NULL_HANDLE; }
  if (surface) { vkDestroySurfaceKHR(instance, surface, nullptr); surface = VK_NULL_HANDLE; }
  if (instance) { vkDestroyInstance(instance, nullptr); instance = VK_NULL_HANDLE; }
}
