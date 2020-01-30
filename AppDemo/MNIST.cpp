#if _MSC_VER
// Disables the console window on Windows
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#define _CRT_SECURE_NO_WARNINGS // before <cstdlib>
#endif

#include "app.hpp"
#include "teximage.hpp"
#include "imgui_plot.h"
#include "tinyfiledialogs.h"
#include <dlib/dnn.h>
#include <dlib/data_io.h>

using namespace spt;
using namespace dlib;

const size_t max_buf_size = 16<<10;
static float x_data[max_buf_size];
static float y_data1[max_buf_size];
static float y_data2[max_buf_size];
static std::atomic_int buf_size = 0;
char dataset_path[512] = "<Dataset Location>";

class MNISTModel {
protected:
    std::vector<matrix<unsigned char>> training_images;
    std::vector<unsigned long> training_labels;
    std::vector<matrix<unsigned char>> testing_images;
    std::vector<unsigned long> testing_labels;
    std::atomic_bool stop_training = false;
    enum ModelState {
        Uninitialized, Ready, Started, Stopped, Completed
    };
    std::atomic<ModelState> state{Uninitialized};
    unsigned long num_epoch;
    unsigned long steps_per_epoch;
public:
    //@formatter:off
    using net_type = loss_multiclass_log<
            fc<10,
            relu<fc<84,
            relu<fc<120,
            max_pool<2, 2, 2, 2, relu<con<16, 5, 5, 1, 1,
            max_pool<2, 2, 2, 2, relu<con<6, 5, 5, 1, 1,
            input<matrix<unsigned char>>
            >>>>>>>>>>>>;
    //@formatter:on
    net_type net;
    dnn_trainer<net_type> trainer;

    MNISTModel() : trainer(net) {
        trainer.set_learning_rate(0.01);
        trainer.set_min_learning_rate(0.00001);
        trainer.set_iterations_without_progress_threshold(500);
        trainer.set_mini_batch_size(128);
        trainer.be_quiet();
    }

    bool Initialize(const char *pthDataset) {
        if (state == Uninitialized) {
            load_mnist_dataset(pthDataset, training_images, training_labels, testing_images, testing_labels);
            num_epoch = training_images.size();
            steps_per_epoch = num_epoch / trainer.get_mini_batch_size();
            state = Ready;
            return true;
        } else return false;
    }

    void Start() {
        if (state != Ready) return;
        std::thread task_train(&MNISTModel::TrainLoop, this);
        task_train.detach();
        state = Started;
    }

    void Stop() {
        if (state == Started) {
            stop_training = true;
            state = Stopped;
        }
    }

    std::pair<unsigned long, unsigned long> GetProgress() {
        if (state == Uninitialized) {
            return std::make_pair(0, 0);
        } else {
            unsigned long total_steps = trainer.get_train_one_step_calls();
            return std::make_pair(total_steps / steps_per_epoch, total_steps % steps_per_epoch);
        }
    }

    bool IsRunning() {
        return state == Started;
    }

    unsigned long GetStepsPerEpoch() {
        return state == Uninitialized ? 0 : steps_per_epoch;
    }

    float GetLoss() {
        return static_cast<float>(trainer.get_average_loss());
    }

protected:
    void TrainLoop() {
        auto steps_per_epoch = GetStepsPerEpoch();
        std::vector<matrix<unsigned char>> mini_batch_samples;
        std::vector<unsigned long> mini_batch_labels;
        dlib::rand rnd(time(0));
        while (trainer.get_learning_rate() >= 1e-6 && !stop_training) {
            auto step = trainer.get_train_one_step_calls(); // 0-based step index

            mini_batch_samples.clear();
            mini_batch_labels.clear();
            while (mini_batch_samples.size() < 128) {
                auto idx = rnd.get_random_32bit_number() % training_images.size();
                mini_batch_samples.push_back(training_images[idx]);
                mini_batch_labels.push_back(training_labels[idx]);
            }
            trainer.train_one_step(mini_batch_samples, mini_batch_labels);

            if (step % steps_per_epoch == 0) {
                mini_batch_samples.clear();
                mini_batch_labels.clear();
                while (mini_batch_samples.size() < 128) {
                    auto idx = rnd.get_random_32bit_number() % testing_images.size();
                    mini_batch_samples.push_back(testing_images[idx]);
                    mini_batch_labels.push_back(testing_labels[idx]);
                }
                trainer.test_one_step(mini_batch_samples, mini_batch_labels);
                auto iTest = trainer.get_test_one_step_calls() - 1;
            }
            x_data[step] = static_cast<float>(step);
            y_data1[step] = static_cast<float>(trainer.get_average_loss());
            y_data2[step] = static_cast<float>(trainer.get_average_test_loss()); // TODO Space optimization wanted
            buf_size += 1;
        }
        trainer.get_net();
        net.clean();
        state = Completed;
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

    MNISTModel model;

    const char *btnStartStop;
    auto t0_train_progress = std::chrono::high_resolution_clock::now();
    auto train_progress = model.GetProgress();

    const float *y_data[] = {y_data1, y_data2};
    ImU32 colors[3] = {ImColor(255, 255, 255), ImColor(200, 150, 0)};
    ImGui::PlotConfig conf;
    conf.values.xs = x_data;
    conf.values.ys_list = y_data;
    conf.values.ys_count = 2;
    conf.values.colors = colors;
    conf.scale.min = -0.1;
    conf.scale.max = 1.5;
    conf.grid_y.show = true;
    conf.grid_y.size = 0.5f;
    conf.grid_y.subticks = 5;
    conf.frame_size = ImVec2(500, 200);

    while (AppEngine::App::EventLoop()) {
        ImGui::Begin("MNIST");
        btnStartStop = model.IsRunning() ? "Stop" : "Run";
        if (ImGui::Button(btnStartStop)) {
            if (model.IsRunning()) {
                model.Stop();
            } else {
                if (model.Initialize(dataset_path))
                    model.Start();
                else
                    tinyfd_messageBox(
                            "MNIST",
                            "Restart the program to train the model again.",
                            "Ok", "info", 1);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Dataset...")) {
            const char *pth = tinyfd_selectFolderDialog("Select dataset path", dataset_path);
            if (pth) std::strncpy(dataset_path, pth, sizeof(dataset_path));
        }
        ImGui::SameLine();
        ImGui::InputText("", dataset_path, sizeof(dataset_path));

        ImGui::Dummy(ImVec2(0, 5));
        auto steps_per_epoch = model.GetStepsPerEpoch();
        if (steps_per_epoch > 0) {
            auto t1 = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float, std::milli> elapsed = t1 - t0_train_progress;
            if (elapsed.count() > 10.f) {
                train_progress = model.GetProgress();
                t0_train_progress = t1;
            }

            ImGui::Text("Epoch");
            ImGui::SameLine(70);
            ImGui::ProgressBar(std::min(static_cast<float>(train_progress.first) / 20, 1.f));
            ImGui::Text("Step");
            ImGui::SameLine(70);
            ImGui::ProgressBar(static_cast<float>(train_progress.second) / steps_per_epoch);
        }

        conf.values.count = buf_size;
        ImGui::Plot("Loss", conf);
        ImGui::End();
        AppEngine::App::Render(clear_color);
    }

    return 0;
}
