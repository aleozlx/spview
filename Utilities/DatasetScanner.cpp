#if _MSC_VER
// Disables the console window on Windows
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#define _CRT_SECURE_NO_WARNINGS // before <cstdlib>
#endif

#include <vector>
#include <string>
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

struct DatasetHeader : AppEngine::FlatlandObject {
    DatasetHeader(const char *src, size_t buffer_size) : AppEngine::FlatlandObject('h', '\x20', src, buffer_size) {
    }
};

struct Class : AppEngine::FlatlandObject {
    size_t count;
    std::string name;

    Class(const char *src, size_t buffer_size) : AppEngine::FlatlandObject('c', '+', src, buffer_size) {
		count = std::stoi(values.front());
		values.pop_front();
		name = values.front();
		values.pop_front();
    }
};

struct Partition : AppEngine::FlatlandObject {
    size_t count;
    std::string name;

    Partition(const char *src, size_t buffer_size) : AppEngine::FlatlandObject('p', '+', src, buffer_size) {
        count = std::stoi(values.front());
        values.pop_front();
		name = values.front();
		values.pop_front();
    }
};

struct Frame : AppEngine::FlatlandObject {
    std::string fname, cname, pname;

    Frame(const char *src, size_t buffer_size) : AppEngine::FlatlandObject('f', '+', src, buffer_size) {
        fname = values.front();
        values.pop_front();
        cname = values.front();
        values.pop_front();
        pname = values.front();
        values.pop_front();
    }
};

int main(int argc, char *argv[]) {
    if (argc == 2)
        std::strncpy(dataset_path, argv[1], sizeof(dataset_path));
    auto app = AppEngine::App::Initialize();
    if (!app.ok) return 1;
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    std::vector<DatasetHeader> headers;
    std::vector<Class> image_classes;
    std::vector<Partition> image_partitions;
    std::vector<Frame> frames;
    AppEngine::LineBuffer dirscan_line;
    static unsigned long szOutput = 0;
    while (app.EventLoop()) {
        ImGui::Begin("Dataset Scanner");
        if (ImGui::Button("Scan")) {
            AppEngine::Pipe scanner;
            snprintf(cmd_buffer, sizeof(cmd_buffer), "%s \"%s\"", cmd_dirscan, dataset_path);
            scanner.Open(cmd_buffer);
            static char pipe_buffer[8 << 10];
            size_t r = 0;
            do {
                r = scanner.Read(pipe_buffer, sizeof(pipe_buffer));
                while (dirscan_line.GetLine(pipe_buffer, r)) {
                    switch (*dirscan_line.buffer) {
                        case 'h':
                            headers.emplace_back(dirscan_line.buffer, dirscan_line.buffer_size);
                            break;
                        case 'c':
                            image_classes.emplace_back(dirscan_line.buffer, dirscan_line.buffer_size);
                            break;
                        case 'p':
                            image_partitions.emplace_back(dirscan_line.buffer, dirscan_line.buffer_size);
                            break;
                        case 'f':
                            frames.emplace_back(dirscan_line.buffer, dirscan_line.buffer_size);
                            break;
                    }
                    //szOutput += 1;
                }
                //szOutput += r;
            } while (r != 0);
            szOutput = frames.size();
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
