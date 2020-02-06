#include "spview.h"
using namespace spt::AppEngine;

bool WindowRegistry::Draw() {
    ImGui::Begin("Resource Registry");
    ImGui::Text("test");
    ImGui::End();
    return true;
}

