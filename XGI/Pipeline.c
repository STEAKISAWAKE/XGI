#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "Pipeline.h"
#include "Graphics.h"
#include "Swapchain.h"
#include "UniformBuffer.h"
#include "spirv/spirv_reflect.h"
#include "File.h"

static void CreateReflectModules(Pipeline pipeline, PipelineConfigure config)
{
	pipeline->StageCount = config.ShaderCount;
	pipeline->Stages = malloc(pipeline->StageCount * sizeof(struct PipelineStage));
	
	for (int i = 0; i < pipeline->StageCount; i++)
	{
		pipeline->Stages[i].ShaderType = config.Shaders[i].Type;
		spvReflectCreateShaderModule(config.Shaders[i].DataSize, config.Shaders[i].Data, &pipeline->Stages[i].Module);
	}
}

static VkPushConstantRange GetPushConstantRange(Pipeline pipeline)
{
	VkPushConstantRange pushConstantRange = { 0 };
	for (int i = 0; i < pipeline->StageCount; i++)
	{
		unsigned int pushConstantCount;
		spvReflectEnumeratePushConstantBlocks(&pipeline->Stages[i].Module, &pushConstantCount, NULL);
		SpvReflectBlockVariable * pushConstants = malloc(pushConstantCount * sizeof(SpvReflectBlockVariable));
		spvReflectEnumeratePushConstantBlocks(&pipeline->Stages[i].Module, &pushConstantCount, &pushConstants);
		if (pushConstantCount > 0)
		{
			pipeline->UsesPushConstant = true;
			pipeline->PushConstantInfo = pushConstants[0];
			pipeline->PushConstantData = malloc(pushConstants[0].size);
			pipeline->PushConstantSize = pushConstants[0].size;
			return (VkPushConstantRange)
			{
				.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				.offset = pushConstants[0].offset,
				.size = pushConstants[0].size,
			};
		}
	}
	return pushConstantRange;
}

static void CreateDescriptorLayout(Pipeline pipeline, int * uboCount, int * samplerCount)
{
	unsigned int bindingCount = 0;
	for (int i = 0; i < pipeline->StageCount; i++)
	{
		struct PipelineStage * stage = pipeline->Stages + i;
		unsigned int setCount;
		spvReflectEnumerateDescriptorSets(&stage->Module, &setCount, NULL);
		SpvReflectDescriptorSet * sets = malloc(setCount * sizeof(SpvReflectDescriptorSet));
		spvReflectEnumerateDescriptorSets(&stage->Module, &setCount, &sets);
		
		stage->BindingCount = 0;
		if (setCount > 0)
		{
			pipeline->UsesDescriptors = true;
			stage->DescriptorInfo = sets[0];
			stage->BindingCount = stage->DescriptorInfo.binding_count;
			bindingCount += stage->DescriptorInfo.binding_count;
			printf("%i\n", bindingCount);
		}
	}
	
	VkDescriptorSetLayoutBinding * layoutBindings = malloc(bindingCount * sizeof(VkDescriptorSetLayoutBinding));
	for (int i = 0, c = 0; i < pipeline->StageCount; i++)
	{
		struct PipelineStage * stage = pipeline->Stages + i;
		unsigned int setCount;
		spvReflectEnumerateDescriptorSets(&stage->Module, &setCount, NULL);
		SpvReflectDescriptorSet * sets = malloc(setCount * sizeof(SpvReflectDescriptorSet));
		spvReflectEnumerateDescriptorSets(&stage->Module, &setCount, &sets);
		
		if (setCount > 0)
		{
			for (int j = 0; j < stage->DescriptorInfo.binding_count; j++, c++)
			{
				SpvReflectDescriptorBinding * binding = stage->DescriptorInfo.bindings[j];
				printf("set:%i binding:%i %s\n", binding->set, binding->binding, binding->name);
				layoutBindings[c] = (VkDescriptorSetLayoutBinding)
				{
					.binding = binding->binding,
					.descriptorCount = binding->count,
					.descriptorType = (VkDescriptorType)binding->descriptor_type,
					.stageFlags = stage->ShaderType,
				};
				if (binding->descriptor_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) { (*samplerCount)++; }
				if (binding->descriptor_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) { (*uboCount)++; }
			}
		}
	}
	
	VkDescriptorSetLayoutCreateInfo layoutInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = bindingCount,
		.pBindings = layoutBindings,
	};
	vkCreateDescriptorSetLayout(Graphics.Device, &layoutInfo, NULL, &pipeline->DescriptorLayout);
	free(layoutBindings);
}

static void CreateDescriptorSets(Pipeline pipeline)
{
	if (pipeline->UsesDescriptors)
	{
		pipeline->DescriptorSet = malloc(Graphics.FrameResourceCount * sizeof(VkDescriptorSet));
		for (int i = 0; i < Graphics.FrameResourceCount; i++)
		{
			VkDescriptorSetAllocateInfo allocateInfo =
			{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
				.descriptorPool = Graphics.DescriptorPool,
				.descriptorSetCount = 1,
				.pSetLayouts = &pipeline->DescriptorLayout,
			};
			vkAllocateDescriptorSets(Graphics.Device, &allocateInfo, pipeline->DescriptorSet + i);
		}
	}
}

static void CreatePipelineLayout(Pipeline pipeline, VkPushConstantRange pushConstantRange)
{
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = pipeline->UsesDescriptors ? 1 : 0,
		.pSetLayouts = &pipeline->DescriptorLayout,
		.pushConstantRangeCount = pipeline->UsesPushConstant ? 1 : 0,
		.pPushConstantRanges = &pushConstantRange,
	};
	vkCreatePipelineLayout(Graphics.Device, &pipelineLayoutCreateInfo, NULL, &pipeline->Layout);
}

static void CreateLayout(Pipeline pipeline, PipelineConfigure config)
{
	CreateReflectModules(pipeline, config);
	VkPushConstantRange pushConstantRange = GetPushConstantRange(pipeline);
	int samplerCount = 0;
	int uboCount = 0;
	CreateDescriptorLayout(pipeline, &uboCount, &samplerCount);
	CreatePipelineLayout(pipeline, pushConstantRange);
	CreateDescriptorSets(pipeline);
	//PipelineSetUniform(pipeline, ShaderStageVertex, 0, 0, "Transform", &Matrix4x4Identity, 0);
}

Pipeline PipelineCreate(PipelineConfigure config)
{
	Pipeline pipeline = malloc(sizeof(struct Pipeline));
	*pipeline = (struct Pipeline){ .VertexLayout = config.VertexLayout };
	
	VkPipelineShaderStageCreateInfo shaderInfos[5];
	VkShaderModule modules[5];
	for (int i = 0; i < config.ShaderCount; i++)
	{
		unsigned long size = 0;
		void * data = NULL;
		if (config.Shaders[i].LoadFromFile && config.Shaders[i].Precompiled)
		{
			File spv = FileOpen(config.Shaders[i].File, FileModeReadBinary);
			size = FileSize(spv);
			data = malloc(size);
			FileRead(spv, 0, size, data);
			FileClose(spv);
			config.Shaders[i].Data = data;
			config.Shaders[i].DataSize = size;
		}
		if (!config.Shaders[i].LoadFromFile && config.Shaders[i].Precompiled)
		{
			size = config.Shaders[i].DataSize;
			data = config.Shaders[i].Data;
		}
		
		VkShaderModuleCreateInfo moduleInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = size,
			.pCode = data,
		};
		vkCreateShaderModule(Graphics.Device, &moduleInfo, NULL, modules + i);
		
		shaderInfos[i] = (VkPipelineShaderStageCreateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = (VkShaderStageFlagBits)config.Shaders[i].Type,
			.module = modules[i],
			.pName = "main",
		};
	}
	
	VkPipelineVertexInputStateCreateInfo vertexInput =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &config.VertexLayout->Binding,
		.vertexAttributeDescriptionCount = config.VertexLayout->AttributeCount,
		.pVertexAttributeDescriptions = config.VertexLayout->Attributes,
	};
	
	VkPipelineInputAssemblyStateCreateInfo inputAssembly =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE,
	};
	
	VkViewport viewport =
	{
		.x = 0.0f,
		.y = 0.0f,
		.width = Swapchain.Extent.width,
		.height = Swapchain.Extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	
	VkRect2D scissor =
	{
		.offset = { 0, 0 },
		.extent = Swapchain.Extent,
	};

	VkPipelineViewportStateCreateInfo viewportState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor,
	};
	
	VkPipelineRasterizationStateCreateInfo rasterizationState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = config.WireFrame ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL,
		.lineWidth = 1.0f,
		.cullMode = config.FaceCull ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
	};
	
	VkPipelineMultisampleStateCreateInfo multisampleState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.sampleShadingEnable = VK_FALSE,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
	};
	
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState =
	{
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		.blendEnable = config.AlphaBlend ? VK_TRUE : VK_FALSE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
	};
	
	VkPipelineColorBlendStateCreateInfo colorBlendState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachmentState,
	};
	
	VkPipelineDepthStencilStateCreateInfo depthStencilState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = config.DepthTest ? VK_TRUE : VK_FALSE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
	};
	
	VkDynamicState dynamicStates[] =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};
	VkPipelineDynamicStateCreateInfo dynamicState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = 2,
		.pDynamicStates = dynamicStates,
	};
	
	CreateLayout(pipeline, config);
	VkGraphicsPipelineCreateInfo pipelineCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = config.ShaderCount,
		.pStages = shaderInfos,
		.pVertexInputState = &vertexInput,
		.pInputAssemblyState = &inputAssembly,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizationState,
		.pMultisampleState = &multisampleState,
		.pDepthStencilState = &depthStencilState,
		.pColorBlendState = &colorBlendState,
		.pDynamicState = &dynamicState,
		.layout = pipeline->Layout,
		.renderPass = Swapchain.RenderPass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1,
	};
	VkResult result = vkCreateGraphicsPipelines(Graphics.Device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL, &pipeline->Instance);
	if (result != VK_SUCCESS)
	{
		printf("[Error] Unable to create graphics pipeline: %i\n", result);
		exit(-1);
	}
	
	for (int i = 0; i < config.ShaderCount; i++)
	{
		vkDestroyShaderModule(Graphics.Device, modules[i], NULL);
	}
	return pipeline;
}

void PipelineSetPushConstant(Pipeline pipeline, const char * variable, void * value)
{
	if (pipeline->UsesPushConstant)
	{
		for (int i = 0; i < pipeline->PushConstantInfo.member_count; i++)
		{
			SpvReflectBlockVariable member = pipeline->PushConstantInfo.members[i];
			if (strcmp(member.name, variable) == 0)
			{
				memcpy(pipeline->PushConstantData + member.offset, value, member.size);
			}
		}
	}
}

void PipelineSetUniform(Pipeline pipeline, int binding, struct UniformBuffer * uniform)
{
	if (pipeline->UsesDescriptors)
	{
		for (int i = 0; i < Graphics.FrameResourceCount; i++)
		{
			VkDescriptorBufferInfo * bufferInfo = malloc(sizeof(VkDescriptorBufferInfo));
			*bufferInfo = (VkDescriptorBufferInfo)
			{
				.buffer = uniform->Buffer,
				.offset = 0,
				.range = uniform->Size,
			};
			
			VkWriteDescriptorSet * writeInfo = malloc(sizeof(VkWriteDescriptorSet));
			*writeInfo = (VkWriteDescriptorSet)
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.dstArrayElement = 0,
				.dstBinding = binding,
				.dstSet = pipeline->DescriptorSet[i],
				.pBufferInfo = bufferInfo,
			};
			ListPush(Graphics.FrameResources[i].UpdateDescriptorQueue, writeInfo);
		}
	}
}

void PipelineSetSampler(Pipeline pipeline, int binding, Texture texture)
{
	if (pipeline->UsesDescriptors)
	{
		for (int i = 0; i < Graphics.FrameResourceCount; i++)
		{
			VkDescriptorImageInfo * imageInfo = malloc(sizeof(VkDescriptorImageInfo));
			*imageInfo = (VkDescriptorImageInfo)
			{
				.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
				.sampler = texture->Sampler,
				.imageView = texture->ImageView,
			};
			VkWriteDescriptorSet * writeInfo = malloc(sizeof(VkWriteDescriptorSet));
			*writeInfo = (VkWriteDescriptorSet)
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.dstArrayElement = 0,
				.dstBinding = binding,
				.dstSet = pipeline->DescriptorSet[i],
				.pImageInfo = imageInfo,
			};
			ListPush(Graphics.FrameResources[i].UpdateDescriptorQueue, writeInfo);
		}
	}
}

void PipelineDestroy(Pipeline pipeline)
{
	vkDeviceWaitIdle(Graphics.Device);
	vkDestroyPipelineLayout(Graphics.Device, pipeline->Layout, NULL);
	free(pipeline->DescriptorSet);
	vkDestroyDescriptorSetLayout(Graphics.Device, pipeline->DescriptorLayout, NULL);
	if (pipeline->UsesPushConstant) { free(pipeline->PushConstantData); }
	spvReflectDestroyShaderModule(&pipeline->Stages[0].Module);
	spvReflectDestroyShaderModule(&pipeline->Stages[1].Module);
	free(pipeline->Stages);
	vkDestroyPipeline(Graphics.Device, pipeline->Instance, NULL);
	free(pipeline);
}
