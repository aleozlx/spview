#include "app.hpp"

using namespace spview;
const unsigned int WIDTH = 432;
const unsigned int HEIGHT = 240;

int main(int, char**) {
    auto app = AppEngine::App::Initialize();
    if (!app.ok) return 1;
//    GLFWwindow* window = app.window;
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    while (app.EventLoop()){
//        for (auto w = windows.begin(); w != windows.end();) {
//            if (!(*w)->Draw()) windows.erase(w++);
//            else ++w;
//        }
        app.Render(clear_color);
    }

    return 0;
}
