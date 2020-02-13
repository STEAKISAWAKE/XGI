# XGI : X-TeK Graphics Programming Interface

**An easy to use graphics library built on top of Vulkan.**

## Features:
- A low level, yet easy to use, graphics library
- A drastic abstraction over vulkan, but still provides much control to the programmer
- Makes use of Vulkan's asynchronous draw calls and uses multiple frames in flight for optimal frame rate
- Provides an abstraction for handling window creation, input devices, events, etc.
- The API is programmed fully in C

## Modules:
Module            | Description
------------------|---------------------
`EventHandler`    | Processes events and manages callbacks
`File`            | Provides an easy way to read/write files
`FrameBuffer`     | Abstracts a color texture and depth-stencil texture for use in rendering
`Graphics`        | Provides all of the commands necessary for rendering
`Input`           | Provides the functionality to query information about input devices
`LinearMath`      | Provides all of the linear algebra functions needed for transformations
`List`            | Provides a dynamic and generic list object (uses void \*)
`Pipeline`        | Abstracts shaders, state configuration, and uniform variables into an object
`Texture`         | Allows for creating/loading images for use in rendering
`UniformBuffer`   | Provides the ability to upload memory to the gpu for use as uniforms in shaders
`VertexBuffer`    | Provides the ability to upload vertices to the gpu for use as input in shaders
`Window`          | Provides the ability to configure and control the window

## How to setup:

### Windows:

### MacOS:

### Linux:

## Example:
```C
#include "../XGI/XGI.h"

// A textured triangle example: in 100 lines of code instead of 1000

// The custom vertex that is used in the shader
typedef struct Vertex
{
	Vector3 Position;
	Vector2 UV;
} Vertex;

// All the objects needed to start rendering
FrameBuffer frameBuffer;   // You need at least one framebuffer to render to
VertexLayout vertexLayout; // Needed for describing the custom vertex to the pipeline
Pipeline pipeline;         // Enscapulates shaders and other configuration into a state object
VertexBuffer vertexBuffer; // The quad used for rendering
Texture texture;           // The texture used to render on the quad

// This is a callback function that is registered with the eventhandler below
static void OnKeyPressed(Key key)
{
	// Detects if key 'escape' is pressed then quits
	if (key == KeyEscape) { WindowExitLoop(); }
}

// Another callback function that is called when the window resizes
static void OnResize(int width, int height)
{
	// You must manually resize framebuffers after a resize
	frameBuffer = FrameBufferResize(frameBuffer, width, height);
}

int main(int argc, char * argv[])
{
	// Window configuration
	WindowConfigure windowConfig =
	{
		.Width = 800,
		.Height = 600,
		.Title = "XGI Example",
		.Resizable = true,
	};
	// Graphics Configuration
	GraphicsConfigure graphicsConfig =
	{
		.VulkanValidation = true,
		.FrameResourceCount = 3,
	};
	XGIInitialize(windowConfig, graphicsConfig);
	
	// Initialization code starts here
	
	// Create the framebuffer
	FrameBufferConfigure frameConfig =
	{
		.Width = Window.Width,
		.Height = Window.Height,
		.Filter = TextureFilterNearest,
		.AddressMode = TextureAddressModeMirroredRepeat,
	};
	frameBuffer = FrameBufferCreate(frameConfig);

	// Create the vertex layout
	VertexAttribute attributes[] = { VertexAttributeVector3, VertexAttributeVector2 };
	vertexLayout = VertexLayoutCreate(2, attributes);

	// Create the pipeline
	PipelineConfigure pipelineConfig =
	{
		.VertexLayout = vertexLayout,
		.ShaderCount = 2,
		.Shaders =
		{
			ShaderDataFromFile(ShaderTypeVertex, "Example/Shaders/Default.vert", false),
			ShaderDataFromFile(ShaderTypeFragment, "Example/Shaders/Default.frag", false),
		},
		.Primitive = VertexPrimitiveTriangleList,
		.LineWidth = 1.0,
		.PolygonMode = PolygonModeFill,
		.CullMode = CullModeNone,
		.CullClockwise = true,
		.AlphaBlend = true,
		.DepthTest = false,
		.StencilTest = false,
	};
	pipeline = PipelineCreate(pipelineConfig);

	// Create the vertex buffer
	vertexBuffer = VertexBufferCreate(6, sizeof(Vertex));
	Vertex * vertices = VertexBufferMapVertices(vertexBuffer);
	vertices[0] = (Vertex){ { -1.0, -1.0, 0.0 }, { 0.0, 0.0 } };
	vertices[1] = (Vertex){ { 1.0, -1.0, 0.0 }, { 1.0, 0.0 } };
	vertices[2] = (Vertex){ { 1.0, 1.0, 0.0 }, { 1.0, 1.0 } };
	vertices[3] = vertices[0];
	vertices[4] = vertices[2];
	vertices[5] = (Vertex){ { -1.0, 1.0, 0.0 }, { 0.0, 1.0 } };
	VertexBufferUnmapVertices(vertexBuffer);
	VertexBufferUpload(vertexBuffer);

	// Create the texture
	TextureData data = TextureDataFromFile("Example/texture.jpg");
	TextureConfigure textureConfig =
	{
		.Format = TextureFormatColor,
		.Filter = TextureFilterNearest,
		.AddressMode = TextureAddressModeRepeat,
		.LoadFromData = true,
		.Data = data,
	};
	texture = TextureCreate(textureConfig);
	TextureDataDestroy(data);
	// Set the pipeline binding 0 to the texture
	PipelineSetSampler(pipeline, 0, 0, texture);

	// Set the event handler callbacks
	EventHandlerSetCallback(EventTypeKeyPressed, (void (*)(void))OnKeyPressed);
	EventHandlerSetCallback(EventTypeWindowResized, (void (*)(void))OnResize);
	
	// Start the main loop
	while (WindowRunning())
	{
		EventHandlerPoll();
		
		// Update code starts here
		// Set the push constant
		PipelineSetPushConstant(pipeline, "Transform", &Matrix4x4Identity);

		SwapchainAquireNextImage();
		
		// Render code starts here
		GraphicsBegin(frameBuffer);
		GraphicsClearColor(ColorFromHex(0x204080ff));
		GraphicsBindPipeline(pipeline);
		GraphicsRenderVertexBuffer(vertexBuffer);
		GraphicsEnd();
		// Copy the framebuffer to the swapchain for rendering
		GraphicsCopyToSwapchain(frameBuffer);

		SwapchainPresent();
	}
	GraphicsStopOperations(); // Important to call this right after exiting the main loop

	// Deinitialization code starts here
	TextureDestroy(texture);
	VertexBufferDestroy(vertexBuffer);
	PipelineDestroy(pipeline);
	FrameBufferDestroy(frameBuffer);
	VertexLayoutDestroy(vertexLayout);

	XGIDeinitialize();
	return 0;
}
```
