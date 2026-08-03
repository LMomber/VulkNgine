#include "vkDevice.h"

#include "vkPhysicalDevice.h"
#include "vkQueue.h"

#include "vkResourceManager.h"

#pragma warning(push, 0)
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>
#pragma warning(pop)

#include <iostream>
#include <set>
#include <algorithm>
#include <fstream>
#include <string>

const std::vector<const char*> validationLayers =
{
	"VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool g_enableValidationLayers = false;
#else 
const bool g_enableValidationLayers = true;
#endif

void Device::Initialize()
{
	m_pVkWindow = std::make_shared<Window>();
	CreateInstance();
	InitDebugMessenger();
	m_pSurface = std::make_unique<VK::Surface>(m_instance, m_pVkWindow->GetWindow());
	m_pPhysicalDevice = std::make_unique<PhysicalDevice>(m_instance, m_pSurface->GetSurface());

	QueueFamilyIndices indices = m_pPhysicalDevice->FindQueueFamilies(m_pPhysicalDevice->GetDevice(), GetSurface());

	CreateLogicalDevice(indices);
	m_pSwapchain = std::make_shared<Swapchain>(m_device, m_pSurface->GetSurface(), m_pVkWindow, m_pPhysicalDevice);
	m_pQueue = std::make_shared<Queue>(m_device, indices);

	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = m_pPhysicalDevice->GetDevice();
	allocatorInfo.device = m_device;
	allocatorInfo.instance = m_instance;

	vmaCreateAllocator(&allocatorInfo, &m_allocator);

	CreateTextureSampler(m_samplers.m_linearRepeatAnisotropic, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_TRUE);
}

void Device::ShutDown()
{
	vkDestroySampler(m_device, m_samplers.m_linearRepeatAnisotropic, nullptr);

	m_pQueue.reset();
	m_pSwapchain.reset();

	vmaDestroyAllocator(m_allocator);

	vkDestroyDevice(m_device, nullptr);

	if (g_enableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
	}

	m_pSurface.reset();

	vkDestroyInstance(m_instance, nullptr);

	m_pVkWindow.reset();

	glfwTerminate();
}

void Device::InitDebugMessenger()
{
	if (!g_enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	PopulateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to set up debug messenger!");
	}
}

void Device::CreateInstance()
{
	if (!CheckValidationLayerSupport())
	{
		throw std::runtime_error("Validation layers requested, but not available");
	}

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "VulkNgine";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "VulkNgine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_4;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	auto extensions = GetRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	// Rids warning on MacOS, but removes compatibility with RenderDoc
	//createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (g_enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		PopulateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;

		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create instance");
	}
}

GLFWwindow* Device::GetWindow() const
{
	ASSERT_GLFW_WINDOW_PTR(m_pVkWindow->GetWindow());
	return m_pVkWindow->GetWindow();
}

VkDevice Device::GetVkDevice() const
{
	ASSERT_VK_LOGICAL_DEVICE(m_device);
	return m_device;
}

VkInstance Device::GetInstance() const
{
	ASSERT_VK_INSTANCE(m_instance);
	return m_instance;
}

VkSurfaceKHR Device::GetSurface() const
{
	ASSERT_VK_SURFACE_CLASS(m_pSurface);
	return m_pSurface->GetSurface();
}

std::shared_ptr<Window> Device::GetVkWindow() const
{
	ASSERT_VK_WINDOW_CLASS(m_pVkWindow);
	return m_pVkWindow;
}

std::shared_ptr<PhysicalDevice> Device::GetPhysicalDevice() const
{
	assert(m_pPhysicalDevice && "Physical device is either uninitialized or deleted");
	return m_pPhysicalDevice;
}

std::shared_ptr<Swapchain> Device::GetSwapchain() const
{
	ASSERT_VK_SWAPCHAIN_CLASS(m_pSwapchain);
	return m_pSwapchain;
}

std::shared_ptr<Queue> Device::GetQueue() const
{
	ASSERT_VK_QUEUE_CLASS(m_pQueue);
	return m_pQueue;
}

VkExtent2D Device::GetExtent() const
{
	ASSERT_VK_SWAPCHAIN_CLASS(m_pSwapchain);
	return m_pSwapchain->GetExtent();
}

const VmaAllocator& Device::GetAllocator() const
{
	return m_allocator;
}

const VkSampler Device::GetSampler(const SamplerType type)
{
	switch (type)
	{
	case LinearRepeatAnisotropic:
		return m_samplers.m_linearRepeatAnisotropic;
		break;
	default:
		throw std::logic_error("Sampler not implemented yet.");
		break;
	}
}

uint32_t Device::GetCurrentFrame() const
{
	return m_currentFrame;
}

void Device::AdvanceCurrentFrame()
{
	m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Device::CreateLogicalDevice(QueueFamilyIndices indices)
{

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.m_graphicsFamily.value(), indices.m_presentFamily.value(), indices.m_transferFamily.value() };

	float queuePriority = 1.f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceTimelineSemaphoreFeatures timelineFeatures{};
	timelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
	timelineFeatures.timelineSemaphore = VK_TRUE;
	timelineFeatures.pNext = nullptr;

	VkPhysicalDeviceFeatures features{};
	features.samplerAnisotropy = VK_TRUE;

	VkPhysicalDeviceVulkan13Features features13{};
	features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	features13.synchronization2 = VK_TRUE;
	features13.dynamicRendering = VK_TRUE;
	features13.pNext = &timelineFeatures;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pEnabledFeatures = &features;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	createInfo.pNext = &features13;

	if (g_enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(m_pPhysicalDevice->GetDevice(), &createInfo, nullptr, &m_device) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create logical device");
	}
}

std::vector<const char*> Device::GetRequiredExtensions() const
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (g_enableValidationLayers)
	{
		glfwExtensionCount++;
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	std::vector<const char*> requiredGlfwExtensions;

	for (uint32_t i = 0; i < glfwExtensionCount; i++)
	{
		requiredGlfwExtensions.emplace_back(extensions[i]);
	}

	// Rids warning on MacOS, but removes compatibility with RenderDoc
	//requiredGlfwExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

	ValidateExtensionAvailability(requiredGlfwExtensions);

	return requiredGlfwExtensions;
}

void Device::ValidateExtensionAvailability(const std::vector<const char*>& inputExtensions) const
{
	assert((inputExtensions.size() > 0) && "input extension array is empty");

	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensions(extensionCount);

	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	for (const auto& input : inputExtensions)
	{
		bool isAvailable = false;
		for (const auto& extension : extensions)
		{
			if (strcmp(input, extension.extensionName) == 0)
			{
				isAvailable = true;
				break;
			}
		}

		if (!isAvailable)
		{
			throw std::runtime_error("extension is not available");
		}
	}
}

bool Device::CheckValidationLayerSupport() const
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers)
	{
		bool layerFound = false;
		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}
	}

	return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL Device::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*)
{
	switch (messageSeverity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		std::cout << "INFO: ";
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		std::cout << "WARNING: ";
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		std::cout << "ERROR: ";
		break;
	default:
		return VK_FALSE;
		break;
	}

	std::cout << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

VkResult Device::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void Device::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

void Device::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) const
{
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
	createInfo.pUserData = nullptr; // Optional
}

CommandBuffer Device::BeginSingleTimeCommands(unsigned int currentFrame) const
{
	const QueueType type = QueueType::GRAPHICS;
	const auto queue = GetQueue();

	const auto& commandBuffer = queue->CreateSingleTimeCommandBuffer(type, currentFrame);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	commandBuffer.BeginCommandBuffer(&beginInfo);

	return commandBuffer;
}

void Device::EndSingleTimeCommands(CommandBuffer commandBuffer) const
{
	commandBuffer.EndCommandBuffer();

	const QueueType type = QueueType::GRAPHICS;

	const auto vkCommandBuffer = commandBuffer.GetVkPtr();

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = vkCommandBuffer;

	auto queue = GetQueue();

	vkQueueSubmit(queue->GetQueue(type), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue->GetQueue(type));
}

RID Device::CreateGpuTexture(const std::shared_ptr<CPU::MaterialTexture> srcTexture, VkFormat format, VkImageAspectFlags aspectFlags, VkImageUsageFlagBits usageFlags, VmaMemoryUsage memoryFlags, VkSharingMode sharingMode) const
{
	if (!srcTexture)
	{
		return RID();
	}

	Vulkan::Texture dstTexture{};
	CreateTextureImage(srcTexture, dstTexture, usageFlags, memoryFlags, sharingMode);
	dstTexture.m_imageView = CreateImageView(dstTexture.m_image, format, aspectFlags);

	return Vulkan::ResourceManager::Get().textureOwner.CreateRID(dstTexture);
}

VkDescriptorSetLayout Device::CreateDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding>& layoutBindings) const
{
	VkDescriptorSetLayoutCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	info.bindingCount = static_cast<uint32_t>(layoutBindings.size());
	info.pBindings = layoutBindings.data();

	VkDescriptorSetLayout layout;
	vkCreateDescriptorSetLayout(m_device, &info, nullptr, &layout);

	return layout;
}

// TODO: Get rid of MAX_FRAMES_IN_FLIGHT
void Device::CreateDescriptorSets(std::vector<VkDescriptorSet>& descriptorSets, VkDescriptorSetLayout layout, VkDescriptorPool descriptorPool) const
{
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, layout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	if (vkAllocateDescriptorSets(m_device, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate descriptor sets");
	}
}

void Device::UpdateDescriptorSets(std::vector<VkWriteDescriptorSet>& descriptorWrites) const
{
	vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Device::DestroyGpuTexture(const Vulkan::Texture& texture) const
{
	const auto vkDevice = m_device;
	vkDestroyImageView(vkDevice, texture.m_imageView, nullptr);
	vmaDestroyImage(GetAllocator(), texture.m_image, texture.m_allocation);
}

RID Device::CreateGpuMesh(const CPU::Mesh& mesh) const
{
	Vulkan::Mesh vkMesh;
	const auto& verts = mesh.GetVertices();
	const auto& normals = mesh.GetNormals();
	const auto& tangents = mesh.GetTangents();
	const auto& coords = mesh.GetTexCoords();

	vkMesh.m_vertices.reserve(verts.size());
	for (uint32_t i = 0; i < verts.size(); i++)
	{
		vkMesh.m_vertices.emplace_back(verts[i], normals[i], glm::vec3(tangents[i]), coords[0][i], coords.size() > 1 ? coords[1][i] : glm::vec2{0, 0});
	}
	const VkDeviceSize vertexBufferSize = sizeof(vkMesh.m_vertices[0]) * vkMesh.m_vertices.size();
	CreateBufferWithStaging(vertexBufferSize, vkMesh.m_vertexBuffer, vkMesh.m_vertexAllocation, vkMesh.m_vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

	vkMesh.m_indices = mesh.GetIndices();
	const VkDeviceSize indexBufferSize = sizeof(vkMesh.m_indices[0]) * vkMesh.m_indices.size();
	CreateBufferWithStaging(indexBufferSize, vkMesh.m_indexBuffer, vkMesh.m_indexAllocation, vkMesh.m_indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

	return Vulkan::ResourceManager::Get().meshOwner.CreateRID(vkMesh);
}

void Device::DestroyGpuMesh(const Vulkan::Mesh& mesh) const
{
	vmaDestroyBuffer(GetAllocator(), mesh.m_vertexBuffer, mesh.m_vertexAllocation);
	vmaDestroyBuffer(GetAllocator(), mesh.m_indexBuffer, mesh.m_indexAllocation);
}

VkImageView Device::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) const
{
	VkImageView imageView;

	VkImageViewCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = format;
	createInfo.subresourceRange.aspectMask = aspectFlags;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(m_device, &createInfo, nullptr, &imageView) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create image view");
	}

	return imageView;
}

VkFormat Device::GetVkFormat(CPU::TextureFormat format) const
{
	switch (format)
	{
	case CPU::TextureFormat::RGBA8_UNORM:
		return VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
		break;
	case CPU::TextureFormat::RGB8_UNORM:
		return VkFormat::VK_FORMAT_R8G8B8_UNORM;
		break;
	case CPU::TextureFormat::RG8_UNORM:
		return VkFormat::VK_FORMAT_R8G8_UNORM;
		break;
	case CPU::TextureFormat::R8_UNORM:
		return VkFormat::VK_FORMAT_R8_UNORM;
		break;
	case CPU::TextureFormat::SRGBA8:
		return VkFormat::VK_FORMAT_R8G8B8A8_SRGB;
		break;
	default:
		throw std::logic_error("Non-specified format used.");
		break;
	}
}

void Device::CreateBuffer(VkDeviceSize size, VkBuffer& buffer, VmaAllocation& allocation, VkBufferUsageFlags bufferUsageFlags, VmaMemoryUsage memoryUsageFlags, VmaAllocationCreateFlags memoryCreateFlags) const
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = bufferUsageFlags;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = memoryUsageFlags;
	allocInfo.flags = memoryCreateFlags;

	vmaCreateBuffer(GetAllocator(), &bufferInfo, &allocInfo,
		&buffer, &allocation, nullptr);
}

void Device::CreateAndMapBuffer(void*& mappedBuffer, VkDeviceSize size, VkBuffer& buffer, VmaAllocation& allocation, VkBufferUsageFlags bufferUsageFlags, VmaMemoryUsage memoryUsageFlags, VmaAllocationCreateFlags memoryCreateFlags) const
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = bufferUsageFlags;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocCreateInfo{};
	allocCreateInfo.usage = memoryUsageFlags;
	allocCreateInfo.flags = memoryCreateFlags;

	VmaAllocationInfo allocInfo{};
	if (vmaCreateBuffer(GetAllocator(), &bufferInfo, &allocCreateInfo, &buffer, &allocation, &allocInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("Memory allocation failed");
	}

	mappedBuffer = allocInfo.pMappedData;
}

template <typename T>
void Device::CreateBufferWithStaging(VkDeviceSize size, VkBuffer& buffer, VmaAllocation& allocation, std::vector<T>& bufferData, VkBufferUsageFlags usageFlag) const
{
	// Create staging buffer
	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	{
		CreateBuffer(size, stagingBuffer, stagingAllocation, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

		void* data;
		vmaMapMemory(GetAllocator(), stagingAllocation, &data);
		memcpy(data, bufferData.data(), static_cast<size_t>(size));
		vmaUnmapMemory(GetAllocator(), stagingAllocation);
	}

	// Create vertex buffer in device local memory
	CreateBuffer(size, buffer, allocation, usageFlag | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

	CommandBuffer commandBuffer = BeginSingleTimeCommands(GetCurrentFrame());
	commandBuffer.CopyBuffer(stagingBuffer, buffer, size);
	EndSingleTimeCommands(commandBuffer);

	// Cleanup staging
	vmaDestroyBuffer(GetAllocator(), stagingBuffer, stagingAllocation);
}

void Device::CreateBufferWithStaging(VkDeviceSize size, VkBuffer& buffer, VmaAllocation& allocation, void* bufferData, VkBufferUsageFlags usageFlag) const
{
	// Create staging buffer
	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	{
		CreateBuffer(size, stagingBuffer, stagingAllocation, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

		void* data;
		vmaMapMemory(GetAllocator(), stagingAllocation, &data);
		memcpy(data, bufferData, static_cast<size_t>(size));
		vmaUnmapMemory(GetAllocator(), stagingAllocation);
	}

	// Create vertex buffer in device local memory
	CreateBuffer(size, buffer, allocation, usageFlag | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

	CommandBuffer commandBuffer = BeginSingleTimeCommands(GetCurrentFrame());
	commandBuffer.CopyBuffer(stagingBuffer, buffer, size);
	EndSingleTimeCommands(commandBuffer);

	// Cleanup staging
	vmaDestroyBuffer(GetAllocator(), stagingBuffer, stagingAllocation);
}

void Device::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags ImageUsageFlags, VmaMemoryUsage memoryUsageFlags, VkImage& image, VmaAllocation& imageAllocation) const
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | ImageUsageFlags;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = memoryUsageFlags;

	vmaCreateImage(GetAllocator(), &imageInfo, &allocInfo,
		&image, &imageAllocation, nullptr);
}

void Device::CreateTextureImage(const std::shared_ptr<CPU::MaterialTexture> srcTexture, Vulkan::Texture& dstTexture, VkImageUsageFlagBits flags, VmaMemoryUsage memoryFlag, VkSharingMode sharingMode) const
{
	VkDeviceSize imageSize = srcTexture->m_width * srcTexture->m_height * 4;

	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation{};
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = imageSize;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

		vmaCreateBuffer(GetAllocator(), &bufferInfo, &allocInfo,
			&stagingBuffer, &stagingAllocation, nullptr);

		void* data;
		vmaMapMemory(GetAllocator(), stagingAllocation, &data);
		memcpy(data, srcTexture->m_pixels.data(), srcTexture->m_pixels.size());
		vmaUnmapMemory(GetAllocator(), stagingAllocation);
	}

	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = static_cast<uint32_t>(srcTexture->m_width);
		imageInfo.extent.height = static_cast<uint32_t>(srcTexture->m_height);
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = GetVkFormat(srcTexture->m_format);;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | flags;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = sharingMode;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = memoryFlag;

		vmaCreateImage(GetAllocator(), &imageInfo, &allocInfo,
			&dstTexture.m_image, &dstTexture.m_allocation, nullptr);
	}

	CommandBuffer commandBuffer = BeginSingleTimeCommands(GetCurrentFrame());
	commandBuffer.TransitionImageLayout(dstTexture.m_image, GetVkFormat(srcTexture->m_format),
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	commandBuffer.CopyBufferToImage(stagingBuffer, dstTexture.m_image,
		static_cast<uint32_t>(srcTexture->m_width), static_cast<uint32_t>(srcTexture->m_height));
	commandBuffer.TransitionImageLayout(dstTexture.m_image, GetVkFormat(srcTexture->m_format),
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	EndSingleTimeCommands(commandBuffer);

	vmaDestroyBuffer(GetAllocator(), stagingBuffer, stagingAllocation);
}

void Device::CreateTextureSampler(VkSampler& sampler, VkFilter filter, VkSamplerAddressMode addressMode, VkBool32 useAnisotropy) const
{
	auto properties = GetPhysicalDevice()->GetProperties();

	VkSamplerCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	createInfo.magFilter = filter;
	createInfo.minFilter = filter;
	createInfo.addressModeU = addressMode;
	createInfo.addressModeV = addressMode;
	createInfo.addressModeW = addressMode;
	createInfo.anisotropyEnable = useAnisotropy;
	createInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	createInfo.unnormalizedCoordinates = VK_FALSE;
	createInfo.compareEnable = VK_FALSE;
	createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	createInfo.mipLodBias = 0.0f;
	createInfo.minLod = 0.0f;
	createInfo.maxLod = 0.0f;

	if (vkCreateSampler(m_device, &createInfo, nullptr, &sampler) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create sampler");
	}
}
