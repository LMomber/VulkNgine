#pragma once

#include "pch.h"

#include "transform.h"
#include "common.h"

// enum class for strong typing
enum class ObjectType
{
	TYPE_ROOT,
	TYPE_MODEL,
	TYPE_NONE
};

class Object : public std::enable_shared_from_this<Object>
{
public:
	Object() = default;
	Object(ObjectType type, AssetID id = AssetID()) noexcept;

	AssetID GetHandle() const noexcept;
	void SetHandle(AssetID rid) noexcept;
	ObjectType GetType() const noexcept;
	void SetType(ObjectType type) noexcept;

private:
	ObjectType m_type = ObjectType::TYPE_NONE;
	AssetID m_id{ std::numeric_limits<size_t>().max() };
};

// Using raw pointers to avoid reference counting (ref counting == safe but slow).
class Node : public std::enable_shared_from_this<Node>
{
public:
    Node(const Object& object, Node* parent = nullptr) noexcept;

    Node* AddChild(const Object& object);

    const Object& GetObject() const noexcept;
    void SetObject(const Object& object) noexcept;

    Node* GetParent() const noexcept;
    void SetParent(Node* parent) noexcept;

	Transform& GetLocalTransform() noexcept;
	void SetLocalTransform(const Transform& transform) noexcept;

	Transform& GetWorldTransform() noexcept;
	void SetWorldTransform(const Transform& transform) noexcept;

    const std::vector<std::unique_ptr<Node>>& GetChildren() const noexcept;

    bool IsRoot() const noexcept;
    void SetRoot(bool root = true) noexcept;

private:
    Node* m_parent = nullptr;

    Object m_object;

	Transform m_localTransform;
	Transform m_worldTransform;

    bool m_root = false;

    std::vector<std::unique_ptr<Node>> m_children{};
};

// A scene element
struct SceneData
{
	Node m_node;
	size_t m_filepathHash;
};
