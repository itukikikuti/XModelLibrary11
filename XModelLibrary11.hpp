// © 2017 itukikikuti
#pragma once

#include <fbxsdk.h>

#if defined(_WIN64)
#define A "x64/"
#else
#define A "x86/"
#endif

#if defined(_DEBUG)
#define B "debug/"
#else
#define B "release/"
#endif

#if defined(_DLL)
#define C "libfbxsdk-md.lib"
#else
#define C "libfbxsdk-mt.lib"
#endif

#pragma comment(lib, A B C)
#undef A
#undef B
#undef C

namespace XLibrary11 {

using namespace DirectX;

class Model
{
public:
	XLibrary11::Float3 position;
	XLibrary11::Float3 angles;
	XLibrary11::Float3 scale;
	std::vector<std::unique_ptr<XLibrary11::Mesh>> meshes;

	Model()
	{
		Initialize();
	}
	Model(const wchar_t* const filePath)
	{
		Initialize();
		Load(filePath);
	}
	virtual ~Model()
	{
	}
	void Load(const wchar_t* const filePath)
	{
		static std::unique_ptr<FbxManager, FbxManagerDeleter> manager(FbxManager::Create());

		size_t length = wcslen(filePath) + 1;
		std::unique_ptr<char[]> charFilePath(new char[length]);
		wcstombs_s(nullptr, charFilePath.get(), length, filePath, _TRUNCATE);

		std::unique_ptr<FbxImporter, FbxImporterDeleter> importer(FbxImporter::Create(manager.get(), ""));
		importer->Initialize(charFilePath.get(), -1, manager->GetIOSettings());

		std::unique_ptr<FbxScene, FbxSceneDeleter> scene(FbxScene::Create(manager.get(), ""));
		importer->Import(scene.get());

		FbxGeometryConverter converter(manager.get());
		converter.Triangulate(scene.get(), true);

		FbxNode* rootNode = scene->GetRootNode();
		LoadMeshAll(rootNode);

		LoadAnimation(scene.get(), rootNode, "Take 001");
	}
	void Draw()
	{
		frame++;
		if (animationMap["Take 001"].animationData.size() <= frame + 1)
			frame = 0;

		constant.bone = animationMap["Take 001"].animationData[frame][0];

		for (size_t i = 0; i < meshes.size(); i++)
		{
			meshes[i]->position = position;
			meshes[i]->angles = angles;
			meshes[i]->scale = scale;

			meshes[i]->Draw();
		}
	}

private:
	struct FbxManagerDeleter
	{
		void operator()(FbxManager* fbxManager) const
		{
			fbxManager->Destroy();
		}
	};
	struct FbxImporterDeleter
	{
		void operator()(FbxImporter* fbxImporter) const
		{
			fbxImporter->Destroy();
		}
	};
	struct FbxSceneDeleter
	{
		void operator()(FbxScene* fbxScene) const
		{
			fbxScene->Destroy();
		}
	};
	struct Animation
	{
		float length = 0.0f;
		std::vector<std::vector<XMMATRIX>> animationData;
	};
	struct Constant
	{
		XMMATRIX bone;
	};

	Constant constant;
	int frame;
	std::vector<std::string> boneList;
	std::unordered_map<std::string, Animation> animationMap;

	void Initialize()
	{
		XLibrary11::App::Initialize();

		position = XLibrary11::Float3(0.0f, 0.0f, 0.0f);
		angles = XLibrary11::Float3(0.0f, 0.0f, 0.0f);
		scale = XLibrary11::Float3(1.0f, 1.0f, 1.0f);

		frame = 0;
	}
	void LoadMeshAll(FbxNode *node)
	{
		LoadMesh(node);

		for (int i = 0; i < node->GetChildCount(); i++)
		{
			LoadMeshAll(node->GetChild(i));
		}
	}
	void LoadMesh(FbxNode *node)
	{
		if (!node->GetVisibility())
			return;

		FbxNodeAttribute* attribute = node->GetNodeAttribute();
		if (attribute == nullptr)
			return;

		if (attribute->GetAttributeType() == FbxNodeAttribute::eMesh)
		{
			FbxMesh* mesh = node->GetMesh();
			std::unique_ptr<XLibrary11::Mesh> item(new XLibrary11::Mesh());
			item->vertices.clear();
			item->indices.clear();

			for (int i = 0; i < mesh->GetPolygonCount(); i++)
			{
				int verticesSize = mesh->GetPolygonSize(i);

				for (int j = 0; j < verticesSize; j++)
				{
					int v = mesh->GetPolygonVertex(i, j);
					const DirectX::XMMATRIX transform = FbxMatrixToXMMatrix(node->EvaluateGlobalTransform());

					FbxVector4* fbxPosition = mesh->GetControlPoints();
					XLibrary11::Float3 position(static_cast<float>(-fbxPosition[v].mData[0]), static_cast<float>(-fbxPosition[v].mData[1]), static_cast<float>(-fbxPosition[v].mData[2]));
					DirectX::XMVector3TransformCoord(position, transform);

					FbxVector4 fbxNormal;
					mesh->GetPolygonVertexNormal(i, j, fbxNormal);
					XLibrary11::Float3 normal(static_cast<float>(-fbxNormal.mData[0]), static_cast<float>(-fbxNormal.mData[1]), static_cast<float>(-fbxNormal.mData[2]));
					DirectX::XMVector3TransformCoord(normal, transform);

					FbxStringList uvSetNames;
					mesh->GetUVSetNames(uvSetNames);
					FbxVector2 fbxUV;
					bool isMapped;
					if (mesh->GetElementUVCount() > 0)
						mesh->GetPolygonVertexUV(i, j, uvSetNames[0], fbxUV, isMapped);
					XLibrary11::Float2 uv(static_cast<float>(fbxUV.mData[0]), static_cast<float>(1.0f - fbxUV.mData[1]));

					item->vertices.push_back(XLibrary11::Vertex(position, normal, uv));
				}
			}
			item->Apply();
			item->SetCullingMode(D3D11_CULL_FRONT);
			item->GetMaterial().SetBuffer(2, &constant, sizeof(Constant));
			meshes.push_back(std::move(item));
		}

		if (attribute->GetAttributeType() == FbxNodeAttribute::eSkeleton)
		{
			boneList.push_back(node->GetName());
		}
	}
	void LoadAnimation(FbxScene *scene, FbxNode *node, std::string name)
	{
		FbxAnimStack *animStack = scene->GetSrcObject<FbxAnimStack>();
		if (animStack == nullptr)
			return;

		//FbxAnimLayer *animLayer = animStack->GetMember<FbxAnimLayer>();

		animationMap[name].length = animStack->GetLocalTimeSpan().GetDuration().GetMilliSeconds() / 1000.0f;
		int frameCount = (int)(animationMap[name].length * 60.0f);
		animationMap[name].animationData.resize(frameCount);

		for (int i = 0; i < frameCount; i++)
		{
			FbxTime time;
			time.SetMilliSeconds(i / 60.0f * 1000.0f);
			animationMap[name].animationData[i].resize(boneList.size());
			LoadBoneMatrixAll(node, name, i, time);
		}
	}
	void LoadBoneMatrixAll(FbxNode *node, std::string name, int index, FbxTime& time)
	{
		LoadBoneMatrix(node, name, index, time);

		for (int i = 0; i < node->GetChildCount(); i++)
		{
			LoadBoneMatrixAll(node->GetChild(i), name, index, time);
		}
	}
	void LoadBoneMatrix(FbxNode *node, std::string name, int index, FbxTime& time)
	{
		FbxNodeAttribute* attribute = node->GetNodeAttribute();

		if (attribute == nullptr)
			return;

		if (attribute->GetAttributeType() != FbxNodeAttribute::eMesh)
			return;

		FbxMesh* mesh = node->GetMesh();
		int skinCount = mesh->GetDeformerCount(FbxDeformer::eSkin);

		for (int i = 0; i < skinCount; i++)
		{
			FbxSkin* skin = (FbxSkin*)mesh->GetDeformer(i, FbxDeformer::eSkin);
			int clusterCount = skin->GetClusterCount();

			for (int j = 0; j < clusterCount; j++)
			{
				FbxCluster* cluster = skin->GetCluster(j);
				if (cluster->GetLink() == nullptr)
					continue;

				std::string boneName = cluster->GetLink()->GetName();
				int boneIndex = 0;

				for (boneIndex = 0; boneIndex < boneList.size(); boneIndex++)
				{
					if (boneName.compare(boneList[boneIndex]) == 0)
						break;
				}

				FbxAMatrix offsetMatrix = node->EvaluateGlobalTransform(time);
				FbxAMatrix modelMatrix = GetModelMatrix(node);
				FbxAMatrix worldMatrix = offsetMatrix * modelMatrix;

				XMMATRIX boneMatrix = GetClusterMatrix(worldMatrix, mesh, cluster, time);
				XMMATRIX scaling = XMMatrixScaling(0.015f, 0.015f, 0.015f);
				boneMatrix = boneMatrix * scaling;

				animationMap[name].animationData[index][boneIndex] = boneMatrix;
			}
		}
	}
	static FbxAMatrix GetModelMatrix(FbxNode* node)
	{
		FbxVector4 translation = node->GetGeometricTranslation(FbxNode::eSourcePivot);
		FbxVector4 rotation = node->GetGeometricRotation(FbxNode::eSourcePivot);
		FbxVector4 scaling = node->GetGeometricScaling(FbxNode::eSourcePivot);

		return FbxAMatrix(translation, rotation, scaling);
	}
	static DirectX::XMMATRIX GetClusterMatrix(FbxAMatrix& worldMatrix, FbxMesh* mesh, FbxCluster* cluster, FbxTime& time)
	{
		XLibrary11::Float4 plane(1.0f, 0.0f, 0.0f, 0.0f);
		FbxAMatrix reflection = FbxMatrixReflect(plane);

		FbxAMatrix clusterTransformMatrix;
		cluster->GetTransformMatrix(clusterTransformMatrix);
		clusterTransformMatrix = reflection * clusterTransformMatrix;

		FbxAMatrix meshModelMatrix = GetModelMatrix(mesh->GetNode());
		meshModelMatrix = reflection * meshModelMatrix;

		clusterTransformMatrix *= meshModelMatrix;

		FbxAMatrix clusterTransformLinkMatrix;
		cluster->GetTransformLinkMatrix(clusterTransformLinkMatrix);
		clusterTransformLinkMatrix = reflection * clusterTransformLinkMatrix;

		FbxAMatrix clusterOffsetMatrix = cluster->GetLink()->EvaluateGlobalTransform(time);
		clusterOffsetMatrix = reflection * clusterOffsetMatrix;

		FbxAMatrix clusterTransformInverseMatrix = clusterTransformLinkMatrix * clusterTransformMatrix;

		FbxAMatrix clusterInverseMatrix = worldMatrix * clusterOffsetMatrix;

		return FbxMatrixToXMMatrix(clusterInverseMatrix * clusterTransformInverseMatrix);
	}
public:
	static DirectX::XMMATRIX FbxMatrixToXMMatrix(FbxMatrix source)
	{
		DirectX::XMMATRIX destination;
		for (int x = 0; x < 4; x++)
		{
			for (int y = 0; y < 4; y++)
			{
				destination.r[x].m128_f32[y] = static_cast<float>(source.mData[x][y]);
			}
		}
		return destination;
	}
	static FbxAMatrix FbxMatrixReflect(XLibrary11::Float4 plane)
	{
		plane = XMPlaneNormalize(plane);
		FbxAMatrix reflection;

		reflection.mData[0][0] = -2.0f * plane.x * plane.x + 1.0f;
		reflection.mData[0][1] = -2.0f * plane.y * plane.x;
		reflection.mData[0][2] = -2.0f * plane.z * plane.x;
		reflection.mData[0][3] = 0.0f;
		reflection.mData[1][0] = -2.0f * plane.x * plane.y;
		reflection.mData[1][1] = -2.0f * plane.y * plane.y + 1.0f;
		reflection.mData[1][2] = -2.0f * plane.z * plane.y;
		reflection.mData[1][3] = 0.0f;
		reflection.mData[2][0] = -2.0f * plane.x * plane.z;
		reflection.mData[2][1] = -2.0f * plane.y * plane.z;
		reflection.mData[2][2] = -2.0f * plane.z * plane.z + 1.0f;
		reflection.mData[2][3] = 0.0f;
		reflection.mData[3][0] = -2.0f * plane.x * plane.w;
		reflection.mData[3][1] = -2.0f * plane.y * plane.w;
		reflection.mData[3][2] = -2.0f * plane.z * plane.w;
		reflection.mData[3][3] = 1.0f;

		return reflection;
	}
};

}
