#pragma once
#include <vulkan/vulkan.h>

static int g_counter = 0;
static void* alloc_fun(void* pUserData, size_t  size,  size_t  alignment, VkSystemAllocationScope allocationScope)
{
	g_counter++;
	return malloc(size);
}

static void* realloc_fun(void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
	return realloc(pOriginal, size);
}

static void free_fun(void* pUserData, void* pMemory)
{
	if (pMemory != nullptr)
	{
		g_counter--;
		free(pMemory);
	}
}

VkAllocationCallbacks g_allocators = {
	NULL,                 /* pUserData;             */
	alloc_fun,            /* pfnAllocation;         */
	realloc_fun,          /* pfnReallocation;       */
	free_fun,             /* pfnFree;               */
	NULL,                 /* pfnInternalAllocation; */
	NULL                  /* pfnInternalFree;       */
};