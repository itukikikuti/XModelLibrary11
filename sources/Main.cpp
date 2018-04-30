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
	camera.SetPerspective(60.0f, 0.1f, 100.0f);
	camera.position = Float3(0.0f, 3.0f, -5.0f);
	camera.angles.x = 20.0f;

	Texture texture(L"assets/drone.jpg");

	Model model(L"assets/drone.fbx");
	model.angles.x = 90.0f;
	model.scale = 0.02f;
	for (int i = 0; i < model.meshes.size(); i++)
	{
		model.meshes[i]->GetMaterial().SetTexture(0, &texture);
		//model.meshes[i]->GetMaterial().Load(L"assets/test.hlsl");
	}

	while (App::Refresh())
	{
		camera.Update();

		model.angles.y += App::GetDeltaTime() * 50.0f;
		model.Draw();
	}

	return 0;
}
