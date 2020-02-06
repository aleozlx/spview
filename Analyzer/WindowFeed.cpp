#include "spview.h"
using namespace spt::AppEngine;

const char *WindowFeed::static_image_ext[] = {"*.jpg", "*.png"};
const char *WindowFeed::static_video_ext[] = {"*.mp4"};

WindowFeed::WindowFeed() {
    b_static_image_path = new char[sz_static_image_path];
    b_static_image_path[0] = '\0';
}

WindowFeed::~WindowFeed() {
    delete[] b_static_image_path;
}

WindowFeed::WindowFeed(WindowFeed &&o) noexcept {
    this->b_static_image_path = o.b_static_image_path;
    o.b_static_image_path = nullptr;
}

bool WindowFeed::Draw() {
    const ImVec4 color_header(0.8f, 0.55f, 0.2f, 1.f);
    ImGui::Begin("Feed");
    ImGui::TextColored(color_header, "From a Static Image");
    if (ImGui::Button("Launch")) {
        auto w = std::make_unique<WindowAnalyzerS>(std::string(b_static_image_path));
        this->CreateIWindow(std::move(w));
    }
    ImGui::SameLine();
    if (ImGui::Button("Select Image...")) {
        const char *pth = tinyfd_openFileDialog("Select an image", b_static_image_path,
                                                IM_ARRAYSIZE(static_image_ext), static_image_ext,
                                                nullptr, 0);
        if (pth)
            SetStaticImagePath(pth);
    }
    ImGui::InputText("", b_static_image_path, sizeof(b_static_image_path));
    ImGui::Separator();

    ImGui::TextColored(color_header, "From a Video Footage");
    ImGui::Button("Launch");
    ImGui::SameLine();
    if (ImGui::Button("Select Video...")) {
        const char *pth = tinyfd_openFileDialog("Select a video", b_static_image_path,
                                                IM_ARRAYSIZE(static_video_ext), static_video_ext,
                                                nullptr, 0);
//        if (pth)
//            SetStaticImagePath(pth);
    }
    ImGui::InputText("", b_static_image_path, sizeof(b_static_image_path));
    ImGui::Separator();

    ImGui::TextColored(color_header, "From a Camera");
    ImGui::Button("Launch");
    ImGui::SameLine();
    ImGui::Combo("Camera", &b_camera_selection, "Video 0\0Video 1\0");
    ImGui::End();
    return true;
}

void WindowFeed::SetStaticImagePath(const char *src) {
    std::strncpy(b_static_image_path, src, sz_static_image_path);
}

void WindowFeed::GrantCreateWindow(std::function<void(std::unique_ptr<IWindow>&&)> cw) {
    this->CreateIWindow = std::move(cw);
}
