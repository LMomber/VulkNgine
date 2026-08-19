#include "engine.h"
#include "vkDevice.h"

#include "timer.h"
#include "transform.h"
#include "renderComponents.h"

#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <limits>

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

	{
		auto entity = registry.create();
		Camera& camera = registry.emplace<Camera>(entity);

		auto extent = engine.GetDevice().GetExtent();
		float aspectRatio = static_cast<float>(extent.width) / static_cast<float>(extent.height);
		camera.projection = glm::perspective(45.f, aspectRatio, 0.1f, 10000.f);
		camera.projection[1][1] *= -1;

		Transform& transform = registry.emplace<Transform>(entity);
		transform.SetTranslation(glm::vec3(0.f, 1.f, 0.f));
	}

	uint32_t sponza = UINT_MAX;
	uint32_t helmet1 = UINT_MAX;
	uint32_t helmet2 = UINT_MAX;

	{
		auto entity = registry.create();
		Transform& transform = registry.emplace<Transform>(entity);
		transform.SetTranslation(glm::vec3(0.f));

		std::string filePath = "../Engine/models/Sponza/glTF/Sponza.gltf";
		sponza = engine.LoadModelFromFile(filePath, transform);
	}

	{
		auto entity = registry.create();
		Transform& transform = registry.emplace<Transform>(entity);
		transform.SetTranslation(glm::vec3(2.f, 1.f, 0.f));
		transform.SetScale(glm::vec3(0.5f));
		transform.SetRotation(glm::angleAxis(90.f, glm::vec3(0.f, 1.f, 0.f)));

		std::string filePath = "../Engine/models/DamagedHelmet.glb";
		helmet1 = engine.LoadModelFromFile(filePath, transform);
	}

	{
		auto entity = registry.create();
		Transform& transform = registry.emplace<Transform>(entity);
		transform.SetTranslation(glm::vec3(0.f, 1.f, 0.f));

		std::string filePath = "../Engine/models/DamagedHelmet.glb";

		helmet2 = engine.LoadModelFromFile(filePath, transform);
	}

	//engine.FreeSceneObject(sponza, true);
	//engine.FreeSceneObject(helmet1, true);
	//engine.FreeSceneObject(helmet2);

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