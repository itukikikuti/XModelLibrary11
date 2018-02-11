// © 2017 itukikikuti
#pragma once

#include <fbxsdk.h>
#include <locale.h>

#if defined(_DLL)
#pragma comment(lib, "libfbxsdk-md.lib")
#else
#pragma comment(lib, "libfbxsdk-mt.lib")
#endif

XLIBRARY_NAMESPACE_BEGIN

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
		
		fbxsdk::FbxNode* node = scene->GetRootNode();
	}
	PUBLIC virtual ~Model() {
	}
};

XLIBRARY_NAMESPACE_END
