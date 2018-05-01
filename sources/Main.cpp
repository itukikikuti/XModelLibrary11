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
	camera.position = Float3(0.0f, 30.0f, 100.0f);
	camera.angles.y = 180.0f;

	//Texture texture(L"assets/drone.jpg");

	//Model model(L"assets/humanoid.fbx");
	//model.angles.x = 90.0f;
	//model.scale = 0.02f;
	//for (int i = 0; i < model.meshes.size(); i++)
	//{
	//	model.meshes[i]->GetMaterial().SetTexture(0, &texture);
	//	model.meshes[i]->GetMaterial().Load(L"assets/test.hlsl");
	//}

	Mesh item;
	item.angles.y = 30.0f;
	item.GetMaterial().Load(L"assets/test.hlsl");
	item.vertices.clear();
	item.indices.clear();
	item.Apply();

	FbxManager* fbxManager = nullptr;
	FbxScene* fbxScene = nullptr;
	FbxNode* meshNode = nullptr;
	FbxMesh* mesh = nullptr;
	int AnimStackNumber = 0;
	FbxTime FrameTime, timeCount, start, stop;

	fbxManager = FbxManager::Create();
	fbxScene = FbxScene::Create(fbxManager, "");

	FbxGeometryConverter converter(fbxManager);
	converter.Triangulate(fbxScene, true);

	FbxImporter* fbxImporter = FbxImporter::Create(fbxManager, "");
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

	item.vertices.resize(mesh->GetControlPointsCount());
	item.indices.resize(mesh->GetPolygonVertexCount());
	int a = mesh->GetElementNormalCount();
	for (int i = 0; i < mesh->GetPolygonVertexCount(); i++)
	{
		item.indices[i] = mesh->GetPolygonVertices()[i];
	}
	item.Apply();

	const int tempSize = 20;
	vector<vector<FbxMatrix>> clusterDeformation;
	clusterDeformation.resize(tempSize);

	for (int i = 0; i < tempSize; i++)
	{
		timeCount += FrameTime;
		if (timeCount > stop) timeCount = start;

		FbxMatrix globalPosition = meshNode->EvaluateGlobalTransform(timeCount);
		FbxVector4 t0 = meshNode->GetGeometricTranslation(FbxNode::eSourcePivot);
		FbxVector4 r0 = meshNode->GetGeometricRotation(FbxNode::eSourcePivot);
		FbxVector4 s0 = meshNode->GetGeometricScaling(FbxNode::eSourcePivot);
		FbxAMatrix geometryOffset = FbxAMatrix(t0, r0, s0);

		clusterDeformation[i].resize(mesh->GetControlPointsCount());
		memset(clusterDeformation[i].data(), 0, sizeof(FbxMatrix) * mesh->GetControlPointsCount());

		FbxSkin *skinDeformer = (FbxSkin *)mesh->GetDeformer(0, FbxDeformer::eSkin);
		int clusterCount = skinDeformer->GetClusterCount();

		for (int clusterIndex = 0; clusterIndex < clusterCount; clusterIndex++) {
			FbxCluster* cluster = skinDeformer->GetCluster(clusterIndex);
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

			for (int j = 0; j < cluster->GetControlPointIndicesCount(); j++) {
				int index = cluster->GetControlPointIndices()[j];
				double weight = cluster->GetControlPointWeights()[j];
				FbxMatrix influence = vertexTransformMatrix * weight;
				clusterDeformation[i][index] += influence;
			}
		}
	}

	int temp = 0;

	while (App::Refresh())
	{
		camera.Update();

		temp++;
		if (temp >= tempSize)
			temp = 0;

		for (int i = 0; i < mesh->GetControlPointsCount(); i++) {
			const XMMATRIX transform = Model::FbxMatrixToXMMatrix(mesh->GetNode()->EvaluateGlobalTransform());

			FbxVector4 outVertex = clusterDeformation[temp][i].MultNormalize(mesh->GetControlPointAt(i));
			item.vertices[i].position.x = (FLOAT)outVertex[0];
			item.vertices[i].position.y = (FLOAT)outVertex[1];
			item.vertices[i].position.z = (FLOAT)outVertex[2];

			FbxLayerElement::EReferenceMode mode = mesh->GetElementNormal()->GetReferenceMode();
			FbxVector4 normal = mesh->GetElementNormal()->GetDirectArray().GetAt(i);
			item.vertices[i].normal = Float3(-normal.mData[0], normal.mData[1], -normal.mData[2]);
			XMVector3TransformCoord(item.vertices[i].normal, transform);
		}
		item.Apply();

		item.Draw();

		App::SetTitle(to_wstring(App::GetFrameRate()).c_str());
		//model.angles.y += App::GetDeltaTime() * 50.0f;
		//model.Draw();
	}

	fbxScene->Destroy();
	fbxManager->Destroy();

	return 0;
}
