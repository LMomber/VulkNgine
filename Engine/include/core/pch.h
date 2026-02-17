#pragma once

#include "glm/glm.hpp"

#include "Assimp/Importer.hpp"
#include "Assimp/Scene.h"
#include "Assimp/postprocess.h"

#pragma warning(push)
#pragma warning(disable : 4267) // Deprecated declarations
#include <entt/entity/registry.hpp>
#pragma warning(pop)

// TODO: Move this include to VUlkan specific
#define GLFW_INCLUDE_VULKAN
#include "glfw/include/GLFW/glfw3.h"
//

#include <stb/stb_image.h>

#include <filesystem>
#include <cstdint>
#include <vector>
#include <array>
#include <memory>
#include <fstream>
#include <optional>
#include <set>
#include <iostream>
#include <unordered_map>
#include <cassert>
#include <stdexcept>