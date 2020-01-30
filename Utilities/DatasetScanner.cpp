#if _MSC_VER
// Disables the console window on Windows
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#define _CRT_SECURE_NO_WARNINGS // before <cstdlib>
#endif

#include <vector>
#include <list>
#include <unordered_map>
#include <string>
#include <memory>
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
    size_t ct_classes = 0, ct_partitions = 0, ct_frames = 0;

    DatasetHeader() : AppEngine::FlatlandObject('h', '\x20') {

    }

    void Decode(const char *src, size_t buffer_size) {
        buffer_size = ValidBufferSize(src, buffer_size);
        this->PushValues(src + 2, buffer_size - 2);
        while (!values.empty()) {
            std::string kv = values.front();
            values.pop_front();
            const auto eq = kv.find('=');
            if (eq != std::string::npos) {
                std::string key = kv.substr(0, eq);
                std::string val = kv.substr(eq + 1);
                if (key == "classes") {
                    ct_classes = std::stoi(val);
                } else if (key == "parts") {
                    ct_partitions = std::stoi(val);
                } else if (key == "images") {
                    ct_frames = std::stoi(val);
                }
            }
        }
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
    std::string cname;

    Partition(const char *src, size_t buffer_size) : AppEngine::FlatlandObject('p', '+', src, buffer_size) {
        count = std::stoi(values.front());
        values.pop_front();
        name = values.front();
        values.pop_front();
        cname = values.front();
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

void invoke_dirscan(DatasetHeader &header, std::vector<Class> &image_classes, std::vector<Partition> &image_partitions,
                    std::unordered_map<std::string, std::list<Partition *>> &classPartitions, std::vector<Frame> &frames) {
    AppEngine::Pipe scanner;
    AppEngine::LineBuffer dirscan_line;
    image_classes.clear();
    image_partitions.clear();
	classPartitions.clear();
    frames.clear();
    snprintf(cmd_buffer, sizeof(cmd_buffer), "%s \"%s\"", cmd_dirscan, dataset_path);
    scanner.Open(cmd_buffer);
    static char pipe_buffer[8 << 10];
    size_t r = 0;
    do {
        r = scanner.Read(pipe_buffer, sizeof(pipe_buffer));
        while (dirscan_line.GetLine(pipe_buffer, r)) {
            switch (*dirscan_line.buffer) {
                case 'h':
                    header.Decode(dirscan_line.buffer, dirscan_line.buffer_size);
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
        }
    } while (r != 0);
    scanner.Close();
	for (auto &part : image_partitions) {
		auto &entry = classPartitions[part.cname];
		entry.push_back(&part);
	}
}

int main(int argc, char *argv[]) {
    if (argc == 2)
        std::strncpy(dataset_path, argv[1], sizeof(dataset_path));
    auto app = AppEngine::App::Initialize();
    if (!app.ok) return 1;
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    DatasetHeader header;
    std::vector<Class> image_classes;
    std::vector<Partition> image_partitions;
    std::unordered_map<std::string, std::list<Partition *>> classPartitions;
    std::vector<Frame> frames;

    while (app.EventLoop()) {
        ImGui::Begin("Dataset Scanner");
        if (ImGui::Button("Scan"))
            invoke_dirscan(header, image_classes, image_partitions, classPartitions, frames);
        ImGui::SameLine();
        if (ImGui::Button("Dataset...")) {
            const char *pth = tinyfd_selectFolderDialog("Select dataset path", dataset_path);
            if (pth) std::strncpy(dataset_path, pth, sizeof(dataset_path));
        }
        ImGui::SameLine();
        ImGui::InputText("", dataset_path, sizeof(dataset_path));
        ImGui::Text("Classes: %zu  Partitions: %zu  Images: %zu", header.ct_classes, header.ct_partitions,
                    header.ct_frames);
        for (const auto &cls : image_classes) {
            if (ImGui::TreeNode(cls.name.c_str())) {
				for (const auto part : classPartitions.at(cls.name))
					ImGui::Text("%s (%zu images)", part->name.c_str(), part->count);
                ImGui::TreePop();
            }
        }

        ImGui::End();
        app.Render(clear_color);
    }

    return 0;
}
