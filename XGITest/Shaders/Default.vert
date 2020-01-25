#version 450

layout (location = 0) in vec4 PositionAttribute;
layout (location = 1) in vec4 ColorAttribute;

layout (push_constant) uniform PushConstant
{
	float Time;
	vec4 Dimensions;
	mat4 Transform;
	vec4 Color;
} Input;

layout (location = 0) out vec4 VertexColor;

void main()
{
	VertexColor = ColorAttribute;
	float aspect = Input.Dimensions.x / Input.Dimensions.y;
	gl_Position = Input.Transform * vec4(PositionAttribute.xy, 0.0, 1.0);
	gl_Position.xy /= vec2(aspect, 1.0);
}
