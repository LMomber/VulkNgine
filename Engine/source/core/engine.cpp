#include "engine.h"

#include "vkDevice.h"
#include "vkRender.h"
#include "input.h"
#include "inputHandler.h"

#include "importer.h"
#include "assetStorage.h"
#include "../rendering/vulkan/vkModelStorage.h"

Core::Engine Core::engine;

void Core::Engine::Initialize()
{
	// Macro practice
	INIT_WRAPPER("input handler", m_pInputHandler = std::make_shared<InputHandler>());
	INIT_WRAPPER("device class",
		{
			m_pDevice = std::make_shared<Device>();
			m_pDevice->Initialize();
		};);
	INIT_WRAPPER("renderer", m_pRenderer = std::make_shared<Renderer>(m_pDevice));
	INIT_WRAPPER("input class",
		{
				std::shared_ptr<Core::Input> inputPtr(new Core::Input, Core::InputDelFunc);
				m_pInput = inputPtr;
		});
}

void Core::Engine::Update(double deltaTime)
{
	m_pInput->Update();
	m_pInputHandler->Update(deltaTime);

	static bool once = true;
	if (once)
	{
		// Update render transforms once
		for (auto& sceneData : m_sceneData)
		{
			Node* rootNode = &sceneData.m_node;

			glm::mat4 combinedMatrix = rootNode->GetWorldTransform().GetMatrix() * rootNode->GetLocalTransform().GetMatrix();
			UpdateChildMatrices(combinedMatrix, rootNode);
		}

		m_pRenderer->QueueObjectBufferUpdate();

		m_pRenderer->SetNodeToInspect(&m_sceneData[0].m_node);
		once = false;
	}

	m_pRenderer->Update();
}

void Core::Engine::Render()
{
	m_pRenderer->Render();
}

void Core::Engine::ShutDown()
{
	m_pInputHandler.reset(); // No dependencies
	m_pInput.reset();
	m_pRenderer.reset();
	m_pDevice->ShutDown();
	m_pDevice.reset();
}

const Device& Core::Engine::GetDevice() const
{
	assert(m_pDevice.get() && "Device is either uninitialized or deleted");
	return *m_pDevice.get();
}

Renderer& Core::Engine::GetRenderer() const
{
	assert(m_pRenderer.get() && "Renderer is either uninitialized or deleted");
	return *m_pRenderer.get();
}

const Core::Input& Core::Engine::GetInput() const
{
	assert(m_pInput.get() && "Input is either uninitialized or deleted");
	return *m_pInput.get();
}

GLFWwindow* Core::Engine::GetWindow() const
{
	auto* window = m_pDevice->GetWindow();
	assert(window && "Window is not instantiated");
	return window;
}

entt::registry& Core::Engine::GetRegistry()
{
	return m_registry;
}

void Core::Engine::UpdateNodeTransform(Node* pNode)
{
	if (!pNode->IsRoot())
	{
		throw std::runtime_error("node passed to UpdateNodeTransform() in not a root node.");
	}

	glm::mat4 combinedMatrix = pNode->GetWorldTransform().GetMatrix() * pNode->GetLocalTransform().GetMatrix();
	UpdateChildMatrices(combinedMatrix, pNode);

	m_pRenderer->QueueObjectBufferUpdate();
}

uint32_t Core::Engine::LoadModelFromFile(const std::string& filePath, Transform& transform)
{
	SceneData sceneData{ {Object(ObjectType::TYPE_ROOT)} };
	sceneData.m_node.GetWorldTransform().SetFromMatrix(transform.GetMatrix());
	sceneData.m_filepathHash = std::hash<std::string>{}(filePath);
	aiScene* pScene = nullptr;
	Importer::ImportScene(filePath, sceneData.m_node, pScene);

	uint32_t id = 0;
	if (m_freeList.empty())
	{
		m_sceneData.push_back(std::move(sceneData));
		id = static_cast<uint32_t>(m_sceneData.size()) - 1;
	}
	else
	{
		id = m_freeList.back();
		m_freeList.pop_back();

		m_sceneData[id] = std::move(sceneData);
	}

	return id;
}

void Core::Engine::FreeSceneObject(uint32_t sceneID, bool shouldBeDestroyed)
{
	if (shouldBeDestroyed)
	{
		throw std::logic_error("Gpu destruction is not currently supported, as counters are not in place.");
	}

	auto& sceneData = m_sceneData[sceneID];

	//Vulkan::MeshStorage::Get().DecrementUsageCount(sceneData.m_filepathHash);
	//AssetStorage::Get().DecrementRenderData(sceneData.m_filepathHash);
	FreeNodeData(&sceneData.m_node, shouldBeDestroyed);

	if (shouldBeDestroyed)
	{
		Vulkan::MeshStorage::Get().EraseMeshes(sceneData.m_filepathHash);
	}
}

void Core::Engine::FreeNodeData(Node* node, bool shouldBeDestroyed)
{
	auto& object = node->GetObject();
	if (object.GetType() != ObjectType::TYPE_ROOT)
	{
		if (object.GetType() != ObjectType::TYPE_MODEL)
		{
			throw std::logic_error("This function is currently only implemented for models.");
		}

		auto handle = object.GetHandle();
		const auto& renderIndices = AssetStorage::Get().GetRenderIndices(handle);
		m_pRenderer->FreeGpuModels(renderIndices, shouldBeDestroyed);
	}

	const auto& children = node->GetChildren();

	for (auto& child : children)
	{
		FreeNodeData(child.get(), shouldBeDestroyed);
	}
}

void Core::Engine::UpdateChildMatrices(const glm::mat4& matrix, Node* parent)
{
	const auto& children = parent->GetChildren();
	for (uint16_t i = 0; i < children.size(); i++)
	{
		const auto& childNode = children[i];
		const glm::mat4 combinedMatrix = matrix * childNode->GetLocalTransform().GetMatrix();

		const Object& object = childNode->GetObject();
		if (object.GetType() == ObjectType::TYPE_MODEL)
		{
			m_pRenderer->UpdateModelTransform(AssetStorage::Get().GetRenderIndices(object.GetHandle()), combinedMatrix);
		}

		UpdateChildMatrices(combinedMatrix, childNode.get());
	}
}
