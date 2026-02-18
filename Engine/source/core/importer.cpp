#include "importer.h"

#include "sceneObject.h"

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

void Importer::CopyNodes(const aiScene& scene, const aiNode& node, Node& targetParent)
{
	// Empty node
	Node* parent = &targetParent;
	Transform transform;

	//If node has meshes, create a new scene object for it
	if (node.mNumMeshes > 0)
	{
		ModelID meshID = AssetStorage::Get().CreateModel(scene, node);
		Node* pNode = targetParent.AddChild(Object{ ObjectType::TYPE_MESH, AssetStorage::Get().CreateAssetID(meshID)});
		transform = AssimpMatrixToTransform(node.mTransformation);

		//The new object is the parent for all child nodes
		parent = pNode;
	}
	else
	{
		// if no meshes, skip the node, but keep its transformation
		if (!targetParent.IsRoot())
		{
			parent->SetParent(&targetParent);
		}
		transform.SetFromMatrix(AssimpMatrixToTransform(node.mTransformation).GetWorld() * parent->GetTransform().GetWorld());
	}

	parent->SetTransform(transform);

	// continue for all child nodes
	for (unsigned int i = 0; i < node.mNumChildren; i++)
	{
		CopyNodes(scene, *node.mChildren[i], *parent);
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
void Importer::ImportScene(const std::string& pFile, Node& root, const aiScene* pScene)
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
	pScene = importer.ReadFile(pFile, importFlags);

	// If the import failed, report it
	if (nullptr == pScene)
	{
		throw std::logic_error(importer.GetErrorString());
	}

	root.SetRoot();
	aiNode* pAssimpNode = pScene->mRootNode;
	root.GetTransform().SetFromMatrix(AssimpMatrixToTransform(pAssimpNode->mTransformation).GetWorld() * root.GetTransform().GetWorld());
	CopyNodes(*pScene, *pAssimpNode, root);
	//DoThepSceneProcessing(ppScene);
}