#include "spview.h"

using namespace spt::AppEngine;

std::string getFname(std::string fname) {
    const size_t i_slash = fname.find_last_of("\\/");
    if (std::string::npos != i_slash)
        fname.erase(0, i_slash + 1);
    return fname;
}

WindowAnalyzerS::WindowAnalyzerS(const std::string &src) :
        b_superpixel_size(18),
        b_superpixel_compactness(6),
        b_superpixel_enforce_conn(true),
        b_superpixel_colorspace(gSLICr::CIELAB) {
    frame_raw = cv::imread(src);
    cv::Size frame_size = frame_raw.size();
    if (b_fit_width) {
        frame_display_size = spt::Math::FitWidth(frame_size, fit_width);
        imSuperpixels = spt::TexImage(frame_display_size.width, frame_display_size.height, 3);
    } else {
        imSuperpixels = spt::TexImage(frame_size.width, frame_size.height, 3);
    }
    if (b_fit_width && b_resize_input) {
        gslic_settings.img_size = {frame_display_size.width, frame_display_size.height};
    } else {
        gslic_settings.img_size = {frame_size.width, frame_size.height};
    }
    gslic_settings.no_segs = 64;
    gslic_settings.spixel_size = b_superpixel_size.val;
    gslic_settings.no_iters = 5;
    gslic_settings.coh_weight = ((float)b_superpixel_compactness.val) / 10.f;
    gslic_settings.do_enforce_connectivity = b_superpixel_enforce_conn.val;
    gslic_settings.color_space = static_cast<gSLICr::COLOR_SPACE>(b_superpixel_colorspace.val);
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

    // Prepare texture
    if (b_fit_width) {
        const auto winWidth = (int) ImGui::GetWindowWidth();
        const int fitWidth = winWidth - 30;
        if (std::abs(fitWidth - this->fit_width) > 6) { // resized
            this->TexFitWidth(fitWidth);
            this->ReloadSuperpixels();
        }
    }

    if(b_gslic_options) {
        ImGui::SliderInt("Superpixel Size", &b_superpixel_size, 8, 64);
        if (b_superpixel_size.Update()) {
            gslic_settings.spixel_size = b_superpixel_size.val;
            this->ReloadSuperpixels();
        }

        ImGui::SliderInt("Compactness", &b_superpixel_compactness, 1, 20);
        if (b_superpixel_compactness.Update()) {
            gslic_settings.coh_weight = ((float) b_superpixel_compactness.val) / 10.f;
            this->ReloadSuperpixels();
        }

        ImGui::Checkbox("Enforce Connectivity", &b_superpixel_enforce_conn);
        if (b_superpixel_enforce_conn.Update()) {
            gslic_settings.do_enforce_connectivity = b_superpixel_enforce_conn.val;
            this->ReloadSuperpixels();
        }

        b_superpixel_colorspace.BeginUpdate();
        ImGui::RadioButton("CIELAB", &b_superpixel_colorspace, gSLICr::CIELAB);
        ImGui::SameLine();
        ImGui::RadioButton("XYZ", &b_superpixel_colorspace, gSLICr::XYZ);
        ImGui::SameLine();
        ImGui::RadioButton("RGB", &b_superpixel_colorspace, gSLICr::RGB);
        if (b_superpixel_colorspace.Update()) {
            gslic_settings.color_space = b_superpixel_colorspace.val;
            this->ReloadSuperpixels();
        }

        ImGui::Separator();
    }

    // Render texture
    ImGui::Image(imSuperpixels.id(), imSuperpixels.size(), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
                 ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
    const cv::Size frame_size = frame.size();
    ImGui::Text("True size: %d x %d;  Resize: %d x %d",
                frame_size.width, frame_size.height, frame_display_size.width, frame_display_size.height);
    ImGui::End();
    return b_is_shown;
}

void WindowAnalyzerS::TexFitWidth(const int fitWidth) {
    fit_width = fitWidth;
    const cv::Size frame_size = frame.size();
    frame_display_size = spt::Math::FitWidth(frame_size, fitWidth);
    imSuperpixels = spt::TexImage(frame_display_size.width, frame_display_size.height, 3);
}

void WindowAnalyzerS::DrawMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("Save as...");
            if (ImGui::MenuItem("Close"))
                this->b_is_shown = false;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Render")) {
            ImGui::MenuItem("Fit Width", nullptr, &b_fit_width);
            ImGui::MenuItem("Resize Input", nullptr, &b_resize_input);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Superpixels")) {
            ImGui::MenuItem("gSLIC Options", nullptr, &b_gslic_options);
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

IWindow *WindowAnalyzerS::Show() {
    this->ReloadSuperpixels();
    this->b_is_shown = true;
    return dynamic_cast<IWindow *>(this);
}

void WindowAnalyzerS::ReloadSuperpixels() {
#ifdef FEATURE_GSLICR
    // Input processing
    if (b_fit_width && b_resize_input) {
        cv::resize(frame_raw, frame, frame_display_size, cv::INTER_CUBIC);
        gslic_settings.img_size = {frame_display_size.width, frame_display_size.height};
    } else {
        cv::Size frame_size = frame_raw.size();
        frame = frame_raw;
        gslic_settings.img_size = {frame_size.width, frame_size.height};
    }

    // Generate superpixels
    spt::GSLIC _superpixel(gslic_settings);
    auto superpixel = _superpixel.Compute(frame);
    superpixel->GetContour(superpixel_contour);
    cv::cvtColor(frame, frame_tex, cv::COLOR_BGR2RGB);
    frame_tex.setTo(cv::Scalar(200, 5, 240), superpixel_contour);

    // Convert to texture Format
    cv::cvtColor(frame_tex, frame_tex, cv::COLOR_RGB2RGBA);

    // Output processing
    if (b_fit_width && !b_resize_input) {
        cv::resize(frame_tex, frame_resized, frame_display_size, cv::INTER_CUBIC);
        imSuperpixels.Load(frame_resized.data);
    } else imSuperpixels.Load(frame_tex.data);
#else
    cv::cvtColor(frame, frame_tex, cv::COLOR_BGR2RGBA);
    imSuperpixels.Load(frame_tex.data);
#endif
}

