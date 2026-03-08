#pragma once

#include "vkCommon.h"
#include "vkWindow.h"
#include "vkSurface.h"
#include "vkSwapchain.h"
#include "vkCommandBuffer.h"

#include "rid.h"
#include "material.h"
#include "model.h"
#include "vkTexture.h"
#include "vkMesh.h"

#pragma warning(push, 0)
#include <vma/vk_mem_alloc.h>
#pragma warning(pop)

struct VmaAllocator_T;
typedef VmaAllocator_T* VmaAllocator;

enum SamplerType
{
	LinearRepeatAnisotropic
};

class PhysicalDevice;
class Queue;
class Device
{
public:
	Device() = default;

	void Initialize();
	void ShutDown();

	GLFWwindow* GetWindow() const;
	VkDevice GetVkDevice() const;
	VkInstance GetInstance() const;
	VkSurfaceKHR GetSurface() const;
	VkExtent2D GetExtent() const;

	std::shared_ptr<PhysicalDevice> GetPhysicalDevice() const;
	std::shared_ptr<Window> GetVkWindow() const;
	std::shared_ptr<Swapchain> GetSwapchain() const;
	std::shared_ptr<Queue> GetQueue() const;

	VkFormat GetVkFormat(CPU::TextureFormat format) const;

	const VmaAllocator& GetAllocator() const;
	const VkSampler GetSampler(const SamplerType type);

	uint32_t GetCurrentFrame() const;
	void AdvanceCurrentFrame();

	CommandBuffer BeginSingleTimeCommands(unsigned int currentFrame) const;
	void EndSingleTimeCommands(CommandBuffer commandBuffer) const;

	// VMA
	void CreateBuffer(VkDeviceSize size, VkBuffer& buffer, VmaAllocation& allocation, VkBufferUsageFlags bufferUsageFlags, VmaMemoryUsage memoryUsageFlags) const;

	template <typename T>
	void CreateBufferWithStaging(VkDeviceSize size, VkBuffer& buffer, VmaAllocation& allocation, std::vector<T>& bufferData, VkBufferUsageFlags usageFlag) const;
	void CreateBufferWithStaging(VkDeviceSize size, VkBuffer& buffer, VmaAllocation& allocation, void* bufferData, VkBufferUsageFlags usageFlag) const;

	void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags ImageUsageFlags, VmaMemoryUsage memoryUsageFlags, VkImage& image, VmaAllocation& imageAllocation) const;
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) const;
	//

	void CreateTextureImage(const std::shared_ptr<CPU::MaterialTexture> srcTexture, Vulkan::Texture& dstTexture, VkImageUsageFlagBits flags, VmaMemoryUsage memoryFlag, VkSharingMode sharingMode) const;
	void CreateTextureSampler(VkSampler& sampler, VkFilter filter, VkSamplerAddressMode addressMode, VkBool32 useAnisotropy) const;

	RID CreateGpuTexture(const std::shared_ptr<CPU::MaterialTexture> srcTexture, VkFormat format, VkImageAspectFlags aspectFlags, VkImageUsageFlagBits usageFlags, VmaMemoryUsage memoryFlags, VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE) const;
	void DestroyGpuTexture(const Vulkan::Texture& texture) const;
	RID CreateGpuMesh(const CPU::Mesh& mesh) const;
	void DestroyGpuMesh(const Vulkan::Mesh& mesh) const;

	VkDescriptorSetLayout CreateDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding>& layoutBindings) const;
	void CreateDescriptorSets(std::vector<VkDescriptorSet>& descriptorSets, VkDescriptorSetLayout layout, VkDescriptorPool descriptorPool) const;
	void UpdateDescriptorSets(std::vector<VkWriteDescriptorSet>& descriptorWrites) const;

	Device(const Device&) = delete;
	Device& operator=(const Device&) = delete;

private:

	void CreateInstance();
	void CreateLogicalDevice(QueueFamilyIndices indices);

	std::vector<const char*> GetRequiredExtensions() const;

	void ValidateExtensionAvailability(const std::vector<const char*>& inputExtensions) const;

	bool CheckValidationLayerSupport() const;

	// Debug Messenger
	void InitDebugMessenger();

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);
	static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) const;
	//

	std::shared_ptr<Swapchain> m_pSwapchain = nullptr;
	std::shared_ptr<Window> m_pVkWindow = nullptr;
	std::shared_ptr<PhysicalDevice> m_pPhysicalDevice = nullptr;
	std::shared_ptr<Queue> m_pQueue = nullptr;

	VmaAllocator m_allocator;

	std::unique_ptr<VK::Surface> m_pSurface = nullptr;

	VkInstance m_instance{};
	VkDevice m_device{};

	struct Samplers
	{
		VkSampler m_linearRepeatAnisotropic;
	} m_samplers;

	VkDebugUtilsMessengerEXT m_debugMessenger{};

	uint32_t m_currentFrame = 0;
};