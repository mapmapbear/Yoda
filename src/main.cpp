#include <windows.h>
#include "application/sample_app_temp.h"
#include "core/logger.h"

using namespace Yoda;

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
	SampleAppConfig config;
	config.windowDesc.width = 900;
	config.windowDesc.height = 600;
	config.windowDesc.title = "Fake Render Editor";
	config.windowDesc.resizableWindow = true;
	Logger::get_singleton().singletonLogger->error("tttttttttttttt");

	SampleAppTemplate app(config);
	app.run();
}