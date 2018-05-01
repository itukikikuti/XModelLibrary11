#define XLIBRARY_NAMESPACE_BEGIN
#define XLIBRARY_NAMESPACE_END
#include "XLibrary11.hpp"
#include "Model.hpp"
#include "Library.cpp"

using namespace std;
using namespace XLibrary11;

int MAIN()
{
	Library::Generate(L"sources/Model.hpp", L"XModelLibrary11.hpp");

	Camera camera;
	camera.SetDepthTest(true);
	camera.SetPerspective(60.0f, 0.1f, 1000.0f);
	camera.position = Float3(0.0f, 30.0f, 300.0f);
	camera.angles.y = 180.0f;

	//Texture texture(L"assets/drone.jpg");

	//Model model(L"assets/drone.fbx");
	//model.angles.x = 90.0f;
	//model.scale = 0.02f;
	//for (int i = 0; i < model.meshes.size(); i++)
	//{
	//	model.meshes[i]->GetMaterial().SetTexture(0, &texture);
	//	model.meshes[i]->GetMaterial().Load(L"assets/test.hlsl");
	//}

	Mesh model;
	model.angles.y = 30.0f;
	model.GetMaterial().Load(L"assets/test.hlsl");
	model.vertices.clear();
	model.indices.clear();
	model.Apply();

	FbxManager* fbxManager = nullptr;
	FbxScene* fbxScene = nullptr;
	FbxNode* meshNode = nullptr;
	FbxMesh* mesh = nullptr;
	int AnimStackNumber = 0;
	FbxTime FrameTime, timeCount, start, stop;

	fbxManager = FbxManager::Create();
	fbxScene = FbxScene::Create(fbxManager, "fbxscene");
	FbxImporter* fbxImporter = FbxImporter::Create(fbxManager, "imp");
	fbxImporter->Initialize("assets/humanoid.fbx", -1, fbxManager->GetIOSettings());
	fbxImporter->Import(fbxScene);
	fbxImporter->Destroy();

	FbxArray<FbxString*> AnimStackNameArray;
	fbxScene->FillAnimStackNameArray(AnimStackNameArray);
	FbxAnimStack* AnimationStack = fbxScene->FindMember<FbxAnimStack>(AnimStackNameArray[AnimStackNumber]->Buffer());
	fbxScene->SetCurrentAnimationStack(AnimationStack);

	FbxTakeInfo* takeInfo = fbxScene->GetTakeInfo(*(AnimStackNameArray[AnimStackNumber]));
	start = takeInfo->mLocalTimeSpan.GetStart();
	stop = takeInfo->mLocalTimeSpan.GetStop();
	FrameTime.SetTime(0, 0, 0, 1, 0, fbxScene->GetGlobalSettings().GetTimeMode());
	timeCount = start;

	for (int i = 0; i < fbxScene->GetRootNode()->GetChildCount(); i++) {
		if (fbxScene->GetRootNode()->GetChild(i)->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh) {
			meshNode = fbxScene->GetRootNode()->GetChild(i);
			mesh = meshNode->GetMesh();
			break;
		}
	}

	model.vertices.resize(mesh->GetControlPointsCount());
	model.indices.resize(mesh->GetPolygonVertexCount());
	for (int i = 0; i < mesh->GetPolygonVertexCount(); i++)
	{
		model.indices[i] = mesh->GetPolygonVertices()[i];
	}
	model.Apply();

	while (App::Refresh())
	{
		camera.Update();

		timeCount += FrameTime;
		if (timeCount > stop) timeCount = start;

		FbxMatrix globalPosition = meshNode->EvaluateGlobalTransform(timeCount);
		FbxVector4 t0 = meshNode->GetGeometricTranslation(FbxNode::eSourcePivot);
		FbxVector4 r0 = meshNode->GetGeometricRotation(FbxNode::eSourcePivot);
		FbxVector4 s0 = meshNode->GetGeometricScaling(FbxNode::eSourcePivot);
		FbxAMatrix geometryOffset = FbxAMatrix(t0, r0, s0);

		FbxMatrix *clusterDeformation = new FbxMatrix[mesh->GetControlPointsCount()];
		memset(clusterDeformation, 0, sizeof(FbxMatrix) * mesh->GetControlPointsCount());

		FbxSkin *skinDeformer = (FbxSkin *)mesh->GetDeformer(0, FbxDeformer::eSkin);
		int clusterCount = skinDeformer->GetClusterCount();

		for (int clusterIndex = 0; clusterIndex < clusterCount; clusterIndex++) {
			FbxCluster *cluster = skinDeformer->GetCluster(clusterIndex);
			FbxMatrix vertexTransformMatrix;
			FbxAMatrix referenceGlobalInitPosition;
			FbxAMatrix clusterGlobalInitPosition;
			FbxMatrix clusterGlobalCurrentPosition;
			FbxMatrix clusterRelativeInitPosition;
			FbxMatrix clusterRelativeCurrentPositionInverse;
			cluster->GetTransformMatrix(referenceGlobalInitPosition);
			referenceGlobalInitPosition *= geometryOffset;
			cluster->GetTransformLinkMatrix(clusterGlobalInitPosition);
			clusterGlobalCurrentPosition = cluster->GetLink()->EvaluateGlobalTransform(timeCount);
			clusterRelativeInitPosition = clusterGlobalInitPosition.Inverse() * referenceGlobalInitPosition;
			clusterRelativeCurrentPositionInverse = globalPosition.Inverse() * clusterGlobalCurrentPosition;
			vertexTransformMatrix = clusterRelativeCurrentPositionInverse * clusterRelativeInitPosition;

			for (int i = 0; i < cluster->GetControlPointIndicesCount(); i++) {
				int index = cluster->GetControlPointIndices()[i];
				double weight = cluster->GetControlPointWeights()[i];
				FbxMatrix influence = vertexTransformMatrix * weight;
				clusterDeformation[index] += influence;
			}
		}

		for (int i = 0; i < mesh->GetControlPointsCount(); i++) {
			FbxVector4 outVertex = clusterDeformation[i].MultNormalize(mesh->GetControlPointAt(i));
			model.vertices[i].position.x = (FLOAT)outVertex[0];
			model.vertices[i].position.y = (FLOAT)outVertex[1];
			model.vertices[i].position.z = (FLOAT)outVertex[2];
		}
		model.Apply();
		delete[] clusterDeformation;

		model.Draw();

		App::SetTitle(to_wstring(App::GetFrameRate()).c_str());
		//model.angles.y += App::GetDeltaTime() * 50.0f;
		//model.Draw();
	}

	fbxScene->Destroy();
	fbxManager->Destroy();

	return 0;
}
