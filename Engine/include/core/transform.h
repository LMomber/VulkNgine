#pragma once

#include <cassert>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <string>

struct Transform
{
	Transform() {}

	Transform(const glm::vec3& translation, const glm::vec3& scale, const glm::quat& rotation)
		: m_translation(translation), m_scale(scale), m_rotation(rotation)
	{
	}

	/// <summary>Gets this Transform's translation in local space.</summary>
	[[nodiscard]] inline const glm::vec3& GetTranslation() const { return m_translation; }
	/// <summary>Gets this Transform's scale in local space.</summary>
	[[nodiscard]] inline const glm::vec3& GetScale() const { return m_scale; }
	/// <summary>Gets this Transform's rotation in local space.</summary>
	[[nodiscard]] inline const glm::quat& GetRotation() const { return m_rotation; }

	/// <summary>Gets the matrix that transforms from local space to world space.
	/// Calling this function recomputes the matrix when necessary.</summary>
	[[nodiscard]] const glm::mat4& World();

	void SetTranslation(const glm::vec3& translation)
	{
		m_translation = translation;
		SetMatrixDirty();
	}

	void SetScale(const glm::vec3& scale)
	{
		m_scale = scale;
		SetMatrixDirty();
	}

	void SetRotation(const glm::quat& rotation)
	{
		m_rotation = rotation;
		SetMatrixDirty();
	}

	void SetFromMatrix(const glm::mat4& transform);

private:
	glm::vec3 m_translation = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::quat m_rotation = glm::identity<glm::quat>();

	glm::mat4 m_worldMatrix = glm::identity<glm::mat4>();
	bool m_worldMatrixDirty = true;

	void SetMatrixDirty();
};
