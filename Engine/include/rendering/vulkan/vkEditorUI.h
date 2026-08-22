#pragma once

#include "pch.h"
#include "vkCommon.h"
#include "vkDevice.h"

#include "../../core/consoleBuffer.h"

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
    void Render(CommandBuffer* commandBuffer, const RenderTarget& renderTarget);

    // Simulation states
    bool m_startSimulation = false;
    bool m_stopTime = true;
    bool m_singleStep = false;
    bool m_fixedStep = true;

    bool m_haltInput = false;

private:
    std::shared_ptr<Device> m_pDevice;

    ImGuiConsoleBuf consoleBuf;
    size_t m_LastConsoleSize;

    void RenderEditorWindows(const VkDescriptorSet gameTexture);
    void RecordCommandBuffer(CommandBuffer* commandBuffer, const RenderTarget& renderTarget);

    // Setting up the separate UI windows
    void GameViewport(const VkDescriptorSet gameTexture);
    void SettingsUI();
    void ConsoleUI();

    void TimeSettings();
};