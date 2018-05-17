// © 2017 itukikikuti
// https://github.com/itukikikuti/XModelLibrary11
// MIT License
#pragma once

#include <fbxsdk.h>
#include "XLibrary11.hpp"

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

XLIBRARY_NAMESPACE_BEGIN

using namespace DirectX;
using namespace XLibrary11;

class Model2
{
public:
	Float3 position;
	Float3 angles;
	Float3 scale;

	Model2()
	{
		Initialize();
	}
	Model2(const wchar_t* const filePath)
	{
		Initialize();
		Load(filePath);
	}
	~Model2()
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

		FbxAxisSystem::DirectX.ConvertScene(scene.get());

		//FbxSystemUnit systemUnit(1.0);
		//systemUnit.ConvertScene(scene.get());
		
		FbxGeometryConverter converter(manager.get());
		converter.Triangulate(scene.get(), true);

		SearchNode(scene.get(), scene->GetRootNode());
	}
	void Play()
	{
		startTime = App::GetTime();
	}
	void Draw()
	{
		float time = App::GetTime() - startTime;
		int frame = (int)(time * 60.0f);
		frame %= frameCount;

		for (int i = 0; i < boneCount; i++)
		{
			constant.bones[i] = XMMatrixTranspose(
				bones[frame][i]
			);
		}

		for (int i = 0; i < meshes.size(); i++)
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
	struct Constant
	{
		XMMATRIX bones[200];
	};
	Constant constant;
	std::vector<std::unique_ptr<Mesh>> meshes;
	std::vector<std::vector<XMMATRIX>> bones;
	float startTime = 0.0f;
	int frameCount = 0;
	int boneCount = 0;

	void Initialize()
	{
		App::Initialize();

		position = Float3(0.0f, 0.0f, 0.0f);
		angles = Float3(0.0f, 0.0f, 0.0f);
		scale = Float3(1.0f, 1.0f, 1.0f);
	}
	void SearchNode(FbxScene* scene, FbxNode* node)
	{
		LoadNode(scene, node);

		for (int i = 0; i < node->GetChildCount(); i++)
		{
			SearchNode(scene, node->GetChild(i));
		}
	}
	void LoadNode(FbxScene* scene, FbxNode* node)
	{
		if (!node->GetVisibility())
			return;

		FbxNodeAttribute* attribute = node->GetNodeAttribute();

		if (attribute == nullptr)
			return;

		if (attribute->GetAttributeType() == FbxNodeAttribute::eMesh)
			LoadMesh(scene, node->GetMesh());
	}
	bool IsOptimized(FbxMesh* mesh)
	{
		if (mesh->GetElementNormal() != nullptr)
		{
			if (mesh->GetElementNormal()->GetMappingMode() != FbxLayerElement::EMappingMode::eByControlPoint)
				return false;
		}

		if (mesh->GetElementUV() != nullptr)
		{
			if (mesh->GetElementUV()->GetMappingMode() != FbxLayerElement::EMappingMode::eByControlPoint)
				return false;
		}

		return true;
	}
	void LoadMesh(FbxScene* scene, FbxMesh* mesh)
	{
		std::unique_ptr<Mesh> item(new Mesh());
		item->vertices.clear();
		item->indices.clear();

		bool isOptimized = IsOptimized(mesh);

		if (isOptimized)
		{
			LoadMeshWithControlPoint(mesh, item.get());
		}
		else
		{
			LoadMeshWithPolygonVertex(mesh, item.get());
		}

		LoadAnim(scene, mesh, item.get(), isOptimized);

		item->Apply();
		item->GetMaterial().Load(L"assets/test.hlsl");
		item->GetMaterial().SetBuffer(2, &constant, sizeof(Constant));

		meshes.push_back(std::move(item));
	}
	void LoadMeshWithControlPoint(FbxMesh* mesh, Mesh* item)
	{
		item->vertices.resize(mesh->GetControlPointsCount());

		for (int i = 0; i < mesh->GetControlPointsCount(); i++)
		{
			FbxVector4 position = mesh->GetControlPointAt(i);
			item->vertices[i].position = Float3(position[0], position[1], position[2]);
			
			FbxGeometryElementNormal* normalElement = mesh->GetElementNormal();
			if (normalElement != nullptr)
			{
				FbxVector4 normal = normalElement->GetDirectArray().GetAt(i);
				item->vertices[i].normal = Float3(normal.mData[0], normal.mData[1], normal.mData[2]);
			}

			FbxGeometryElementUV* uvElement = mesh->GetElementUV();
			if (uvElement != nullptr)
			{
				FbxVector2 uv = uvElement->GetDirectArray().GetAt(i);
				item->vertices[i].uv = Float2(uv.mData[0], 1.0f - uv.mData[1]);
			}
		}

		item->indices.resize(mesh->GetPolygonVertexCount());

		for (int i = 0; i < mesh->GetPolygonVertexCount(); i++)
		{
			item->indices[i] = mesh->GetPolygonVertices()[i];
		}
	}
	void LoadMeshWithPolygonVertex(FbxMesh* mesh, Mesh* item)
	{
		FbxStringList uvSetNames;
		mesh->GetUVSetNames(uvSetNames);

		for (int i = 0; i < mesh->GetPolygonCount(); i++)
		{
			for (int j = 0; j < mesh->GetPolygonSize(i); j++)
			{
				Vertex vertex;

				int v = mesh->GetPolygonVertex(i, j);
				FbxVector4 position = mesh->GetControlPoints()[v];
				vertex.position = Float3(position.mData[0], position.mData[1], position.mData[2]);

				FbxVector4 normal;
				mesh->GetPolygonVertexNormal(i, j, normal);
				vertex.normal = Float3(normal.mData[0], normal.mData[1], normal.mData[2]);

				FbxVector2 uv;
				bool isMapped;
				if (mesh->GetElementUVCount() > 0)
					mesh->GetPolygonVertexUV(i, j, uvSetNames[0], uv, isMapped);
				vertex.uv = Float2(uv.mData[0], 1.0f - uv.mData[1]);

				item->vertices.push_back(vertex);
			}
		}
	}
	void LoadAnim(FbxScene* scene, FbxMesh* mesh, Mesh* item, bool isOptimized)
	{
		int animNum = 0;

		FbxArray<FbxString*> animStackList;
		scene->FillAnimStackNameArray(animStackList);
		printf("anim %d\n", animStackList.Size());

		FbxAnimStack* animStack = scene->FindMember<FbxAnimStack>(animStackList[animNum]->Buffer());
		scene->SetCurrentAnimationStack(animStack);

		//FbxTakeInfo* takeInfo = scene->GetTakeInfo(*animStackList[animNum]);
		//takeInfo->mLocalTimeSpan.GetStart();

		FbxTime start = animStack->GetLocalTimeSpan().GetStart();
		FbxTime stop = animStack->GetLocalTimeSpan().GetStop();
		float length = (stop.GetMilliSeconds() - start.GetMilliSeconds()) / 1000.0f;
		frameCount = (int)(length * 60.0f);
		bones.resize(frameCount);

		printf("deformer %d\n", mesh->GetDeformerCount());
		for (int i = 0; i < mesh->GetDeformerCount(); i++)
		{
			FbxSkin* skin = (FbxSkin*)mesh->GetDeformer(i, FbxDeformer::eSkin);
			if (skin == nullptr)
				continue;

			boneCount = skin->GetClusterCount();

			std::vector<std::vector<int>> controlPointIndices;
			if (!isOptimized)
			{
				controlPointIndices.resize(mesh->GetControlPointsCount());
				for (int i = 0; i < mesh->GetPolygonVertexCount(); i++)
				{
					controlPointIndices[mesh->GetPolygonVertices()[i]].push_back(i);
				}
			}

			FbxTime time;

			for (int j = 0; j < frameCount; j++)
			{
				time.SetMilliSeconds((j / 60.0f * 1000.0f) + start.GetMilliSeconds());

				FbxMatrix globalPosition = mesh->GetNode()->EvaluateGlobalTransform(time);
				FbxVector4 translation = mesh->GetNode()->GetGeometricTranslation(FbxNode::eSourcePivot);
				FbxVector4 rotation = mesh->GetNode()->GetGeometricRotation(FbxNode::eSourcePivot);
				FbxVector4 scaling = mesh->GetNode()->GetGeometricScaling(FbxNode::eSourcePivot);
				FbxAMatrix geometryOffset = FbxAMatrix(translation, rotation, scaling);

				bones[j].resize(boneCount);

				for (int boneIndex = 0; boneIndex < boneCount; boneIndex++)
				{
					FbxCluster* cluster = skin->GetCluster(boneIndex);

					FbxMatrix vertexTransformMatrix;
					FbxAMatrix referenceGlobalInitPosition;
					FbxAMatrix clusterGlobalInitPosition;
					FbxMatrix clusterGlobalCurrentPosition;
					FbxMatrix clusterRelativeInitPosition;
					FbxMatrix clusterRelativeCurrentPositionInverse;
					cluster->GetTransformMatrix(referenceGlobalInitPosition);
					referenceGlobalInitPosition *= geometryOffset;
					cluster->GetTransformLinkMatrix(clusterGlobalInitPosition);
					clusterGlobalCurrentPosition = cluster->GetLink()->EvaluateGlobalTransform(time);
					clusterRelativeInitPosition = clusterGlobalInitPosition.Inverse() * referenceGlobalInitPosition;
					clusterRelativeCurrentPositionInverse = globalPosition.Inverse() * clusterGlobalCurrentPosition;
					vertexTransformMatrix = clusterRelativeCurrentPositionInverse * clusterRelativeInitPosition;

					bones[j][boneIndex] = FbxMatrixToXMMatrix(vertexTransformMatrix);

					if (isOptimized)
						AddBlendInControlPoint(boneIndex, cluster, item);
					else
						AddBlendInPolygonVertex(boneIndex, cluster, item, controlPointIndices);
				}
			}
		}
	}
	void AddBlendInControlPoint(int blendIndex, FbxCluster* cluster, Mesh* item)
	{
		for (int i = 0; i < cluster->GetControlPointIndicesCount(); i++)
		{
			int index = cluster->GetControlPointIndices()[i];
			float weight = (float)cluster->GetControlPointWeights()[i];

			SearchBlendIndex(index, blendIndex, weight, item);
		}
	}
	void AddBlendInPolygonVertex(int blendIndex, FbxCluster* cluster, Mesh* item, std::vector<std::vector<int>>& controlPointIndices)
	{
		for (int i = 0; i < cluster->GetControlPointIndicesCount(); i++)
		{
			int index = cluster->GetControlPointIndices()[i];
			float weight = (float)cluster->GetControlPointWeights()[i];

			for (int v = 0; v < controlPointIndices[index].size(); v++)
			{
				SearchBlendIndex(controlPointIndices[index][v], blendIndex, weight, item);
			}
		}
	}
	void SearchBlendIndex(int vertexIndex, int blendIndex, float blendWeight, Mesh* item)
	{
		for (int i = 0; i < 8; i++)
		{
			if (item->vertices[vertexIndex].blendIndices[i] == blendIndex)
				return;

			if (item->vertices[vertexIndex].blendIndices[i] < 999)
				continue;

			item->vertices[vertexIndex].blendIndices[i] = blendIndex;
			item->vertices[vertexIndex].blendWeights[i] = blendWeight;
			return;
		}
	}
	static XMMATRIX FbxMatrixToXMMatrix(FbxMatrix source)
	{
		XMMATRIX destination;
		for (int x = 0; x < 4; x++)
		{
			for (int y = 0; y < 4; y++)
			{
				destination.r[x].m128_f32[y] = static_cast<float>(source.mData[x][y]);
			}
		}
		return destination;
	}
};

XLIBRARY_NAMESPACE_END
