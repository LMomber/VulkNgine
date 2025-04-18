#include "vkDevice.h"

#include <iostream>
#include <cstdlib>

int main() {
	Device device;

	device.Initialize();
	try
	{
		while (!glfwWindowShouldClose(device.GetWindow()))
		{
			glfwPollEvents();
		}

		device.Cleanup();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}