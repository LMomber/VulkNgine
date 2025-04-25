#include "transform.h"

using namespace glm;

const glm::mat4& Transform::World()
{
    if (m_worldMatrixDirty)
    {
        const auto translation = glm::translate(glm::mat4(1.0f), m_translation);
        const auto rotation = glm::toMat4(m_rotation);
        const auto scale = glm::scale(glm::mat4(1.0f), m_scale);
 
        m_worldMatrix = translation * rotation * scale;
        m_worldMatrixDirty = false;
    }

    return m_worldMatrix;
}

void Transform::SetFromMatrix(const glm::mat4& m44)
{
    m_translation.x = m44[3][0];
    m_translation.y = m44[3][1];
    m_translation.z = m44[3][2];

    m_scale.x = length(vec3(m44[0][0], m44[0][1], m44[0][2]));
    m_scale.y = length(vec3(m44[1][0], m44[1][1], m44[1][2]));
    m_scale.z = length(vec3(m44[2][0], m44[2][1], m44[2][2]));

    mat4 myrot(m44[0][0] / m_scale.x,
        m44[0][1] / m_scale.x,
        m44[0][2] / m_scale.x,
        0,
        m44[1][0] / m_scale.y,
        m44[1][1] / m_scale.y,
        m44[1][2] / m_scale.y,
        0,
        m44[2][0] / m_scale.z,
        m44[2][1] / m_scale.z,
        m44[2][2] / m_scale.z,
        0,
        0,
        0,
        0,
        1);
    m_rotation = quat_cast(myrot);

    SetMatrixDirty();
}

void Transform::SetMatrixDirty()
{
    m_worldMatrixDirty = true;
}
