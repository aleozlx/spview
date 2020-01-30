#include "WindowFeed.h"
using namespace spt::AppEngine;

bool WindowFeed::Draw() {
    ImGui::Begin("Feed");
    ImGui::Text("test");
    ImGui::End();
    return true;
}

IWindow* WindowFeed::Show() {
    return dynamic_cast<IWindow*>(this);
}
