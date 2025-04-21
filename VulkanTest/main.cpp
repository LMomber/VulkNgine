#include "engine.h"

#include "vkDevice.h"

#include <iostream>
#include <cstdlib>

int main() {
	Core::Engine& engine = Core::engine;
	engine.Initialize();

	try
	{
		while (!glfwWindowShouldClose(engine.GetDevice().GetWindow()))
		{
			glfwPollEvents();
			engine.Update();
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