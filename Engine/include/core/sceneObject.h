#pragma once

#include "vkCommon.h"
#include "rid.h"

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
	SceneObject(ObjectType type, RID handle) noexcept : m_type(type), m_handle(handle) {
	}

	RID GetHandle() const noexcept;
	ObjectType GetType() const noexcept;

private:
	// No defaults needed since the default constructor is deleted.
	ObjectType m_type;
	RID m_handle;
};

// Using raw pointers to avoid reference counting (ref counting == safe but slow).
class SceneNode : public std::enable_shared_from_this<SceneNode>
{
public:
	SceneNode(SceneObject* object = nullptr, SceneNode* parent = nullptr) noexcept;

	SceneNode* AddChild(SceneObject* object = nullptr);

	SceneObject* GetObject() const noexcept;
	SceneNode* GetParent() const noexcept;
	const std::vector<std::unique_ptr<SceneNode>>& GetChildren() const noexcept;

private:
	SceneObject* m_object = nullptr;
	SceneNode* m_parent = nullptr;
	std::vector<std::unique_ptr<SceneNode>> m_children{};
};