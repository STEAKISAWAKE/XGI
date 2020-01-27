#include <stdio.h>
#include <assert.h>
#include <XGI/Window.h>
#include <XGI/EventHandler.h>
#include <XGI/LinearMath.h>
#include <XGI/Graphics.h>
#include <XGI/Swapchain.h>
#include <XGI/XGI.h>

int main(int argc, const char * argv[])
{
	struct WindowConfig windowConfig =
	{
		.Width = 800,
		.Height = 600,
		.Title = "XGI Test",
		.HighDPI = true,
		.Resizable = false,
		.FullScreen = false,
	};
	struct GraphicsConfig graphicsConfig =
	{
		.VulkanValidation = true,
		.FrameResourceCount = 3,
	};
	XGIInitialize(windowConfig, graphicsConfig);
	
	FrameBuffer framebuffer = FrameBufferCreate(Window.Width, Window.Height, false);
	
	VertexAttribute attributes[] = { VertexAttributeVector4, VertexAttributeByte4 };
	VertexLayout vertexLayout = VertexLayoutCreate(2, attributes);
	
#include "Shaders/Default.vert.c"
#include "Shaders/Default.frag.c"
	Shader shader =
	{
		.VertexSPVSize = sizeof(SPV_DefaultVertexShader),
		.VertexSPV = SPV_DefaultVertexShader,
		.FragmentSPVSize = sizeof(SPV_DefaultFragmentShader),
		.FragmentSPV = SPV_DefaultFragmentShader,
	};
	Pipeline pipeline = PipelineCreate(shader, vertexLayout);
	float t = 0.0f;
	PipelineSetPushConstant(pipeline, "Time", &t);
	while (Window.Running)
	{
		EventHandlerPoll();
		if (EventHandlerOnKeyPressed(KeyEscape)) { WindowExitLoop(); }
		if (EventHandlerOnWindowResized())
		{
			framebuffer = FrameBufferResize(framebuffer, Window.Width, Window.Height);
		}
		SwapchainAquireNextImage();
		GraphicsBegin(framebuffer);
		GraphicsClear(ColorFromHex(0x204080ff), 1.0f, 0);
		GraphicsBindPipeline(pipeline);
		GraphicsEnd();
		GraphicsCopyToSwapchain(framebuffer);
		SwapchainPresent();
	}
	GraphicsStopOperations();
	
	PipelineDestroy(pipeline);
	FrameBufferDestroy(framebuffer);
	VertexLayoutDestroy(vertexLayout);
	
	XGIDeinitialize();
	return 0;
}
