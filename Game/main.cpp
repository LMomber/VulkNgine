#include "engine.h"
#include "vkDevice.h"

#include "timer.h"
#include "transform.h"
#include "renderComponents.h"

#include <filesystem>
#include <iostream>
#include <cstdlib>

#define PRINT_FPS

#undef APIENTRY
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

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
#ifdef PRINT_FPS
		int fps = 0;
		double stopwatch = 0;
#endif

		long long dt = 0;
		float maxFrameTime = 0.016f;
		while (!glfwWindowShouldClose(engine.GetWindow()))
		{
			glfwPollEvents();
			dt = timer.GetDeltaTime(Unit::MICRO);

			double dt_double = static_cast<double>(dt);
			dt_double /= 1'000'000.f;
			if (dt_double > maxFrameTime) { dt_double = maxFrameTime; }

			engine.Update(dt_double);
			engine.Render();

#ifdef PRINT_FPS
			stopwatch += dt_double;
			fps++;

			if (stopwatch >= 1.f)
			{
				std::cout << "FPS: " << fps << std::endl;
				fps = 0;
				stopwatch = 0.0;
			}
#endif
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