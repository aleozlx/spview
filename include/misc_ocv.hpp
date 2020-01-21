#ifndef __MISC_OCV_HPP__
#define __MISC_OCV_HPP__
#include <string>
#include <vector>
#include <opencv2/videoio.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace cv_misc {
    /// OpenCV Video I/O
    struct Camera {
        int capture_id;
        cv::VideoCapture capture;
        unsigned int width, height;
        Camera(unsigned int idx, unsigned int width, unsigned int height);
        int open();
    };

    struct CameraInfo {
        std::string name;
        int id;
        bool isOpened;
        bool Acquire();
        void Release();
    };
    
    std::vector<int> camera_enumerate();
    std::vector<CameraInfo> camera_enumerate2();
    std::string type2str(int type);

    struct Chipping {
        int width, height;
        int chip_width, chip_height;
        int nx, ny, nchip;
        int chip_stride_x, chip_stride_y;

        Chipping() {}

        /// Calculate maximum number of chips, with size `c`, that can be fit into length `L` with overlapping proportion `e`.
        static int NChip(int c, int L, float e = 0.0);

        Chipping(cv::Size input_size, cv::Size chip_size, float overlap = 0.0);

        cv::Rect GetROI(int chip_id);
    };

    namespace fx {
        void spotlight(cv::OutputArray _frame, cv::InputArray _sel, float alpha);

        // TODO investigate ImGui::PlotHistogram
        struct RGBHistogram {
            float alpha;
            unsigned int width, height;
            std::vector<cv::Mat> bgr_planes;
            bool normalize_component;
            RGBHistogram(cv::InputArray frame, unsigned int width, unsigned int height, float alpha=1.0f, bool normalize_component=false);
            void Compute(cv::OutputArray output, cv::InputArray mask=cv::noArray());
        };

        struct HSVHistogram {
            float alpha;
            unsigned int width, height;
            std::vector<cv::Mat> hsv_planes;
            bool normalize_component;
            HSVHistogram(cv::InputArray frame, unsigned int width, unsigned int height, float alpha=1.0f, bool normalize_component=false);
            void Compute(cv::OutputArray output, cv::InputArray mask=cv::noArray());
        };
    }

}
#endif
