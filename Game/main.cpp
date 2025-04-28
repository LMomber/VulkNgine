#include "engine.h"
#include "vkDevice.h"

#include "timer.h"
#include "transform.h"
#include "renderComponents.h"

#undef APIENTRY
#include <Windows.h>

#include <filesystem>
#include <iostream>
#include <cstdlib>

// TODO: Add cross-platform support
static void SetWorkingDirectory()
{
	char buffer[MAX_PATH] = { 0 };
	GetModuleFileNameA(nullptr, buffer, MAX_PATH);

	// Vulkan\Game\$(platform)\$(config)\game.exe
	std::filesystem::path executablePath(buffer);

	// Vulkan\Game
	auto gameFolderPath = executablePath.parent_path().parent_path().parent_path();

	SetCurrentDirectoryA(gameFolderPath.string().c_str());
}

int main() {
	SetWorkingDirectory();

	Core::Engine& engine = Core::engine;
	INIT_WRAPPER("engine",
		{
			engine.Initialize();
		});

	Timer timer;

	auto& registry = engine.GetRegistry();
	auto entity = registry.create();
	Camera& camera = registry.emplace<Camera>(entity);

	auto extent = engine.GetDevice().GetExtent();
	float aspectRatio = static_cast<float>(extent.width) / static_cast<float>(extent.height);
	camera.projection = glm::perspective(45.f, aspectRatio, 0.1f, 1000.f);
	camera.projection[1][1] *= -1;

	Transform& cameraTransform = registry.emplace<Transform>(entity);
	cameraTransform.SetTranslation(glm::vec3(1, 2, 2));

	try
	{
		while (!glfwWindowShouldClose(engine.GetWindow()))
		{
			glfwPollEvents();
			engine.Update(timer.GetDeltaTime(Unit::MILLI)/1000.f);
			engine.Render();
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	engine.ShutDown();

	return EXIT_SUCCESS;
}