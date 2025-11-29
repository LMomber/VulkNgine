#pragma once

#include "vkCommon.h"

// "using" instead of "typedef" for potential template usage.
using RID = uint64_t;

// enum class for strong typing
enum class ObjectType
{
	TYPE_ROOT,
	TYPE_MESH,
	TYPE_NONE
};

class SceneObject : public std::enable_shared_from_this<SceneObject>
{
public:
	SceneObject() = delete;
	SceneObject(ObjectType type, RID handle, std::shared_ptr<SceneObject> parent = nullptr) noexcept;

	void AddChild(ObjectType type, RID handle) noexcept;

	RID GetHandle() const noexcept;
	ObjectType GetType() const noexcept;

	std::shared_ptr<SceneObject> GetParent() const noexcept;
	const std::vector<std::shared_ptr<SceneObject>>& GetChildren() const noexcept;

private:
	// No defaults needed since the default constructor is deleted.
	ObjectType m_type;
	RID m_handle;

	std::shared_ptr<SceneObject> m_parent;
	std::vector<std::shared_ptr<SceneObject>> m_children;
};