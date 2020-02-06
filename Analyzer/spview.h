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
struct LazyLoader {
    T val, _new_val;

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

class BaseWindow: public spt::AppEngine::IWindow {
public:
    inline IWindow* Show() override {
        return dynamic_cast<IWindow*>(this);
    }
};

class WindowFeed: public BaseWindow {
protected:
    static const size_t sz_static_image_path = 512;
    char *b_static_image_path; // char[512]
    std::function<void(std::unique_ptr<IWindow>&&)> CreateIWindow = nullptr;
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
    void GrantCreateWindow(std::function<void(std::unique_ptr<IWindow>&&)>);
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
#ifdef FEATURE_GSLICR
    cv::Mat frame, frame_tex;
    cv::Mat superpixel_contour;
    gSLICr::objects::settings gslic_settings;
    spt::TexImage imSuperpixels;
#endif
};

class WindowAnalyzerS: public BaseAnalyzerWindow {
protected:
    LazyLoader<int> b_superpixel_size;

    virtual void DrawMenuBar();
    void ReloadSuperpixels();
public:
    explicit WindowAnalyzerS(const std::string &src);
    bool Draw() override;
    IWindow* Show() override;
};

class WindowAnalysisD: public BaseAnalyzerWindow {

};

#endif //SPVIEW_SPVIEW_H
