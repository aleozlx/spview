#include "spview.h"
using namespace spt::AppEngine;

const char *WindowFeed::static_image_ext[] = {"*.jpg", "*.png"};

WindowFeed::WindowFeed() {
    static_image_path = new char[sz_static_image_path];
    static_image_path[0] = '\0';
}

WindowFeed::~WindowFeed() {
    delete[] static_image_path;
}

WindowFeed::WindowFeed(WindowFeed &&o) noexcept {
    this->static_image_path = o.static_image_path;
    o.static_image_path = nullptr;
}

bool WindowFeed::Draw() {
    ImGui::Begin("Feed");
    if (ImGui::Button("Run")) {
        auto w = std::make_unique<WindowAnalyzerS>(std::string(static_image_path));
        this->CreateIWindow(std::move(w));
    }
    ImGui::SameLine();
    if (ImGui::Button("Select File...")) {
        const char *pth = tinyfd_openFileDialog("Select an image", static_image_path,
                2, static_image_ext, nullptr, 0);
        if (pth)
            SetStaticImagePath(pth);
    }
    ImGui::InputText("", static_image_path, sizeof(static_image_path));
    ImGui::Separator();
    ImGui::End();
    return true;
}

IWindow* WindowFeed::Show() {
    return dynamic_cast<IWindow*>(this);
}

void WindowFeed::SetStaticImagePath(const char *src) {
    std::strncpy(static_image_path, src, sz_static_image_path);
}

void WindowFeed::GrantCreateWindow(std::function<void(std::unique_ptr<IWindow>&&)> cw) {
    this->CreateIWindow = std::move(cw);
}
