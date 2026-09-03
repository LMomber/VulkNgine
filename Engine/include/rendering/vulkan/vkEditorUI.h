#pragma once

#include "pch.h"
#include "vkCommon.h"
#include "vkDevice.h"
#include "../renderComponents.h"

#include "../../core/transform.h"
#include "../../core/consoleBuffer.h"
#include "../../core/dataStructures.h"

#include "imgui/imgui_impl_vulkan.h"
#include "imgui/imgui_impl_glfw.h"

#pragma warning(push, 0)
#include "imGuizmo/ImGuizmo.h"
#pragma warning(pop)

class EditorUI
{
public:
	EditorUI(std::shared_ptr<Device> device);

    // Render all windows
    void Render(CommandBuffer* commandBuffer, RenderTarget& renderTarget, entt::entity cameraEntity);
    void SetNodeToInspect(Node* pNode);

private:
    std::shared_ptr<Device> m_pDevice;

    std::streambuf* m_OriginalCoutBuf = nullptr;
    ImGuiConsoleBuf consoleBuf;
    size_t m_LastConsoleSize;

    Node* m_pNode = nullptr;

    void RenderEditorWindows(const VkDescriptorSet gameTexture, entt::entity cameraEntity, ImGuizmo::OPERATION& operation, ImGuizmo::MODE& mode);
    void RecordCommandBuffer(CommandBuffer* commandBuffer, RenderTarget& renderTarget);

    // Setting up the separate UI windows
    void GameViewport(const VkDescriptorSet gameTexture, entt::entity cameraEntity, ImGuizmo::OPERATION& operation, ImGuizmo::MODE& mode);
    void SettingsUI();
    void ConsoleUI();
    void InspectorUI(ImGuizmo::OPERATION& operation, ImGuizmo::MODE& mode);

    void TimeSettings();

    // Render & edit gizmo
    void EditTransform(ImGuizmo::OPERATION& operation, ImGuizmo::MODE& mode);

    Transform* m_pTransform = nullptr;

    // Snap values for gizmo transformations
    glm::vec3 m_snapTranslation{ 1.f, 1.f, 1.f };
    glm::vec3 m_snapRotation{ 1.f, 1.f, 1.f };
    glm::vec3 m_snapScale{ 1.f, 1.f, 1.f };
    glm::vec3 m_snap{ 1.f, 1.f, 1.f };

    // Matrix to use for gizmo transformations
    glm::mat4 m_selectedMatrix = glm::mat4(1.0f);

    // Whether to use snap or not
    bool m_useSnap = false;

    // Simulation states
    bool m_startSimulation = false;
    bool m_stopTime = true;
    bool m_singleStep = false;
    bool m_fixedStep = true;

    bool m_haltInput = false;
};