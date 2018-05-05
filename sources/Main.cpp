#define XLIBRARY_NAMESPACE_BEGIN
#define XLIBRARY_NAMESPACE_END
#include "XLibrary11.hpp"
#include "Model.hpp"
#include "Model2.hpp"
#include "Library.cpp"

using namespace std;
using namespace XLibrary11;

int MAIN()
{
	Library::Generate(L"sources/Model.hpp", L"XModelLibrary11.hpp");

	Camera camera;
	camera.SetDepthTest(true);
	camera.SetPerspective(60.0f, 0.1f, 100.0f);
	camera.position = Float3(0.0f, 3.0f, -5.0f);
	camera.angles.x = 30.0f;

	/*FbxLayerElement::EMappingMode normalMappingMode = mesh->GetElementNormal()->GetMappingMode();
	FbxLayerElement::EReferenceMode normalReferenceMode = mesh->GetElementNormal()->GetReferenceMode();

	int index = 0;
	for (int polygonIndex = 0; polygonIndex < mesh->GetPolygonCount(); polygonIndex++)
	{
		int polygonSize = mesh->GetPolygonSize(polygonIndex);
		for (int i = 0; i < polygonSize; i++)
		{
			FbxVector4 normal = mesh->GetElementNormal()->GetDirectArray().GetAt(index);
			model2.vertices[model2.indices[index]].normal = Float3(normal.mData[0], normal.mData[1], normal.mData[2]);

			index++;
		}
	}

	FbxLayerElement::EMappingMode uvMmappingMode = mesh->GetElementUV()->GetMappingMode();
	FbxLayerElement::EReferenceMode uvReferenceMode = mesh->GetElementUV()->GetReferenceMode();

	FbxGeometryElementUV* uvElement = mesh->GetElementUV();
	bool useIndex = uvElement->GetReferenceMode() != FbxLayerElement::EReferenceMode::eDirect;
	int indexCount = (useIndex) ? uvElement->GetIndexArray().GetCount() : 0;

	index = 0;
	for (int polygonIndex = 0; polygonIndex < mesh->GetPolygonCount(); polygonIndex++)
	{
		int polygonSize = mesh->GetPolygonSize(polygonIndex);
		for (int i = 0; i < polygonSize; i++)
		{
			if (index < indexCount)
			{
				int uvIndex = useIndex ? uvElement->GetIndexArray().GetAt(index) : index;
				FbxVector2 uv = uvElement->GetDirectArray().GetAt(uvIndex);
				model2.vertices[model2.indices[index]].uv = Float2(uv.mData[0], 1.0f - uv.mData[1]);

				index++;
			}
		}
	}*/

	Texture texture(L"assets/drone.jpg");
	texture.Attach(0);

	Model2 model(L"assets/drone.fbx");
	model.scale = 0.02f;

	while (App::Refresh())
	{
		camera.Update();

		model.angles.y -= App::GetMousePosition().x * 0.3f;
		model.Draw();

		App::SetMousePosition(0.0f, 0.0f);
	}

	return 0;
}
