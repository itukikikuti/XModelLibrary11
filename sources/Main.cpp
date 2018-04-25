#define XLIBRARY_NAMESPACE_BEGIN
#define XLIBRARY_NAMESPACE_END
#include "XLibrary11.hpp"
#include "Model.hpp"
#include "Library.cpp"

using namespace std;
using namespace XLibrary11;

int MAIN() {
	Library::Generate(L"sources/Model.hpp", L"XModelLibrary11.hpp");

	Camera camera;

	camera.SetDepthTest(true);
	camera.SetPerspective(60.0f, 0.1f, 100.0f);
	camera.position = Float3(0.0f, 1.0f, -2.0f);
	camera.angles.x = 20.0f;

	Model model(L"assets/monkey.fbx");
	model.meshes[0]->GetMaterial().Load(L"assets/test.hlsl");

	while (App::Refresh()) {
		camera.Update();

		model.angles.y += App::GetDeltaTime() * 100.0f;
		model.Draw();
	}

	return 0;
}
