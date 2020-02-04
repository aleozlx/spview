#ifndef SPVIEW_SPVIEW_H
#define SPVIEW_SPVIEW_H

#if _MSC_VER
#define _CRT_SECURE_NO_WARNINGS // before <cstdlib>
#endif

#include <functional>
#include <memory>
#include "app.hpp"
#include "tinyfiledialogs.h"

class WindowFeed: public spt::AppEngine::IWindow {
protected:
    static const size_t sz_static_image_path = 512;
    char *static_image_path; // char[512]
    std::function<void(std::unique_ptr<IWindow>&&)> CreateWindow = nullptr;
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

#endif //SPVIEW_SPVIEW_H
