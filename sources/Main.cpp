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

	Texture diffuse(L"assets/crab_diffuse.jpg");
    diffuse.Attach(0);

    Texture glossiness(L"assets/crab_glossiness.jpg");
    glossiness.Attach(1);

    Texture normal(L"assets/crab_normal.jpg");
    normal.Attach(2);

	Model2 model(L"assets/crab.fbx");
	//model.scale = 0.05f;

	while (App::Refresh())
	{
		camera.Update();

        model.angles.x -= App::GetMousePosition().y * 0.3f;
        model.angles.y -= App::GetMousePosition().x * 0.3f;
		model.Draw();

		App::SetMousePosition(0.0f, 0.0f);
	}

	return 0;
}
