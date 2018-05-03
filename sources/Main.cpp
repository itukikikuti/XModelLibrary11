#define XLIBRARY_NAMESPACE_BEGIN
#define XLIBRARY_NAMESPACE_END
#include "XLibrary11.hpp"
#include "Model.hpp"
#include "Library.cpp"
#include <iomanip>

using namespace std;
using namespace XLibrary11;

struct Constant
{
	XMMATRIX bone[100];
};

int MAIN()
{
	Library::Generate(L"sources/Model.hpp", L"XModelLibrary11.hpp");

	Camera camera;
	camera.SetDepthTest(true);
	camera.SetPerspective(60.0f, 0.1f, 1000.0f);
	camera.position = Float3(0.0f, 30.0f, 100.0f);
	camera.angles.y = 180.0f;

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

	for (int i = 0; i < fbxScene->GetRootNode()->GetChildCount(); i++) {
		if (fbxScene->GetRootNode()->GetChild(i)->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh) {
			meshNode = fbxScene->GetRootNode()->GetChild(i);
			mesh = meshNode->GetMesh();
			break;
		}
	}

	item.vertices.resize(mesh->GetControlPointsCount());
	for (int i = 0; i < mesh->GetControlPointsCount(); i++)
	{
		//const XMMATRIX transform = Model::FbxMatrixToXMMatrix(mesh->GetNode()->EvaluateGlobalTransform());

		//FbxVector4 outVertex = clusterDeformation[temp][i].MultNormalize(mesh->GetControlPointAt(i));
		FbxVector4 outVertex = mesh->GetControlPointAt(i);
		item.vertices[i].position = Float3((float)outVertex[0], (float)outVertex[1], (float)outVertex[2]);

		//FbxLayerElement::EReferenceMode mode = mesh->GetElementNormal()->GetReferenceMode();
		//FbxVector4 normal = mesh->GetElementNormal()->GetDirectArray().GetAt(i);
		//item.vertices[i].normal = Float3(-normal.mData[0], normal.mData[1], -normal.mData[2]);
		//XMVector3TransformCoord(item.vertices[i].normal, transform);
	}

	item.indices.resize(mesh->GetPolygonVertexCount());
	for (int i = 0; i < mesh->GetPolygonVertexCount(); i++)
	{
		item.indices[i] = mesh->GetPolygonVertices()[i];
	}

	item.Apply();

	float length = AnimationStack->GetLocalTimeSpan().GetDuration().GetMilliSeconds() / 1000.0f;
	const int tempSize = (int)(length * 60.0f);
	vector<vector<XMMATRIX>> clusterDeformation;
	clusterDeformation.resize(tempSize);

	FbxSkin* skinDeformer = (FbxSkin*)mesh->GetDeformer(0, FbxDeformer::eSkin);
	int clusterCount = skinDeformer->GetClusterCount();

	FbxTime time;

	for (int i = 0; i < tempSize; i++)
	{
		time.SetMilliSeconds(i / 60.0f * 1000.0f);

		FbxMatrix globalPosition = meshNode->EvaluateGlobalTransform(time);
		FbxVector4 t0 = meshNode->GetGeometricTranslation(FbxNode::eSourcePivot);
		FbxVector4 r0 = meshNode->GetGeometricRotation(FbxNode::eSourcePivot);
		FbxVector4 s0 = meshNode->GetGeometricScaling(FbxNode::eSourcePivot);
		FbxAMatrix geometryOffset = FbxAMatrix(t0, r0, s0);

		clusterDeformation[i].resize(clusterCount);
		//memset(clusterDeformation[i].data(), 0, sizeof(FbxMatrix) * clusterCount);

		for (int clusterIndex = 0; clusterIndex < clusterCount; clusterIndex++)
		{
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
			clusterGlobalCurrentPosition = cluster->GetLink()->EvaluateGlobalTransform(time);
			clusterRelativeInitPosition = clusterGlobalInitPosition.Inverse() * referenceGlobalInitPosition;
			clusterRelativeCurrentPositionInverse = globalPosition.Inverse() * clusterGlobalCurrentPosition;
			vertexTransformMatrix = clusterRelativeCurrentPositionInverse * clusterRelativeInitPosition;

			clusterDeformation[i][clusterIndex] = Model::FbxMatrixToXMMatrix(vertexTransformMatrix);

			for (int j = 0; j < cluster->GetControlPointIndicesCount(); j++)
			{
				int index = cluster->GetControlPointIndices()[j];
				float weight = (float)cluster->GetControlPointWeights()[j];
				//FbxMatrix influence = vertexTransformMatrix * weight;
				for (int k = 0; k < 8; k++)
				{
					if (item.vertices[index].blendIndices[k] == clusterIndex)
						break;

					if (item.vertices[index].blendIndices[k] < 999)
						continue;

					item.vertices[index].blendIndices[k] = clusterIndex;
					item.vertices[index].blendWeights[k] = weight;
					break;
				}
			}
		}
	}

	Constant constant;

	item.Apply();
	item.GetMaterial().SetBuffer(2, &constant, sizeof(Constant));

	//Texture texture(L"assets/drone.jpg");

	//Model model(L"assets/humanoid.fbx");
	//model.angles.x = 90.0f;
	//model.scale = 0.02f;
	//for (int i = 0; i < model.meshes.size(); i++)
	//{
	//	model.meshes[i]->GetMaterial().SetTexture(0, &texture);
	//	model.meshes[i]->GetMaterial().Load(L"assets/test.hlsl");
	//}

	//for (int i = 0; i < item.vertices.size(); i++)
	//{
	//	cout << "[" << setw(4) << setfill('0') << i << "] " << setfill(' ')
	//		<< setw(10) << item.vertices[i].position.x << ","
	//		<< setw(10) << item.vertices[i].position.y << ","
	//		<< setw(10) << item.vertices[i].position.z << " "
	//		<< setw(3) << item.vertices[i].blendIndices[0] << ","
	//		<< setw(3) << item.vertices[i].blendIndices[1] << ","
	//		<< setw(3) << item.vertices[i].blendIndices[2] << ","
	//		<< setw(3) << item.vertices[i].blendIndices[3] << " "
	//		<< setw(12) << item.vertices[i].blendWeights[0] << ","
	//		<< setw(12) << item.vertices[i].blendWeights[1] << ","
	//		<< setw(12) << item.vertices[i].blendWeights[2] << ","
	//		<< setw(12) << item.vertices[i].blendWeights[3] << " "
	//		<< setw(12) << item.vertices[i].blendWeights[0] + item.vertices[i].blendWeights[1] + item.vertices[i].blendWeights[2] + item.vertices[i].blendWeights[3] << endl;
	//}

	int temp = 0;

	while (App::Refresh())
	{
		camera.Update();

		temp++;
		if (temp >= tempSize)
			temp = 0;

		for (int i = 0; i < clusterCount; i++)
		{
			constant.bone[i] = XMMatrixTranspose(
				clusterDeformation[temp][i]
			);
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
