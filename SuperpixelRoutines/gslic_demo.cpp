#include <iostream>
#include <string>
#include <vector>

#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/core/utility.hpp>

#include "app.hpp"
#include "teximage.hpp"
#include "superpixel.hpp"

#if _MSC_VER
// Disables the console window on Windows
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

using namespace spt;

int main(int, char**) {
    auto app = AppEngine::App::Initialize();
    if (!app.ok) return 1;
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    
    cv::Mat frame, frame_tex;
    cv::Mat superpixel_contour;
	frame = cv::imread(FIXTURES_DIR "test.png");
    cv::Size frame_size = frame.size();
    const int width = frame_size.width, height = frame_size.height, channels=3;
    TexImage imSuperpixels(width, height, channels);
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
    GSLIC _superpixel(gslic_settings);
#endif
    while (AppEngine::App::EventLoop()){
        ImGui::Begin("Superpixel Analyzer");
#ifdef FEATURE_GSLICR
        ISuperpixel* superpixel = _superpixel.Compute(frame);
        superpixel->GetContour(superpixel_contour);
        cv::cvtColor(frame, frame_tex, cv::COLOR_BGR2RGB);
        frame_tex.setTo(cv::Scalar(200, 5, 240), superpixel_contour);
#endif
        cv::cvtColor(frame_tex, frame_tex, cv::COLOR_RGB2RGBA);
        imSuperpixels.Load(frame_tex.data);
        ImGui::Image(imSuperpixels.id(), imSuperpixels.size(), ImVec2(0,0), ImVec2(1,1), ImVec4(1.0f,1.0f,1.0f,1.0f), ImVec4(1.0f,1.0f,1.0f,0.5f));
        ImGui::End();
        AppEngine::App::Render(clear_color);
    }
    return 0;
}
