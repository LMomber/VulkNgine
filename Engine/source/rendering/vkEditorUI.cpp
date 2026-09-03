#include "vkEditorUI.h"

#include "vkQueue.h"

#include "imgui/imgui_internal.h"

#include "glm/gtc/type_ptr.hpp"

#include "../core/engine.h"

EditorUI::EditorUI(std::shared_ptr<Device> device) :
	m_pDevice(device)
{
	ImGui::StyleColorsDark();
	ImGuiIO& io = ImGui::GetIO();

	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.IniFilename = "./imgui.ini";

	ImGuiStyle& style = ImGui::GetStyle();

	style.FrameRounding = 2.0f;

	m_OriginalCoutBuf = std::cout.rdbuf();
	consoleBuf.originalBuf = m_OriginalCoutBuf;

	std::cout.rdbuf(&consoleBuf);

	m_LastConsoleSize = consoleBuf.buffer.size();
}

void EditorUI::Render(CommandBuffer* commandBuffer, RenderTarget& renderTarget, entt::entity cameraEntity)
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();

	static ImGuizmo::OPERATION operation(ImGuizmo::ROTATE);
	static ImGuizmo::MODE mode(ImGuizmo::WORLD);

	ImGui::DockSpaceOverViewport();

	RenderEditorWindows(renderTarget.m_imguiTexture, cameraEntity, operation, mode);
	RecordCommandBuffer(commandBuffer, renderTarget);

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

void EditorUI::SetNodeToInspect(Node* pNode)
{
	m_pNode = pNode;

	if (m_pNode)
	{
		m_pTransform = &m_pNode->GetWorldTransform();
		m_selectedMatrix = m_pTransform->GetMatrix();
	}
	else
	{
		m_pTransform = nullptr;
	}
}

void EditorUI::RenderEditorWindows(const VkDescriptorSet gameTexture, entt::entity cameraEntity, ImGuizmo::OPERATION& operation, ImGuizmo::MODE& mode)
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
	GameViewport(gameTexture, cameraEntity, operation, mode);
	ImGui::End();

	ImGui::Begin("Inspector");
	InspectorUI(operation, mode);
	ImGui::End();

	// Console window
	ImGui::Begin("Console", nullptr);
	ConsoleUI();
	ImGui::End();

	ImGui::Render();
}

void EditorUI::RecordCommandBuffer(CommandBuffer* commandBuffer, RenderTarget& renderTarget)
{
	const auto& swapchain = m_pDevice->GetSwapchain();
	const VkImage swapchainImage = swapchain->GetCurrentImage();
	const VkImageView swapchainImageView = swapchain->GetCurrentImageView();
	const VkFormat swapchainFormat = swapchain->GetImageFormat();

	commandBuffer->TransitionImageLayout(renderTarget.m_image, renderTarget.m_format,
		renderTarget.m_imageLayout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	commandBuffer->TransitionImageLayout(swapchainImage, swapchainFormat, swapchain->GetCurrentImageLayout(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

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

	commandBuffer->TransitionImageLayout(swapchainImage, swapchainFormat, swapchain->GetCurrentImageLayout(), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}

void EditorUI::GameViewport(const VkDescriptorSet gameTexture, entt::entity cameraEntity, ImGuizmo::OPERATION& operation, ImGuizmo::MODE& mode)
{
	ImGuizmo::SetOrthographic(true);
	ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 avail = ImGui::GetContentRegionAvail();

	// Adjustments are necessary to get rid of padding
	pos.x -= 10;
	pos.y -= 10;
	avail.x += 15;
	avail.y += 30;

	ImGui::GetWindowDrawList()->AddImage((ImTextureID)gameTexture, pos, avail);
	ImGuizmo::SetRect(pos.x, pos.y, avail.x, avail.y);

	if (m_pTransform)
	{
		auto& cameraTransform = Core::engine.GetRegistry().get<Transform>(cameraEntity);
		const auto& camera = Core::engine.GetRegistry().get<Camera>(cameraEntity);

		const glm::vec3 trans = cameraTransform.GetTranslation();
		const glm::quat rot = cameraTransform.GetRotation();

		const glm::vec3 localForward = glm::vec3(0.f, 0.f, 1.f);
		const glm::vec3 forward = glm::normalize(glm::rotate(rot, localForward));

		const glm::vec3 focusPoint = trans + forward;
		const glm::vec3 worldUp = glm::vec3(0.f, 1.f, 0.f);

		glm::mat4 viewMatrix = glm::lookAtRH(trans, focusPoint, worldUp);
		glm::mat4 projectionMatrix = camera.projection;

		if (ImGuizmo::Manipulate(glm::value_ptr(viewMatrix),
			glm::value_ptr(projectionMatrix),
			operation,
			mode,
			glm::value_ptr(m_selectedMatrix),
			nullptr,
			m_useSnap ? &m_snap.x : nullptr))
		{
			m_pTransform->SetFromMatrix(m_selectedMatrix);
			Core::engine.UpdateNodeTransform(m_pNode);
		}
	}

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

	const bool wasAtBottom =
		ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f;

	ImGui::TextUnformatted(consoleBuf.buffer.c_str());

	if (consoleBuf.buffer.size() != m_LastConsoleSize)
	{
		if (wasAtBottom)
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

void EditorUI::EditTransform(ImGuizmo::OPERATION& operation, ImGuizmo::MODE& mode)
{
	if (ImGui::RadioButton("Translate", operation == ImGuizmo::TRANSLATE)) operation = ImGuizmo::TRANSLATE;
	ImGui::SameLine();
	if (ImGui::RadioButton("Rotate", operation == ImGuizmo::ROTATE)) operation = ImGuizmo::ROTATE;
	ImGui::SameLine();
	if (ImGui::RadioButton("Scale", operation == ImGuizmo::SCALE)) operation = ImGuizmo::SCALE;

	if (operation != ImGuizmo::SCALE)
	{
		if (ImGui::RadioButton("Local", mode == ImGuizmo::LOCAL)) mode = ImGuizmo::LOCAL;
		ImGui::SameLine();
		if (ImGui::RadioButton("World", mode == ImGuizmo::WORLD)) mode = ImGuizmo::WORLD;
	}

	ImGui::Checkbox("##something", &m_useSnap);
	ImGui::SameLine();

	switch (operation)
	{
	case ImGuizmo::TRANSLATE:
		m_snap = m_snapTranslation;
		ImGui::InputFloat3("Snap", &m_snap.x);
		break;
	case ImGuizmo::ROTATE:
		m_snap = m_snapRotation;
		ImGui::InputFloat("Angle Snap", &m_snap.x);
		break;
	case ImGuizmo::SCALE:
		m_snap = m_snapScale;
		ImGui::InputFloat("Scale Snap", &m_snap.x);
		break;
	default:
		break;
	}

	const auto& matrix = m_pTransform->GetMatrix();
	glm::vec3 position = matrix[3];

	float tempTr[3] = { position.x, position.y, position.z };
	if (ImGui::InputFloat3("Tr", tempTr))
	{
		m_pTransform->SetTranslation(glm::vec3(tempTr[0], tempTr[1], tempTr[2]));
		Core::engine.UpdateNodeTransform(m_pNode);
	}

	glm::vec3 tempRt{ 0.f };
	glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(glm::quat_cast(m_pTransform->GetMatrix())));
	tempRt[0] = eulerAngles.x;
	tempRt[1] = eulerAngles.y;
	tempRt[2] = eulerAngles.z;

	if (ImGui::InputFloat3("Rt", glm::value_ptr(tempRt)))
	{
		glm::vec3 newEulerAngles = glm::radians(glm::vec3(tempRt[0], tempRt[1], tempRt[2]));

		glm::vec3 translation = glm::vec3(m_pTransform->GetTranslation());
		glm::mat4 newRotationMatrix = glm::mat4(1.0f);

		newRotationMatrix = glm::rotate(newRotationMatrix, newEulerAngles.x, glm::vec3(1.0f, 0.0f, 0.0f));  // X-axis
		newRotationMatrix = glm::rotate(newRotationMatrix, newEulerAngles.y, glm::vec3(0.0f, 1.0f, 0.0f));  // Y-axis
		newRotationMatrix = glm::rotate(newRotationMatrix, newEulerAngles.z, glm::vec3(0.0f, 0.0f, 1.0f));  // Z-axis

		m_pTransform->SetFromMatrix(newRotationMatrix);
		m_pTransform->SetTranslation(translation);

		Core::engine.UpdateNodeTransform(m_pNode);
	}
}

void EditorUI::InspectorUI(ImGuizmo::OPERATION& operation, ImGuizmo::MODE& mode)
{
	if (m_pNode)
	{
		EditTransform(operation, mode);
	}
}