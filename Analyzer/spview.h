#ifndef SPVIEW_SPVIEW_H
#define SPVIEW_SPVIEW_H

#if _MSC_VER
#define _CRT_SECURE_NO_WARNINGS // before <cstdlib>
#endif

#include <functional>
#include <memory>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/core/utility.hpp>
#include "app.hpp"
#include "tinyfiledialogs.h"
#include "teximage.hpp"
#include "superpixel.hpp"

template<typename T>
class LazyLoader {
protected:
    T _new_val;
public:
    T val;

    explicit LazyLoader(T init_val) : val(init_val) {

    }

    /// Sync value to a temp var for data binding
    T *operator&() { // NOLINT I know exactly what I am doing!
        _new_val = val;
        return &_new_val;
    }

    /// Sync value back and detect changes
    bool Update() {
        return Update(_new_val);
    }

    /// Update value and detect changes
    bool Update(T new_val) {
        if (new_val == val) return false;
        else {
            val = new_val;
            return true;
        }
    }
};

template<typename T>
class LazyEnumLoader {
protected:
    int _new_val;
public:
    T val;

    explicit LazyEnumLoader(T init_val) : val(init_val) {

    }

    /// Sync value to a temp var for data binding
    void BeginUpdate() {
        _new_val = val;
    }

    /// Redirect to the temp var
    int *operator&() { // NOLINT I know exactly what I am doing!
        return &_new_val;
    }

    /// Sync value back and detect changes
    bool Update() {
        if (_new_val == val) return false;
        else {
            val = static_cast<T>(_new_val);
            return true;
        }
    }
};

class BaseWindow: public spt::AppEngine::IWindow {
public:
    inline IWindow* Show() override {
        return dynamic_cast<IWindow*>(this);
    }
};

#define FNAME_BUFFER_SIZE (512)

#define CREATE_BUFFER(BUF_NAME) do { \
    this->BUF_NAME = new char[FNAME_BUFFER_SIZE]; \
    this->BUF_NAME[0] = '\0'; \
} while (0)

#define MOVE_BUFFER(BUF_NAME) do { \
    this->BUF_NAME = o.BUF_NAME; \
    o.BUF_NAME = nullptr; \
} while (0)

class WindowFeed: public BaseWindow, public spt::AppEngine::CreatorIWindow {
protected:
    char *b_static_image_path;
    char *b_video_path;
    int b_camera_selection = 0;
public:
    static const char *static_image_ext[];
    static const char *static_video_ext[];

    WindowFeed();
    ~WindowFeed() override;
    WindowFeed(const WindowFeed&) = delete;
    WindowFeed& operator=(const WindowFeed&) = delete;
    WindowFeed(WindowFeed&&) noexcept;
    bool Draw() override;
    void SetStaticImagePath(const char *src);
};

class WindowRegistry: public BaseWindow {
protected:
public:
    bool Draw() override;
};

class BaseAnalyzerWindow: public BaseWindow {
protected:
    bool b_is_shown = false;
    std::string title;
    int fit_width = 800;
    bool b_fit_width = true;
    bool b_resize_input = true;
    bool b_gslic_options = true;
    ImVec4 b_boundary_color = {200.f/255, 5.f/255, 240.f/255, 0.f};
#ifdef FEATURE_GSLICR
    cv::Mat frame_raw; // directly from file
    cv::Mat frame; // input processing
    cv::Mat frame_tex;
    cv::Size frame_display_size;
    cv::Mat frame_resized; // output processing
    cv::Mat superpixel_contour; // visualized superpixel segmentation
    gSLICr::objects::settings gslic_settings;
    spt::TexImage imSuperpixels; // texture for display
#endif
};

class WindowAnalyzerS: public BaseAnalyzerWindow, public spt::AppEngine::CreatorIWindow {
protected:
    LazyLoader<int> b_superpixel_size;
    LazyLoader<int> b_superpixel_compactness;
    LazyLoader<bool> b_superpixel_enforce_conn;
    LazyEnumLoader<gSLICr::COLOR_SPACE> b_superpixel_colorspace;
    bool s_wSetDisplaySize = false;

    virtual void DrawMenuBar();
    void ReloadSuperpixels();
    void TexFitWidth(int fitWidth);
    void ManualResize(cv::Size new_size);
public:
    static const char *static_image_ext[];
    explicit WindowAnalyzerS(const std::string &src);
    bool Draw() override;
    IWindow* Show() override;
    void SaveOutput(const std::string &pth) const;

    void UIgSLICOptions();
};

class WindowAnalysisD: public BaseAnalyzerWindow {

};

#endif //SPVIEW_SPVIEW_H
