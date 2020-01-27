#if _MSC_VER
// Disables the console window on Windows
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#define _CRT_SECURE_NO_WARNINGS // before <cstdlib>
#endif

#include <cstdlib>
#include "app.hpp"
#include "teximage.hpp"
#include "imgui_plot.h"
#include "tinyfiledialogs.h"

using namespace spview;

const size_t buf_size = 512;
static float x_data[buf_size];
static float y_data1[buf_size];
static float y_data2[buf_size];
static float y_data3[buf_size];
char dataset_path[512] = "<Dataset Location>";

void generate_data() {
    constexpr float sampling_freq = 44100;
    constexpr float freq = 500;
    for (size_t i = 0; i < buf_size; ++i) {
        const float t = i / sampling_freq;
        x_data[i] = t;
        const float arg = 2 * 3.14159 * freq * t;
        y_data1[i] = sin(arg);
        y_data2[i] = y_data1[i] * -0.6 + sin(2 * arg) * 0.4;
        y_data3[i] = y_data2[i] * -0.6 + sin(3 * arg) * 0.4;
    }
}

int main(int, char **) {
    auto app = AppEngine::App::Initialize();
    if (!app.ok) return 1;
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    static const float* y_data[] = { y_data1, y_data2, y_data3 };
    static ImU32 colors[3] = { ImColor(0, 255, 0), ImColor(255, 0, 0), ImColor(0, 0, 255) };
    static uint32_t selection_start = 0, selection_length = 0;
    generate_data();

    /////////////////////////////////////////////
//    char const * lTheSelectFolderName;
//    lTheSelectFolderName = tinyfd_selectFolderDialog(
//            "let us just select a directory", NULL);
//
//    if (!lTheSelectFolderName)
//    {
//        tinyfd_messageBox(
//                "Error",
//                "Select folder name is NULL",
//                "ok",
//                "error",
//                1);
//        return 1;
//    }
//
//    tinyfd_messageBox("The selected folder is",
//                      lTheSelectFolderName, "ok", "info", 1);
    ///////////////////////////////////////////////

    while (app.EventLoop()) {
        ImGui::Begin("MNIST");
        ImGui::Button("Run");
        ImGui::SameLine();
        if(ImGui::Button("Dataset...")) {
            const char *pth = tinyfd_selectFolderDialog("Select dataset path", nullptr);
            if (pth) std::strncpy(dataset_path, pth, sizeof(dataset_path));
        }
        ImGui::SameLine();
        ImGui::InputText("", dataset_path, sizeof(dataset_path));

        ImGui::Dummy(ImVec2(0, 5));
        ImGui::Text("Epoch");
        ImGui::SameLine(70);
        ImGui::ProgressBar(0.1);
        ImGui::Text("Step");
        ImGui::SameLine(70);
        ImGui::ProgressBar(0.1);



        ImGui::PlotConfig conf;
        conf.values.xs = x_data;
        conf.values.count = buf_size;
        conf.values.ys_list = y_data; // use ys_list to draw several lines simultaneously
        conf.values.ys_count = 3;
        conf.values.colors = colors;
        conf.scale.min = -1;
        conf.scale.max = 1;
        conf.tooltip.show = true;
        conf.grid_x.show = true;
        conf.grid_x.size = 128;
        conf.grid_x.subticks = 4;
        conf.grid_y.show = true;
        conf.grid_y.size = 0.5f;
        conf.grid_y.subticks = 5;
        conf.selection.show = true;
        conf.selection.start = &selection_start;
        conf.selection.length = &selection_length;
        conf.frame_size = ImVec2(buf_size, 200);
        ImGui::Plot("plot1", conf);

        // Draw second plot with the selection
        // reset previous values
        conf.values.ys_list = nullptr;
        conf.selection.show = false;
        // set new ones
        conf.values.ys = y_data3;
        conf.values.offset = selection_start;
        conf.values.count = selection_length;
        conf.line_thickness = 2.f;
        ImGui::Plot("plot2", conf);
        ImGui::End();
        app.Render(clear_color);
    }

    return 0;
}
