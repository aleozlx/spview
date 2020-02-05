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

class WindowFeed: public spt::AppEngine::IWindow {
protected:
    static const size_t sz_static_image_path = 512;
    char *static_image_path; // char[512]
    std::function<void(std::unique_ptr<IWindow>&&)> CreateIWindow = nullptr;
public:
    static const char *static_image_ext[];

    WindowFeed();
    ~WindowFeed() override;
    WindowFeed(const WindowFeed&) = delete;
    WindowFeed& operator=(const WindowFeed&) = delete;
    WindowFeed(WindowFeed&&) noexcept;
    bool Draw() override;
    IWindow* Show() override;
    void SetStaticImagePath(const char *src);
    void GrantCreateWindow(std::function<void(std::unique_ptr<IWindow>&&)>);
};

class WindowAnalyzerS: public spt::AppEngine::IWindow {
protected:
    bool _is_shown = false;
#ifdef FEATURE_GSLICR
    cv::Mat frame, frame_tex;
    cv::Mat superpixel_contour;
    gSLICr::objects::settings gslic_settings;
    spt::GSLIC _superpixel;
    spt::TexImage imSuperpixels;
#endif
public:
    WindowAnalyzerS(std::string src);
    bool Draw() override;
    IWindow* Show() override;
};

#endif //SPVIEW_SPVIEW_H
