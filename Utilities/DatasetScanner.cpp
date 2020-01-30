#if _MSC_VER
// Disables the console window on Windows
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#define _CRT_SECURE_NO_WARNINGS // before <cstdlib>
#endif

#include "app.hpp"
#include "teximage.hpp"
#include "tinyfiledialogs.h"

using namespace spt;

char dataset_path[512];
char cmd_buffer[512];
#if _MSC_VER
const char *cmd_dirscan = "dirscan.exe";
#else
const char *cmd_dirscan = "dirscan";
#endif

int main(int argc, char *argv[]) {
    if (argc == 2)
        std::strncpy(dataset_path, argv[1], sizeof(dataset_path));
    auto app = AppEngine::App::Initialize();
    if (!app.ok) return 1;
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    static unsigned long szOutput = 0;
    while (app.EventLoop()) {
        ImGui::Begin("Dataset Scanner");
        if (ImGui::Button("Scan")) {
            AppEngine::Pipe scanner;
            snprintf(cmd_buffer, sizeof(cmd_buffer), "%s \"%s\"", cmd_dirscan, dataset_path);
            scanner.Open(cmd_buffer);
            static char pipe_buffer[4<<10];
            size_t r = 0;
            do {
                r = scanner.Read(pipe_buffer, sizeof(pipe_buffer));
                szOutput += r;
            } while (r != 0);
            scanner.Close();
        }
        ImGui::SameLine();
        if (ImGui::Button("Dataset...")) {
            const char *pth = tinyfd_selectFolderDialog("Select dataset path", dataset_path);
            if (pth) std::strncpy(dataset_path, pth, sizeof(dataset_path));
        }
        ImGui::SameLine();
        ImGui::InputText("", dataset_path, sizeof(dataset_path));

        ImGui::Text("%lu", szOutput);
        ImGui::End();
        app.Render(clear_color);
    }

    return 0;
}
