#include "vkEditorUI.h"

#include "vkQueue.h"

#include "imgui/imgui_internal.h"

EditorUI::EditorUI(std::shared_ptr<Device> device) :
	m_pDevice(device)
{
	ImGui::StyleColorsDark();
	ImGuiIO& io = ImGui::GetIO();

	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.IniFilename = "./imgui.ini";

	ImGuiStyle& style = ImGui::GetStyle();

	style.FrameRounding = 2.0f;

	std::cout.rdbuf(&consoleBuf);
	m_LastConsoleSize = consoleBuf.buffer.size();
}

void EditorUI::Render(CommandBuffer* commandBuffer, const RenderTarget& renderTarget)
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();

	ImGui::DockSpaceOverViewport();

	RenderEditorWindows(renderTarget.m_imguiTexture);
	RecordCommandBuffer(commandBuffer, renderTarget);

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

void EditorUI::RenderEditorWindows(const VkDescriptorSet gameTexture)
{
	// Settings window
	ImGui::Begin("Settings");
	SettingsUI();
	ImGui::End();

	// Game viewport
	ImGui::Begin(
		"Game Viewport",
		nullptr,
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground);
	GameViewport(gameTexture);
	ImGui::End();

	// Console window
	ImGui::Begin("Console", nullptr);
	ConsoleUI();
	ImGui::End();

	ImGui::Render();
}

void EditorUI::RecordCommandBuffer(CommandBuffer* commandBuffer, const RenderTarget& renderTarget)
{
	const VkImage swapchainImage = m_pDevice->GetSwapchain()->GetCurrentImage();
	const VkImageView swapchainImageView = m_pDevice->GetSwapchain()->GetCurrentImageView();
	const VkFormat swapchainFormat = m_pDevice->GetSwapchain()->GetImageFormat();

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	commandBuffer->BeginCommandBuffer(&beginInfo);

	commandBuffer->TransitionImageLayout(renderTarget.m_image, renderTarget.m_format,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	commandBuffer->TransitionImageLayout(swapchainImage, swapchainFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	VkRenderingAttachmentInfo colorAttachment{};
	colorAttachment.sType =
		VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachment.imageView = swapchainImageView;
	colorAttachment.imageLayout =
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	VkRenderingInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.renderArea.offset = { 0, 0 };
	renderingInfo.renderArea.extent = renderTarget.m_extent;
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachment;

	commandBuffer->BeginRendering(&renderingInfo);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *commandBuffer->GetVkPtr());

	commandBuffer->EndRendering();

	commandBuffer->TransitionImageLayout(swapchainImage, swapchainFormat, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	commandBuffer->EndCommandBuffer();
}

void EditorUI::GameViewport(const VkDescriptorSet gameTexture)
{
	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 avail = ImGui::GetContentRegionAvail();

	// Adjustments are necessary to get rid of padding
	pos.x -= 10;
	pos.y -= 10;
	avail.x += 15;
	avail.y += 30;

	ImGui::GetWindowDrawList()->AddImage((ImTextureID)gameTexture, pos, avail);

	ImGuiStyle& style = ImGui::GetStyle();
	style.ImageBorderSize = 0.f;
	style.FramePadding = ImVec2(0, 0);
}

void EditorUI::SettingsUI()
{
	TimeSettings();
}

void EditorUI::ConsoleUI()
{
	ImGui::BeginChild("ConsoleScroll", ImVec2(0, 0), false);

	ImGui::TextUnformatted(consoleBuf.buffer.c_str());

	if (consoleBuf.buffer.size() != m_LastConsoleSize)
	{
		ImGui::SetScrollHereY(1.0f);
		m_LastConsoleSize = consoleBuf.buffer.size();
	}

	ImGui::EndChild();
}

void EditorUI::TimeSettings()
{
	ImGui::Button("StartTest");
    if (!m_startSimulation)
    {
        if (ImGui::Button("Start"))
        {
            m_startSimulation = true;
            m_stopTime = !m_stopTime;
        }
    }
    else
    {
        if (m_stopTime)
        {
            if (ImGui::Button("Resume"))
            {
                m_stopTime = !m_stopTime;
            }
            if (ImGui::Button("Single Step"))
            {
                m_singleStep = true;
            }
        }
        else
        {
            if (ImGui::Button("Pause"))
            {
                m_stopTime = !m_stopTime;
            }
        }
    }

    ImGui::SameLine();

    if (m_fixedStep)
    {
        if (ImGui::Button("Set Dynamic Timestep"))
        {
            m_fixedStep = false;
        }
    }
    else
    {
        if (ImGui::Button("Set Fixed Timestep"))
        {
            m_fixedStep = true;
        }
    }
}