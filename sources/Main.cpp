#include "XLibrary11.hpp"
#include <crtdbg.h>

using namespace std;
using namespace DirectX;
using namespace XLibrary11;

int Main() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	Camera camera;
	Mesh mesh;

	camera.position = Float3(0.0f, 1.0f, -2.0f);
	camera.angles.x = 20.0f;

	while (App::Refresh()) {
		camera.Update();

		mesh.angles.y += App::GetDeltaTime() * 50.0f;
		mesh.Draw();
	}

	return 0;
}
