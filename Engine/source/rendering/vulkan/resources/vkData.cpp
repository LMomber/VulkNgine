#include "vkData.h"

Vulkan::Vertex::Vertex(glm::vec3 pos, glm::vec3 color, glm::vec2 uv0, glm::vec2 uv1)
	: m_pos(pos), m_color(color), m_uv0(uv0), m_uv1(uv1)
{
}
