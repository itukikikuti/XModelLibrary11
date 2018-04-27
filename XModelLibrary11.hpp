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

class Model
{
public:
	XLibrary11::Float3 position;
	XLibrary11::Float3 angles;
	XLibrary11::Float3 scale;
	std::vector<std::unique_ptr<XLibrary11::Mesh>> meshes;

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
		std::unique_ptr<char[]> cFilePath(new char[length]);
		wcstombs_s(nullptr, cFilePath.get(), length, filePath, _TRUNCATE);

		std::unique_ptr<FbxImporter, FbxImporterDeleter> importer(FbxImporter::Create(manager.get(), ""));
		importer->Initialize(cFilePath.get(), -1, manager->GetIOSettings());

		std::unique_ptr<FbxScene, FbxSceneDeleter> scene(FbxScene::Create(manager.get(), ""));
		importer->Import(scene.get());

		FbxGeometryConverter converter(manager.get());
		converter.Triangulate(scene.get(), true);

		FbxNode* rootNode = scene->GetRootNode();
		LoadMesh(rootNode);

		if (scene->GetSrcObjectCount<FbxAnimStack>() <= 0)
			return;

		std::vector<std::string> boneName;
		LoadBone(rootNode, boneName);
	}
	void Draw()
	{
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

	void Initialize()
	{
		position = XLibrary11::Float3(0.0f, 0.0f, 0.0f);
		angles = XLibrary11::Float3(0.0f, 0.0f, 0.0f);
		scale = XLibrary11::Float3(1.0f, 1.0f, 1.0f);
	}
	void LoadMesh(FbxNode *node)
	{
		FbxNodeAttribute* attribute = node->GetNodeAttribute();
		if (attribute)
		{
			if (attribute->GetAttributeType() == FbxNodeAttribute::eMesh)
			{
				FbxMesh* fbxMesh = node->GetMesh();
				std::unique_ptr<XLibrary11::Mesh> mesh(new XLibrary11::Mesh());
				mesh->vertices.clear();
				mesh->indices.clear();

				for (int i = 0; i < fbxMesh->GetPolygonCount(); i++)
				{
					int verticesSize = fbxMesh->GetPolygonSize(i);

					for (int j = 0; j < verticesSize; j++)
					{
						int v = fbxMesh->GetPolygonVertex(i, j);
						const DirectX::XMMATRIX transform = FbxMatrixToXMMatrix(node->EvaluateGlobalTransform());

						FbxVector4* fbxPosition = fbxMesh->GetControlPoints();
						XLibrary11::Float3 position(static_cast<float>(-fbxPosition[v].mData[0]), static_cast<float>(-fbxPosition[v].mData[1]), static_cast<float>(-fbxPosition[v].mData[2]));
						DirectX::XMVector3TransformCoord(position, transform);

						FbxVector4 fbxNormal;
						fbxMesh->GetPolygonVertexNormal(i, j, fbxNormal);
						XLibrary11::Float3 normal(static_cast<float>(-fbxNormal.mData[0]), static_cast<float>(-fbxNormal.mData[1]), static_cast<float>(-fbxNormal.mData[2]));
						DirectX::XMVector3TransformCoord(normal, transform);

						FbxStringList uvSetNames;
						fbxMesh->GetUVSetNames(uvSetNames);
						FbxVector2 fbxUV;
						bool isMapped;
						fbxMesh->GetPolygonVertexUV(i, j, uvSetNames[0], fbxUV, isMapped);
						XLibrary11::Float2 uv(static_cast<float>(fbxUV.mData[1]), static_cast<float>(fbxUV.mData[0]));

						mesh->vertices.push_back(XLibrary11::Vertex(position, normal, uv));
					}
				}
				mesh->Apply();
				mesh->SetCullingMode(D3D11_CULL_FRONT);
				meshes.push_back(std::move(mesh));
			}
		}
		for (int i = 0; i < node->GetChildCount(); i++)
		{
			LoadMesh(node->GetChild(i));
		}
	}
	void LoadBone(FbxNode *node, std::vector<std::string>& boneName)
	{
		if (node->GetNodeAttribute() != nullptr)
		{
			if (node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
			{
				boneName.push_back(node->GetName());
			}
		}

		for (int i = 0; i < node->GetChildCount(); i++)
		{
			LoadBone(node->GetChild(i), boneName);
		}
	}
	static DirectX::XMMATRIX FbxMatrixToXMMatrix(FbxAMatrix source)
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
};

}
