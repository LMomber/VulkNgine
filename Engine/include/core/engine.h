#pragma once

#include "pch.h"

#include "dataStructures.h"

class Device;
class Renderer;
class InputHandler;
namespace Core
{
	class Input; 
	class Engine 
	{
	public:
		void Initialize();
		void Update(double);
		void Render();
		void ShutDown();

		const Device& GetDevice() const;
		Renderer& GetRenderer() const;
		const Input& GetInput() const;
		GLFWwindow* GetWindow() const;
		entt::registry& GetRegistry();
		
		[[nodiscard]] uint32_t LoadModelFromFile(const std::string& filePath, Transform& transform);
		void FreeSceneObject(uint32_t sceneID, bool shouldBeDestroyed = false);
		void FreeNodeData(Node* node, bool shouldBeDestroyed);
	private:
		void UpdateChildMatrices(const glm::mat4& matrix, Node* parent);

		std::shared_ptr<Device> m_pDevice = nullptr;
		std::shared_ptr<Renderer> m_pRenderer = nullptr;
		std::shared_ptr<Input> m_pInput = nullptr;
		std::shared_ptr<InputHandler> m_pInputHandler = nullptr;

		entt::registry m_registry;

		// All elements in a scene
		std::vector<SceneData> m_sceneData;
		std::vector<uint32_t> m_freeList;
	};

	extern Engine engine;
}