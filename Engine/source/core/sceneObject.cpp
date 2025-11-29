#include "sceneObject.h"

SceneObject::SceneObject(ObjectType type, RID handle, std::shared_ptr<SceneObject> parent) noexcept
	: m_type(type), m_handle(handle), m_parent(parent)
{
}

void SceneObject::AddChild(ObjectType type, RID handle) noexcept
{
	std::shared_ptr<SceneObject> child = std::make_shared<SceneObject>(type, handle, shared_from_this());
	m_children.emplace_back(child);
}

RID SceneObject::GetHandle() const noexcept
{
	return m_handle;
}

ObjectType SceneObject::GetType() const noexcept
{
	return m_type;
}

std::shared_ptr<SceneObject> SceneObject::GetParent() const noexcept
{
	return m_parent;
}

const std::vector<std::shared_ptr<SceneObject>>& SceneObject::GetChildren() const noexcept
{
	return m_children;
}

