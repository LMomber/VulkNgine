#include "sceneObject.h"

Object::Object(ObjectType type, AssetID id) noexcept
	: m_type(type), m_id(id)
{}

AssetID Object::GetHandle() const noexcept
{
	return m_id;
}

void Object::SetHandle(AssetID id) noexcept
{
	m_id = id;
}

ObjectType Object::GetType() const noexcept
{
	return m_type;
}

void Object::SetType(ObjectType type) noexcept
{
	m_type = type;
}

Node::Node(const Object& object, Node* parent) noexcept
	: m_object(object), m_parent(parent)
{}

Node* Node::AddChild(const Object& object)
{
	std::unique_ptr<Node> child = std::make_unique<Node>(object, this); 
	Node* raw_ptr = child.get(); // Get the raw ptr because child becomes empty after move
	m_children.emplace_back(std::move(child));
	return raw_ptr;
}

const Object& Node::GetObject() const noexcept
{
	return m_object;
}

void Node::SetObject(const Object& object) noexcept
{
	m_object = object;
}

Node* Node::GetParent() const noexcept
{
	return m_parent;
}

void Node::SetParent(Node* parent) noexcept
{
	m_parent = parent;
}

const std::vector<std::unique_ptr<Node>>& Node::GetChildren() const noexcept
{
	return m_children;
}

Transform& Node::GetTransform() noexcept
{
	return m_transform;
}

void Node::SetTransform(const Transform& transform) noexcept
{
	m_transform = transform;
}

bool Node::IsRoot() const noexcept
{
	return m_root;
}

void Node::SetRoot(bool root) noexcept
{
	m_root = root;
}
