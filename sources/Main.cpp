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

	Model model(L"a;laefal;jlfw");

	while (App::Refresh()) {
	}

	return 0;
}
