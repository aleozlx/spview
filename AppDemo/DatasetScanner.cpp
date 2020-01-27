#include "app.hpp"

#if WIN32
// Disables the console window on Windows
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

using namespace spview;

int main(int, char**) {
    auto app = AppEngine::App::Initialize();
    if (!app.ok) return 1;
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    while (app.EventLoop()){
        ImGui::Begin("Dataset Scanner");
//        ImGui::
        ImGui::End();
        app.Render(clear_color);
    }

    return 0;
}
