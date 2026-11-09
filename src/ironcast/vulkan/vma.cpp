// VMA's implementation, on its own so its warnings and size stay out of backend.cpp. Vulkan functions come from
// VmaAllocatorCreateInfo::pVulkanFunctions: ironcast loads Vulkan at runtime (VK_NO_PROTOTYPES).
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
