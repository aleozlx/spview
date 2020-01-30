#ifndef SPVIEW_WINDOWFEED_H
#define SPVIEW_WINDOWFEED_H

#include "app.hpp"

class WindowFeed: public spt::AppEngine::IWindow {
public:
    bool Draw() override;
    IWindow* Show() override;
};

#endif //SPVIEW_WINDOWFEED_H
