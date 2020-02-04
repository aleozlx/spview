#ifndef SPVIEW_WINDOWFEED_H
#define SPVIEW_WINDOWFEED_H

#if _MSC_VER
#define _CRT_SECURE_NO_WARNINGS // before <cstdlib>
#endif

#include <memory>
#include "app.hpp"
#include "tinyfiledialogs.h"

class WindowFeed: public spt::AppEngine::IWindow {
protected:
    static const size_t sz_static_image_path = 512;
    char *static_image_path; // char[512]
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
};

#endif //SPVIEW_WINDOWFEED_H
