#include "importer.h"

#include "engine.h"
#include "assetStorage.h"

#include "../rendering/vulkan/vkRender.h" // Make platform agnostic later!

static Transform AssimpMatrixToTransform(const aiMatrix4x4& mat)
{
	aiVector3t<float> scaling;
	aiVector3t<float> rotation;
	aiVector3t<float> position;
	mat.Decompose(scaling, rotation, position);

	Transform transform;
	const glm::vec3 pos = { position.x, position.y, position.z };
	const glm::vec3 rot = { rotation.x, rotation.y, rotation.z };
	const glm::vec3 scale = { scaling.x, scaling.y, scaling.z };
	transform.SetTranslation(pos);
	transform.SetRotation(rot);
	transform.SetScale(scale);

	return transform;
}

void Importer::CopyNodes(const std::string& filePath, const aiScene& scene, const aiNode& node, Node& targetParent)
{
	// Empty node 
	Node* nodeToPass = &targetParent;

	// Currently only supports mesh objects
	// If node has meshes, create a new scene object for it 
	if (node.mNumMeshes > 0)
	{
		CPU::Model model(filePath, scene, node);
		size_t hash = std::hash<std::string>{}(filePath);

		std::vector<uint32_t> meshIndices = Core::engine.GetRenderer().CreateGpuModel(model, hash);

		static auto& assetStorage = AssetStorage::Get();
		ModelID modelID = assetStorage.AddToRenderIndices(meshIndices);

		if (modelID.m_id == std::numeric_limits<size_t>::max())
		{
			throw std::runtime_error("ID is larger than the numerical limit of size_t, something is wrong here.");
		}

		Node* pNode = targetParent.AddChild(Object{ ObjectType::TYPE_MODEL, assetStorage.CreateAssetID(modelID) });

		//The new object is the parent for all child nodes 
		nodeToPass = pNode;
	}

	nodeToPass->SetLocalTransform(AssimpMatrixToTransform(node.mTransformation));

	// continue for all child nodes 
	for (unsigned int i = 0; i < node.mNumChildren; i++)
	{
		CopyNodes(filePath, scene, *node.mChildren[i], *nodeToPass);
	}
}

std::vector<char> Importer::ReadShaderFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

// Customized function from: https://the-asset-importer-lib-documentation.readthedocs.io/en/latest/usage/use_the_lib.html
void Importer::ImportScene(const std::string& filePath, Node& root, const aiScene* pScene)
{
	// Create an instance of the Importer class
	Assimp::Importer importer;

	unsigned int importFlags = aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_FlipUVs |
		aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType |
		aiProcess_OptimizeMeshes;
#ifdef _DEBUG
	importFlags |= aiProcess_ValidateDataStructure;
#endif

	// And have it read the given file with some example postprocessing
	// Usually - if speed is not the most important aspect for you - you'll
	// probably to request more postprocessing than we do in this example.
	pScene = importer.ReadFile(filePath, importFlags);

	// If the import failed, report it
	if (nullptr == pScene)
	{
		throw std::logic_error(importer.GetErrorString());
	}

	root.SetRoot();
	aiNode* pAssimpNode = pScene->mRootNode;
	CopyNodes(filePath, *pScene, *pAssimpNode, root);
}