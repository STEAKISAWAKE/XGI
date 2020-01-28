#ifndef Graphics_h
#define Graphics_h

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"
#include "Pipeline.h"
#include "VertexBuffer.h"
#include "LinearMath.h"
#include "UniformBuffer.h"
#include "FrameBuffer.h"

typedef struct GraphicsConfigure
{
	bool VulkanValidation;
	bool TargetIntegratedDevice;
	int FrameResourceCount;
} GraphicsConfigure;

struct Graphics
{
	VkInstance Instance;
	VkSurfaceKHR Surface;
	VkPhysicalDevice PhysicalDevice;
	VkDevice Device;
	VkQueue GraphicsQueue;
	unsigned int GraphicsQueueIndex;
	VkQueue PresentQueue;
	unsigned int PresentQueueIndex;
	VkCommandPool CommandPool;
	VmaAllocator Allocator;
	
	VkDescriptorSetLayout DescriptorSetLayout0;
	VkDescriptorSetLayout DescriptorSetLayout1;
	VkDescriptorPool DescriptorPool;
	
	List PreRenderSemaphores;
	int FrameResourceCount;
	struct
	{
		VkCommandBuffer CommandBuffer;
		VkSemaphore ImageAvailable;
		VkSemaphore RenderFinished;
		VkFence FrameReady;
		List DestroyVertexBufferQueue;
	} * FrameResources;
	int FrameIndex;
	
	FrameBuffer BoundFrameBuffer;
	Pipeline BoundPipeline;
	
	int VertexBufferCount;
	int VertexCount;
} extern Graphics;

void GraphicsInitialize(GraphicsConfigure config);
void GraphicsBegin(FrameBuffer frameBuffer);
void GraphicsClearColor(Color clearColor);
void GraphicsDepthStencil(float depth, int stencil);
void GraphicsClear(Color clearColor, float depth, int stencil);
void GraphicsBindPipeline(Pipeline pipeline);
void GraphicsRenderVertexBuffer(VertexBuffer vertexBuffer);
void GraphicsEnd(void);
void GraphicsCopyToSwapchain(FrameBuffer frameBuffer);
void GraphicsStopOperations(void);
void GraphicsDeinitialize(void);

#endif
