#include "app.hpp"
#include "teximage.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
using namespace spview;
const unsigned int WIDTH = 432;
const unsigned int HEIGHT = 240;

int main(int, char**) {
    auto app = AppEngine::App::Initialize();
    if (!app.ok) return 1;
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load("C:\\Users\\aleoz\\OneDrive\\Pictures\\Screenshots\\test.PNG", &image_width, &image_height, NULL, 4);
    TexImage im(image_width, image_height, 3);
    im.Load(image_data);

    while (app.EventLoop()){
        ImGui::Begin("DirectX11 Texture Test");
        ImGui::Text("pointer = %p", im.id());
        ImGui::Text("size = %d x %d", im.width, im.height);
        ImGui::Image(im.id(), ImVec2(im.width, im.height));

//        for (auto w = windows.begin(); w != windows.end();) {
//            if (!(*w)->Draw()) windows.erase(w++);
//            else ++w;
//        }
        ImGui::End();
        app.Render(clear_color);
    }

    return 0;
}
