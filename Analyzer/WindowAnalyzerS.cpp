#include <utility>

#include "spview.h"

using namespace spt::AppEngine;

std::string getFname(std::string fname) {
    const size_t i_slash = fname.find_last_of("\\/");
    if (std::string::npos != i_slash)
        fname.erase(0, i_slash + 1);
    return fname;
}

const char *WindowAnalyzerS::static_image_ext[] = {"*.jpg", "*.png"};

WindowAnalyzerS::WindowAnalyzerS(const std::string &src) {
    frame_raw = cv::imread(src);
    cv::Size frame_size = frame_raw.size();
    if (b_fit_width) {
        frame_display_size = spt::Math::FitWidth(frame_size, fit_width);
        imSuperpixels = spt::TexImage(frame_display_size.width, frame_display_size.height, 3);
    } else {
        imSuperpixels = spt::TexImage(frame_size.width, frame_size.height, 3);
    }
    if (b_resize_input) {
        gslic_settings.img_size = {frame_display_size.width, frame_display_size.height};
    } else {
        gslic_settings.img_size = {frame_size.width, frame_size.height};
    }
    gslic_settings.no_segs = 64;
    gslic_settings.spixel_size = 18;
    gslic_settings.no_iters = 5;
    gslic_settings.coh_weight = 0.6f;
    gslic_settings.do_enforce_connectivity = true;
    gslic_settings.color_space = gSLICr::CIELAB;
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

    // Render texture
    ImGui::Image(imSuperpixels.id(), imSuperpixels.size(), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
                 ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
    const cv::Size frame_size = frame.size();
    ImGui::Text("True size: %d x %d;  Display size: %d x %d",
                frame_size.width, frame_size.height, frame_display_size.width, frame_display_size.height);
    ImGui::Text("Aspect ratio: %s", spt::Math::AspectRatioSS(frame_display_size));
    ImGui::ColorEdit4("Boundary color", reinterpret_cast<float*>(&b_boundary_color));
    ImGui::End();
    return b_is_shown;
}

void WindowAnalyzerS::TexFitWidth(const int fitWidth) {
    fit_width = fitWidth;
    const cv::Size frame_size = frame_raw.size();
    frame_display_size = spt::Math::FitWidth(frame_size, fitWidth);
    imSuperpixels = spt::TexImage(frame_display_size.width, frame_display_size.height, 3);
}

template<typename S>
struct ResultSetDisplaySize {
    const bool yes;
    const S new_size;

    static ResultSetDisplaySize<S> Yes(const S new_size) {
        return {true, new_size};
    }

    static ResultSetDisplaySize<S> No() {
        return {false};
    }
};

template<typename S>
class WindowSetDisplaySize : public BaseWindow {
protected:
    bool *singleton;
    int b_width, b_height;
    bool b_fixed_aspect = true;
    float init_aspect;
    std::function<void(ResultSetDisplaySize<S>)> callback;
public:
    WindowSetDisplaySize(bool *singleton, const S &display_size, float init_aspect, std::function<void(ResultSetDisplaySize<S>)> &&callback) :
            singleton(singleton),
            b_width(display_size.width),
            b_height(display_size.height),
            init_aspect(init_aspect),
            callback(callback) {
    }

    bool Draw() override {
        ImGui::Begin("Set display size", singleton);
        ImGui::Checkbox("Match input aspect ratio", &b_fixed_aspect);
        if(b_fixed_aspect)
            ImGui::Text("Input aspect ratio: %s", spt::Math::AspectRatioSS(init_aspect));
        ImGui::InputInt("width", &b_width);
        if(b_fixed_aspect) {
            b_height = static_cast<int>(static_cast<float>(b_width) / init_aspect);
        }
        if(b_fixed_aspect)
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        ImGui::InputInt("height", &b_height);
        if(b_fixed_aspect)
            ImGui::PopStyleVar();
        ImGui::Separator();
        if (ImGui::Button("Apply")) {
            *singleton = false;
            if (callback)
                callback(ResultSetDisplaySize<S>::Yes(S(b_width, b_height)));
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            *singleton = false;
            if (callback)
                callback(ResultSetDisplaySize<S>::No());
        }
        ImGui::End();
        return *singleton;
    }
};

class WindowGSlicOptions : public BaseWindow {
protected:
    bool *singleton;
    gSLICr::objects::settings *gslic_settings;
    std::function<void()> callback;
    LazyLoader<int> b_superpixel_size;
    LazyLoader<int> b_superpixel_compactness;
    LazyLoader<int> b_superpixel_iterations;
    LazyLoader<bool> b_superpixel_enforce_conn;
    LazyEnumLoader<gSLICr::COLOR_SPACE> b_superpixel_colorspace;
public:
    WindowGSlicOptions(bool *singleton, gSLICr::objects::settings *options, std::function<void()> &&callback):
            singleton(singleton),
            gslic_settings(options),
            callback(callback),
            b_superpixel_size(18),
            b_superpixel_compactness(6),
            b_superpixel_iterations(5),
            b_superpixel_enforce_conn(true),
            b_superpixel_colorspace(gSLICr::CIELAB) {
    }

    bool Draw() override {
        ImGui::Begin("gSLIC Options", singleton);
        ImGui::SliderInt("Superpixel Size", &b_superpixel_size, 8, 64);
        if (b_superpixel_size.Update()) {
            gslic_settings->spixel_size = b_superpixel_size.val;
            callback();
        }

        ImGui::SliderInt("Compactness", &b_superpixel_compactness, 1, 20);
        if (b_superpixel_compactness.Update()) {
            gslic_settings->coh_weight = ((float) b_superpixel_compactness.val) / 10.f;
            callback();
        }

        ImGui::Checkbox("Enforce Connectivity", &b_superpixel_enforce_conn);
        if (b_superpixel_enforce_conn.Update()) {
            gslic_settings->do_enforce_connectivity = b_superpixel_enforce_conn.val;
            callback();
        }

        ImGui::SliderInt("Iterations", &b_superpixel_iterations, 1, 20);
        if (b_superpixel_iterations.Update()) {
            gslic_settings->no_iters = b_superpixel_iterations.val;
            callback();
        }

        b_superpixel_colorspace.BeginUpdate();
        ImGui::RadioButton("CIELAB", &b_superpixel_colorspace, gSLICr::CIELAB);
        ImGui::SameLine();
        ImGui::RadioButton("XYZ", &b_superpixel_colorspace, gSLICr::XYZ);
        ImGui::SameLine();
        ImGui::RadioButton("RGB", &b_superpixel_colorspace, gSLICr::RGB);
        if (b_superpixel_colorspace.Update()) {
            gslic_settings->color_space = b_superpixel_colorspace.val;
            callback();
        }
        ImGui::End();
        return *singleton;
    }
};

void WindowAnalyzerS::DrawMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save as...")) {
                const char *pth = tinyfd_saveFileDialog("Save image", nullptr,
                                                        IM_ARRAYSIZE(static_image_ext), static_image_ext,
                                                        nullptr);
                if(pth)
                    this->SaveOutput(pth);
            }
            if (ImGui::MenuItem("Close"))
                this->b_is_shown = false;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Render")) {
            if (ImGui::MenuItem("Refresh"))
                this->ReloadSuperpixels();
            ImGui::Separator();
            ImGui::MenuItem("Fit Width", nullptr, &b_fit_width);
            ImGui::MenuItem("Resize Input", nullptr, &b_resize_input);
            if (ImGui::MenuItem("Set Display Size...") && !swSetDisplaySize) {
                this->swSetDisplaySize = true;
                auto w = std::make_unique<WindowSetDisplaySize<cv::Size>>(
                        &this->swSetDisplaySize,
                        frame_display_size,
                        spt::Math::AspectRatio(frame_raw.size()),
                        [=](const ResultSetDisplaySize<cv::Size> &r) {
                            if (r.yes) this->ManualResize(r.new_size);
                            this->swSetDisplaySize = false;
                        });
                this->CreateIWindow(std::move(w));
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Superpixels")) {
            if (ImGui::MenuItem("gSLIC Options...") && !swgSLICOptions) {
                this->swgSLICOptions = true;
                auto w = std::make_unique<WindowGSlicOptions>(
                        &this->swgSLICOptions,
                        &this->gslic_settings,
                        std::bind(&WindowAnalyzerS::ReloadSuperpixels, this));
                this->CreateIWindow(std::move(w));
            }
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

void WindowAnalyzerS::ManualResize(cv::Size new_size) {
    frame_display_size = std::move(new_size);
    imSuperpixels = spt::TexImage(frame_display_size.width, frame_display_size.height, 3);
    ReloadSuperpixels();
}

void WindowAnalyzerS::SaveOutput(const std::string &pth) const {
    cv::Mat frame_save;
    cv::cvtColor(frame_tex, frame_save, cv::COLOR_RGBA2BGR);
    cv::imwrite(pth, frame_save);
}

IWindow *WindowAnalyzerS::Show() {
    this->ReloadSuperpixels();
    this->b_is_shown = true;
    return dynamic_cast<IWindow *>(this);
}

void WindowAnalyzerS::ReloadSuperpixels() {
#ifdef FEATURE_GSLICR
    // Input processing
    if (b_resize_input) {
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
    cv::Scalar boundary_color(b_boundary_color.x*255, b_boundary_color.y*255, b_boundary_color.z*255);
    frame_tex.setTo(boundary_color, superpixel_contour);

    // Convert to texture Format
    cv::cvtColor(frame_tex, frame_tex, cv::COLOR_RGB2RGBA);

    // Output processing
    if (!b_resize_input) {
        cv::resize(frame_tex, frame_resized, frame_display_size, cv::INTER_CUBIC);
        imSuperpixels.Load(frame_resized.data);
    } else imSuperpixels.Load(frame_tex.data);
#else
    cv::cvtColor(frame, frame_tex, cv::COLOR_BGR2RGBA);
    imSuperpixels.Load(frame_tex.data);
#endif
}

