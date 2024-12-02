#include <vk_mem_alloc.h>

struct VmaAllocatorWrap
{
    VmaAllocator allocator = VK_NULL_HANDLE;
};

struct VmaAllocationWrap
{
    VmaAllocation allocation = VK_NULL_HANDLE;
};