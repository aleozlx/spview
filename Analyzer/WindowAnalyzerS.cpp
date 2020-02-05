#include "spview.h"
using namespace spt::AppEngine;

WindowAnalyzerS::WindowAnalyzerS(std::string src) {
    frame = cv::imread(src);
    cv::Size frame_size = frame.size();
    const int width = frame_size.width, height = frame_size.height, channels=3;
    imSuperpixels = spt::TexImage(width, height, channels);
    gslic_settings.img_size = { width, height };
    gslic_settings.no_segs = 64;
    gslic_settings.spixel_size = 32;
    gslic_settings.no_iters = 5;
    gslic_settings.coh_weight = 0.6f;
    gslic_settings.do_enforce_connectivity = true;
    gslic_settings.color_space = gSLICr::XYZ; // gSLICr::CIELAB | gSLICr::RGB
    gslic_settings.seg_method = gSLICr::GIVEN_SIZE; // gSLICr::GIVEN_NUM
    _superpixel = spt::GSLIC(gslic_settings);
}

bool WindowAnalyzerS::Draw() {
    ImGui::Begin("Superpixel Analyzer", &_is_shown);
    ImGui::Image(imSuperpixels.id(), imSuperpixels.size(), ImVec2(0,0), ImVec2(1,1), ImVec4(1.0f,1.0f,1.0f,1.0f), ImVec4(1.0f,1.0f,1.0f,0.5f));
    ImGui::End();
    return _is_shown;
}

IWindow* WindowAnalyzerS::Show() {
#ifdef FEATURE_GSLICR
    spt::ISuperpixel* superpixel = _superpixel.Compute(frame);
    superpixel->GetContour(superpixel_contour);
    cv::cvtColor(frame, frame_tex, cv::COLOR_BGR2RGB);
    frame_tex.setTo(cv::Scalar(200, 5, 240), superpixel_contour);
#endif
    cv::cvtColor(frame_tex, frame_tex, cv::COLOR_RGB2RGBA);
    imSuperpixels.Load(frame_tex.data);
    this->_is_shown = true;
    return dynamic_cast<IWindow*>(this);
}

