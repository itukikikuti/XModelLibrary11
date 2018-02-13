﻿// © 2017 itukikikuti
#pragma once

#include <fbxsdk.h>

#if defined(_DLL)
#pragma comment(lib, "libfbxsdk-md.lib")
#else
#pragma comment(lib, "libfbxsdk-mt.lib")
#endif

namespace XLibrary11 {

void DeleteFbxManager(fbxsdk::FbxManager* fbxManager)
{
	fbxManager->Destroy();
}

void DeleteFbxImporter(fbxsdk::FbxImporter* fbxImporter)
{
	fbxImporter->Destroy();
}

void DeleteFbxScene(fbxsdk::FbxScene* fbxScene)
{
	fbxScene->Destroy();
}

class Model {
	PUBLIC std::vector<std::unique_ptr<XLibrary11::Mesh>> meshes;

	PUBLIC Model(wchar_t* filePath) {
		std::unique_ptr<fbxsdk::FbxManager, decltype(&DeleteFbxManager)> manager(fbxsdk::FbxManager::Create(), DeleteFbxManager);
		std::unique_ptr<fbxsdk::FbxImporter, decltype(&DeleteFbxImporter)> importer(fbxsdk::FbxImporter::Create(manager.get(), ""), DeleteFbxImporter);

		size_t length = wcslen(filePath) + 1;
		std::unique_ptr<char[]> cFilePath(new char[length]);
		wcstombs_s(nullptr, cFilePath.get(), length, filePath, _TRUNCATE);

		importer->Initialize(cFilePath.get(), -1, manager->GetIOSettings());

		std::unique_ptr<fbxsdk::FbxScene, decltype(&DeleteFbxScene)> scene(fbxsdk::FbxScene::Create(manager.get(), ""), DeleteFbxScene);
		importer->Import(scene.get());

		fbxsdk::FbxGeometryConverter converter(manager.get());
		converter.Triangulate(scene.get(), true);

		fbxsdk::FbxNode* rootNode = scene->GetRootNode();
		LoadMeshRecursively(rootNode);
	}
	PUBLIC virtual ~Model() {
	}
	PUBLIC void Draw() {
		for (int i = 0; i < meshes.size(); i++) {
			meshes[i]->Draw();
		}
	}
	PRIVATE void LoadMeshRecursively(FbxNode *node) {
		fbxsdk::FbxNodeAttribute* attribute = node->GetNodeAttribute();
		if (attribute) {
			if (attribute->GetAttributeType() == fbxsdk::FbxNodeAttribute::eMesh) {
				fbxsdk::FbxMesh* fbxMesh = node->GetMesh();
				std::unique_ptr<XLibrary11::Mesh> mesh(new XLibrary11::Mesh());

				for (int i = 0; i < fbxMesh->GetPolygonCount(); i++) {
					int verticesSize = fbxMesh->GetPolygonSize(i);

					for (int j = 0; j < verticesSize; j++) {
						int v = fbxMesh->GetPolygonVertex(i, j);
						const DirectX::XMMATRIX transform = FbxMatrixToXMMatrix(node->EvaluateGlobalTransform());

						fbxsdk::FbxVector4* fbxPosition = fbxMesh->GetControlPoints();
						XLibrary11::Float3 position(static_cast<float>(-fbxPosition[v].mData[0]), static_cast<float>(-fbxPosition[v].mData[1]), static_cast<float>(-fbxPosition[v].mData[2]));
						DirectX::XMVector3TransformCoord(position, transform);

						fbxsdk::FbxVector4 fbxNormal;
						fbxMesh->GetPolygonVertexNormal(i, j, fbxNormal);
						XLibrary11::Float3 normal(static_cast<float>(-fbxNormal.mData[0]), static_cast<float>(-fbxNormal.mData[1]), static_cast<float>(-fbxNormal.mData[2]));
						DirectX::XMVector3TransformCoord(normal, transform);

						mesh->vertices.push_back(XLibrary11::Vertex(position, normal, XLibrary11::Float2()));
					}
				}
				mesh->Apply();
				meshes.push_back(std::move(mesh));
			}
		}
		for (int i = 0; i < node->GetChildCount(); i++) {
			LoadMeshRecursively(node->GetChild(i));
		}
	}
	PRIVATE DirectX::XMMATRIX FbxMatrixToXMMatrix(fbxsdk::FbxAMatrix source) {
		DirectX::XMMATRIX destination;
		for (int x = 0; x < 4; x++) {
			for (int y = 0; y < 4; y++) {
				destination.r[x].m128_f32[y] = static_cast<float>(source.mData[x][y]);
			}
		}
		return destination;
	}
};

}