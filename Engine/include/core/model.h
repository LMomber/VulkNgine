#pragma once

//#include "rid.h"

#include "pch.h"

#include "common.h"
#include "material.h"

struct Mesh
{
public:
	Mesh() = delete;
	Mesh(const aiScene& aiScene, const aiMesh& aiMesh);

	const std::vector<glm::vec3>& GetVertices() const;
	const std::vector<glm::vec3>& GetNormals() const;
	const std::vector<std::vector<glm::vec2>>& GetTexCoords() const;
	const std::vector<uint32_t>& GetIndices() const;

	const Material& GetMaterial() const;

private:
	uint32_t m_numVertices = 0;
	uint32_t m_numFaces = 0;
	Material m_material;

	std::vector<glm::vec3> m_vertices{};
	std::vector<glm::vec3> m_normals{};
	std::vector<glm::vec3> m_tangents{};
	std::vector<glm::vec3> m_bitangents{};
	std::vector<std::vector<glm::vec2>> m_texCoords{};
	std::vector<uint32_t> m_indices{};
};

// Models should be uniquely stored in a MeshStorage. 
// If a model appears multiple times in a scene, indices to the same model are used. 
class Model
{
public:
	Model() = default;
	Model(const aiScene& aiScene, const aiNode& aiNode);

	std::vector<Mesh> m_meshes{};
private:
};