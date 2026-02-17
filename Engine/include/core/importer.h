#pragma once

#include "pch.h"

#include "material.h"
#include "sceneObject.h"

// Only static functions
// Made it a class instead of a namespace to cleanly hide private functions
class Importer
{
public:
	static std::vector<char> ReadShaderFile(const std::string& filename);
	static void ImportScene(const std::string& pFile, Node& root, const aiScene* pScene);

	Importer() = delete;
	~Importer() = delete;
	Importer(const Importer&) = delete;
	Importer operator=(const Importer&) = delete;
	Importer(Importer&&) noexcept = delete;
	Importer& operator=(Importer&&) noexcept = delete;

private:
	static void CopyNodes(const aiScene& scene, const aiNode& node, Node& targetParent);
};