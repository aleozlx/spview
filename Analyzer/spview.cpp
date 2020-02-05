#include <memory>
#include <functional>

#include "app.hpp"
#include "teximage.hpp"

#include "spview.h"

#if _MSC_VER
// Disables the console window on Windows
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

using namespace spt::AppEngine;

int main(int argc, char *argv[]) {
    auto app = App::Initialize();
    if (!app.ok) return 1;
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    static std::list<std::unique_ptr<IWindow>> windows;
    auto RegisterWindow = [](std::unique_ptr<IWindow> &&w) {
        if (w->Show() != nullptr)
            windows.push_back(std::move(w));
    };

    { // Feed Window
        auto w = std::make_unique<WindowFeed>();
        if (argc == 2) w->SetStaticImagePath(argv[1]);
        w->GrantCreateWindow(RegisterWindow);
        RegisterWindow(std::move(w));
    }

    while (App::EventLoop()){
        for (auto w = windows.begin(); w != windows.end();) {
            if (!(*w)->Draw()) windows.erase(w++);
            else ++w;
        }
        App::Render(clear_color);
    }
    windows.clear();
    return 0;
}
