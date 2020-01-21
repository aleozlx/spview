#ifndef __DCNN_HPP__
#define __DCNN_HPP__

#ifdef HAS_TF
#include <string>
#include <memory>
#include <cstdlib>
#include <opencv2/core/utility.hpp>
#include <opencv2/imgproc.hpp>
#include <tensorflow/core/public/session.h>
#include <tensorflow/core/platform/env.h>

namespace spt::dnn {
    class TensorFlowInference {
    public:
        tensorflow::GraphDef graph;
        tensorflow::Session *session = nullptr;

        TensorFlowInference(std::string const &graph_path);

        virtual ~TensorFlowInference();

        virtual bool NewSession();
        virtual bool NewSession(const tensorflow::SessionOptions &config);

        virtual void Summary();

        // virtual void Compute(cv::InputArray frame) {}

    protected:
        bool _loaded;
    };

    class IComputeFrame {
    public:
        virtual void Compute(cv::InputArray frame) = 0;
    };

    class IComputeFrameSuperpixel {
    public:
        virtual IComputeFrameSuperpixel* Compute(cv::InputArray frame, cv::InputArray superpixels) = 0;
        virtual int GetFeatureDim() const = 0;
        virtual int GetNSP() const = 0;
        virtual void GetFeature(float *output_array) const = 0;
        virtual void GetFeature(int superpixel_id, float *output_array) const = 0;
    };

    class VGG16 : public TensorFlowInference, public IComputeFrame {
    public:
        VGG16();

        void SetInputResolution(unsigned int width, unsigned int height);

        void Compute(cv::InputArray frame) override;

    protected:
        tensorflow::TensorShape input_shape;
        tensorflow::Tensor input_tensor;
        std::vector<std::pair<std::string, tensorflow::Tensor>> inputs;
        std::vector<tensorflow::Tensor> outputs;
    };

    class VGG16SP : public TensorFlowInference, public IComputeFrameSuperpixel {
    public:
        VGG16SP();

        void SetInputResolution(unsigned int width, unsigned int height);

        IComputeFrameSuperpixel* Compute(cv::InputArray frame, cv::InputArray superpixels) override;

        int GetFeatureDim() const override;

        int GetNSP() const override;

        void GetFeature(float *output_array) const override;

        void GetFeature(int superpixel_id, float *output_array) const override;

    protected:
        tensorflow::TensorShape input_shape;
        tensorflow::Tensor input_tensor;
        tensorflow::TensorShape superpixel_shape;
        tensorflow::Tensor superpixel_tensor;
        std::vector<std::pair<std::string, tensorflow::Tensor>> inputs;
        std::vector<tensorflow::Tensor> outputs;
    };
}

#endif // HAS_TF

#endif
