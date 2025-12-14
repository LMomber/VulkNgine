#include "sceneObject.h"

RID SceneObject::GetHandle() const noexcept
{
	return m_handle;
}

ObjectType SceneObject::GetType() const noexcept
{
	return m_type;
}

SceneNode::SceneNode(SceneObject* object, SceneNode* parent) noexcept
	: m_object(object), m_parent(parent)
{}

SceneNode* SceneNode::AddChild(SceneObject* object)
{
	std::unique_ptr<SceneNode> child = std::make_unique<SceneNode>(object, this); 
	SceneNode* raw_ptr = child.get(); // Get the raw ptr because child is invalid after move
	m_children.emplace_back(std::move(child));
	return raw_ptr;
}

SceneObject* SceneNode::GetObject() const noexcept
{
	return m_object;
}

SceneNode* SceneNode::GetParent() const noexcept
{
	return m_parent;
}

const std::vector<std::unique_ptr<SceneNode>>& SceneNode::GetChildren() const noexcept
{
	return m_children;
}
