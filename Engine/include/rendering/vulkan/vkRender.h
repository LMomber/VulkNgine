#pragma once

#include "vkCommon.h"
#include "vkDevice.h"
#include "vkEditorUI.h"

#include "vkData.h"

#include "../../core/dataStructures.h"

#pragma warning(push, 0)
#include <vma/vk_mem_alloc.h>
#pragma warning(pop)

struct SceneRenderData
{
	std::vector<Vulkan::Model> m_meshesToRender;
	std::vector<uint32_t> m_freeList;
};

class CommandBuffer;
class Pipeline;
class Renderer
{
public:
	Renderer(std::shared_ptr<Device> device);
	~Renderer();

	void Update();
	void Render();

	std::vector<uint32_t> CreateGpuModel(const CPU::Model& model, size_t hash);
	void FreeGpuModels(const std::vector<uint32_t>& ids, bool shouldBeDestroyed);
	void UpdateModelTransform(const std::vector<uint32_t>& ids, const glm::mat4& worldMatrix);
	void QueueObjectBufferUpdate();

	const std::vector<uint32_t> AddMeshesToRenderList(const std::vector<Vulkan::Model>& meshes);

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;
private:
	void CreateMaterialDescriptorSetLayout();
	void CreateGraphicsPipeline();
	void CreateBuffers();
	void CreateSyncObjects();
	void CreateFallbackMaterial();
	void CreateRenderTargets();
	void DestroyRenderTargets();

	void ChooseSharingMode();

	void UpdateBuffers(const int currentFrame);

	VkShaderModule CreateShaderModule(const std::vector<char>& code);

	void RecordCommandBuffer(CommandBuffer commandBuffer, uint32_t imageIndex) const;

	void DestroyGpuModel(const Vulkan::Model& model);

	std::shared_ptr<Device> m_pDevice;
	std::shared_ptr<Pipeline> m_pipeline;
	std::shared_ptr<EditorUI> m_pEditorUI;

	SceneRenderData m_sceneRenderData;

	std::array<RenderTarget, MAX_FRAMES_IN_FLIGHT> m_renderTargets;

	// PBR materials
	VkDescriptorSetLayout m_materialDescriptorSetLayout;
	Vulkan::Material fallbackMaterial;

	// Scene buffers
	VkDescriptorSetLayout m_sceneBuffersDescriptorSetLayout;
	std::vector<VkDescriptorSet> m_sceneBuffersDescriptorSets;

	std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> m_cameraBuffers;
	std::array<VmaAllocation, MAX_FRAMES_IN_FLIGHT> m_cameraAllocations;
	std::array<void*, MAX_FRAMES_IN_FLIGHT> m_mappedCameraBuffers;

	std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> m_objectBuffers;
	std::array<VmaAllocation, MAX_FRAMES_IN_FLIGHT> m_objectAllocations;
	std::array<void*, MAX_FRAMES_IN_FLIGHT> m_mappedObjectBuffers;
	std::array<bool, MAX_FRAMES_IN_FLIGHT> m_objectBufferUpdate{ true };

	std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> m_lightBuffers;
	std::array<VmaAllocation, MAX_FRAMES_IN_FLIGHT> m_lightAllocations;
	std::array<void*, MAX_FRAMES_IN_FLIGHT> m_mappedLightBuffers;
	std::array<uint32_t, MAX_FRAMES_IN_FLIGHT> m_lightCounts{ 0 };
	//

	std::vector<VkSemaphore> m_renderFinishedPerImage;

	std::array<FrameContext, MAX_FRAMES_IN_FLIGHT> m_frameContexts{};

	std::vector<uint32_t> m_queueSetIndices;
	VkSharingMode m_sharingMode;
};