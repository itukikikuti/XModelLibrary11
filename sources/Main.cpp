#define XLIBRARY_NAMESPACE_BEGIN
#define XLIBRARY_NAMESPACE_END
#include "XLibrary11.hpp"
#include "Model.hpp"
#include <crtdbg.h>

using namespace std;
using namespace DirectX;
using namespace XLibrary11;

int Main() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	Camera camera;
	Model model(L"assets/monkey.fbx");
	Texture texture(L"assets/box.jpg");

	camera.position = Float3(0.0f, 1.0f, -2.0f);
	camera.angles.x = 20.0f;
	for (int i = 0; i < model.meshes.size(); i++) {
		model.meshes[i]->material.Load(L"assets/test.hlsl");
		model.meshes[i]->SetCullingMode(D3D11_CULL_FRONT);
	}

	while (App::Refresh()) {
		camera.Update();

		for (int i = 0; i < model.meshes.size(); i++) {
			model.meshes[i]->angles.y += App::GetDeltaTime() * 50.0f;
		}
		model.Draw();
	}

	return 0;
}
