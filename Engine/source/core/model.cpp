#include "model.h"
#include "material.h"

#include "assetStorage.h"

using namespace CPU;

static glm::vec3 ToGlmVec3(const aiVector3t<float>& v)
{
	return glm::vec3(v.x, v.y, v.z);
}

static glm::vec2 ToGlmVec2(const aiVector3t<float>& v)
{
	return glm::vec2(v.x, v.y);
}

Mesh::Mesh(const aiScene& aiScene, const aiMesh& aiMesh)
{
	m_numVertices = aiMesh.mNumVertices;
	m_numFaces = aiMesh.mNumFaces;
	m_material = Material(aiScene, aiMesh);
		//= AssetStorage::Get().CreateMaterial(aiScene, aiMesh);

	m_vertices.reserve(m_numVertices);
	m_normals.reserve(m_numVertices);
	m_tangents.reserve(m_numVertices);
	m_bitangents.reserve(m_numVertices);

	std::vector<uint32_t> uvChannels;
	for (uint32_t channel = 0; channel < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++channel)
	{
		if (aiMesh.HasTextureCoords(channel))
			uvChannels.push_back(channel);
	}

	m_texCoords.reserve(uvChannels.size());
	for (uint32_t i = 0; i < uvChannels.size(); i++)
	{
		std::vector<glm::vec2>& uvSet = m_texCoords.emplace_back();
		uvSet.reserve(m_numVertices);
	}

	for (uint32_t i = 0; i < m_numVertices; i++)
	{
		m_vertices.push_back(ToGlmVec3(aiMesh.mVertices[i]));

		if (aiMesh.HasNormals())
		{
			m_normals.push_back(ToGlmVec3(aiMesh.mNormals[i]));
		}

		if (aiMesh.HasTangentsAndBitangents())
		{
			m_tangents.push_back(ToGlmVec3(aiMesh.mTangents[i]));
			m_bitangents.push_back(ToGlmVec3(aiMesh.mBitangents[i]));
		}

		for (uint32_t j = 0; j < uvChannels.size(); j++)
		{
			m_texCoords[j].push_back(ToGlmVec2(aiMesh.mTextureCoords[uvChannels[j]][i]));
		}
	}

	for (uint32_t i = 0; i < m_numFaces; i++)
	{
		auto& face = aiMesh.mFaces[i];
		for (uint32_t j = 0; j < face.mNumIndices; j++)
		{
			m_indices.push_back(face.mIndices[j]);
		}
	}
}

const std::vector<glm::vec3>& Mesh::GetVertices() const
{
	return m_vertices;
}

const std::vector<glm::vec3>& Mesh::GetNormals() const
{
	return m_normals;
}

const std::vector<std::vector<glm::vec2>>& Mesh::GetTexCoords() const
{
	return m_texCoords;
}

const std::vector<uint32_t>& Mesh::GetIndices() const
{
	return m_indices;
}

const Material& Mesh::GetMaterial() const
{
	return m_material;
}

Model::Model(const aiScene& aiScene, const aiNode& aiNode)
{
	assert(aiNode.mNumMeshes > 0);
	
	for (uint16_t i = 0; i < aiNode.mNumMeshes; i++)
	{
		m_meshes.emplace_back(aiScene, *aiScene.mMeshes[aiNode.mMeshes[i]]);
	}
}
