#include "spview.h"
using namespace spt::AppEngine;

std::string getFname(std::string fname) {
    const size_t i_slash = fname.find_last_of("\\/");
    if (std::string::npos != i_slash)
        fname.erase(0, i_slash + 1);
    return fname;
}

WindowAnalyzerS::WindowAnalyzerS(const std::string &src): b_superpixel_size(18) {
    frame = cv::imread(src);
    cv::Size frame_size = frame.size();
    const int width = frame_size.width, height = frame_size.height, channels=3;
    imSuperpixels = spt::TexImage(width, height, channels);
    gslic_settings.img_size = { width, height };
    gslic_settings.no_segs = 64;
    gslic_settings.spixel_size = b_superpixel_size.val;
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
    ImGui::Begin(title.c_str(), &b_is_shown, ImGuiWindowFlags_MenuBar);
    this->DrawMenuBar();
    ImGui::SliderInt("Superpixel Size", &b_superpixel_size, 8, 64);
    if (b_superpixel_size.Update()) {
        gslic_settings.spixel_size = b_superpixel_size.val;
        this->ReloadSuperpixels();
    }
    ImGui::Image(imSuperpixels.id(), imSuperpixels.size(), ImVec2(0,0), ImVec2(1,1), ImVec4(1.0f,1.0f,1.0f,1.0f), ImVec4(1.0f,1.0f,1.0f,0.5f));
    ImGui::End();
    return b_is_shown;
}

void WindowAnalyzerS::DrawMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("Save as...");
            if(ImGui::MenuItem("Close"))
                this->b_is_shown = false;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Superpixels")) {
            ImGui::MenuItem("gSLIC Options");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Stats")) {
            ImGui::MenuItem("Count");
            ImGui::MenuItem("Histogram");
            ImGui::MenuItem("Entropy");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Features")) {
            ImGui::MenuItem("VGG16");
            ImGui::MenuItem("VGG19");
            ImGui::MenuItem("Res50");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Structure")) {
            ImGui::MenuItem("MST");
            ImGui::MenuItem("CRF");
            ImGui::MenuItem("GNG");
            ImGui::MenuItem("MIL Bag");
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

IWindow* WindowAnalyzerS::Show() {
    this->ReloadSuperpixels();
    this->b_is_shown = true;
    return dynamic_cast<IWindow*>(this);
}

void WindowAnalyzerS::ReloadSuperpixels() {
#ifdef FEATURE_GSLICR
    spt::GSLIC _superpixel(gslic_settings);
    auto superpixel = _superpixel.Compute(frame);
    superpixel->GetContour(superpixel_contour);
    cv::cvtColor(frame, frame_tex, cv::COLOR_BGR2RGB);
	frame_tex.setTo(cv::Scalar(200, 5, 240), superpixel_contour);

    // Convert to texture Format
    cv::cvtColor(frame_tex, frame_tex, cv::COLOR_RGB2RGBA);
#else
    cv::cvtColor(frame, frame_tex, cv::COLOR_BGR2RGBA);
#endif
    imSuperpixels.Load(frame_tex.data);
}

