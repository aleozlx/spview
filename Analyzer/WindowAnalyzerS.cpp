#include "spview.h"
using namespace spt::AppEngine;

std::string getFname(std::string fname) {
    const size_t i_slash = fname.find_last_of("\\/");
    if (std::string::npos != i_slash)
        fname.erase(0, i_slash + 1);
    return fname;
}

WindowAnalyzerS::WindowAnalyzerS(const std::string &src): d_superpixel_size(18) {
    frame = cv::imread(src);
    cv::Size frame_size = frame.size();
    const int width = frame_size.width, height = frame_size.height, channels=3;
    imSuperpixels = spt::TexImage(width, height, channels);
    gslic_settings.img_size = { width, height };
    gslic_settings.no_segs = 64;
    gslic_settings.spixel_size = d_superpixel_size.val;
    gslic_settings.no_iters = 5;
    gslic_settings.coh_weight = 0.6f;
    gslic_settings.do_enforce_connectivity = true;
    gslic_settings.color_space = gSLICr::CIELAB; // gSLICr::XYZ | gSLICr::RGB
    gslic_settings.seg_method = gSLICr::GIVEN_SIZE; // gSLICr::GIVEN_NUM
    char _title[512];
    std::string fname = getFname(src);
    std::snprintf(_title, sizeof(_title),
            "Superpixel Analyzer [%s]", fname.c_str());
    title = _title;
}

bool WindowAnalyzerS::Draw() {
    static int a;
    ImGui::Begin(title.c_str(), &_is_shown);
    ImGui::SliderInt("Superpixel Size", &d_superpixel_size, 8, 64);
    if (d_superpixel_size.Update()) {
        gslic_settings.spixel_size = d_superpixel_size.val;
        this->ReloadSuperpixels();
        a+=1;
    }
    ImGui::Text("test: %d %d %p", a, gslic_settings.spixel_size, frame_tex.data);
    ImGui::Image(imSuperpixels.id(), imSuperpixels.size(), ImVec2(0,0), ImVec2(1,1), ImVec4(1.0f,1.0f,1.0f,1.0f), ImVec4(1.0f,1.0f,1.0f,0.5f));
    ImGui::End();
    return _is_shown;
}

IWindow* WindowAnalyzerS::Show() {
    this->ReloadSuperpixels();
    this->_is_shown = true;
    return dynamic_cast<IWindow*>(this);
}

void WindowAnalyzerS::ReloadSuperpixels() {
#ifdef FEATURE_GSLICR
    spt::GSLIC _superpixel(this->gslic_settings);
    auto superpixel = _superpixel.Compute(this->frame);
    superpixel->GetContour(this->superpixel_contour);
    cv::cvtColor(this->frame, this->frame_tex, cv::COLOR_BGR2RGB);
    this->frame_tex.setTo(cv::Scalar(200, 5, 240), this->superpixel_contour);

    // Convert to texture Format
    cv::cvtColor(this->frame_tex, this->frame_tex, cv::COLOR_RGB2RGBA);
#else
    cv::cvtColor(this->frame, this->frame_tex, cv::COLOR_BGR2RGBA);
#endif
    this->imSuperpixels.Load(this->frame_tex.data);
}

