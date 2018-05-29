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

	Texture texture(L"assets/crab.jpg");
	texture.Attach(0);

	Model2 model(L"assets/crab.fbx");
	//model.scale = 0.05f;

    Mesh mesh;
    mesh.CreateCube();

	while (App::Refresh())
	{
		camera.Update();

		if (App::GetKeyDown('1'))
		{
			model.Play(0);
		}
		if (App::GetKeyDown('2'))
		{
			model.Play(1);
		}
		if (App::GetKeyDown('3'))
		{
			model.Play(2);
		}

		model.angles.y -= App::GetMousePosition().x * 0.3f;
		model.Draw();

		App::SetMousePosition(0.0f, 0.0f);

        mesh.Draw();
	}

	return 0;
}
