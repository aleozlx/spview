#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <functional>

#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/core/utility.hpp>

#include "app.hpp"
#include "teximage.hpp"
#include "superpixel.hpp"

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
    std::function<void(std::unique_ptr<IWindow>&&)> RegisterWindow = [](std::unique_ptr<IWindow> &&w) {
        if (w->Show() != nullptr)
            windows.push_back(std::move(w));
    };

    { // Feed Window
        auto w = std::make_unique<WindowFeed>();
        if (argc == 2) w->SetStaticImagePath(argv[1]);
        w->GrantCreateWindow(RegisterWindow);
        RegisterWindow(std::move(w));
    }

    cv::Mat frame, frame_tex;
    cv::Mat superpixel_contour;
    frame = cv::imread(FIXTURES_DIR "test.png");
    cv::Size frame_size = frame.size();
    const int width = frame_size.width, height = frame_size.height, channels=3;
    spt::TexImage imSuperpixels(width, height, channels);
#ifdef FEATURE_GSLICR
    gSLICr::objects::settings gslic_settings;
    gslic_settings.img_size = { width, height };
    gslic_settings.no_segs = 64;
    gslic_settings.spixel_size = 32;
    gslic_settings.no_iters = 5;
    gslic_settings.coh_weight = 0.6f;
    gslic_settings.do_enforce_connectivity = true;
    gslic_settings.color_space = gSLICr::XYZ; // gSLICr::CIELAB | gSLICr::RGB
    gslic_settings.seg_method = gSLICr::GIVEN_SIZE; // gSLICr::GIVEN_NUM
    spt::GSLIC _superpixel(gslic_settings);
#endif
    while (App::EventLoop()){
//        ImGui::Begin("Superpixel Analyzer");
//#ifdef FEATURE_GSLICR
//        spt::ISuperpixel* superpixel = _superpixel.Compute(frame);
//        superpixel->GetContour(superpixel_contour);
//        cv::cvtColor(frame, frame_tex, cv::COLOR_BGR2RGB);
//        frame_tex.setTo(cv::Scalar(200, 5, 240), superpixel_contour);
//#endif
//        cv::cvtColor(frame_tex, frame_tex, cv::COLOR_RGB2RGBA);
//        imSuperpixels.Load(frame_tex.data);
//        ImGui::Image(imSuperpixels.id(), imSuperpixels.size(), ImVec2(0,0), ImVec2(1,1), ImVec4(1.0f,1.0f,1.0f,1.0f), ImVec4(1.0f,1.0f,1.0f,0.5f));
//        ImGui::Text("windows: %llu", windows.size());
//        ImGui::End();

        for (auto w = windows.begin(); w != windows.end();) {
            if (!(*w)->Draw()) windows.erase(w++);
            else ++w;
        }
        App::Render(clear_color);
    }
    windows.clear();
    return 0;
}
