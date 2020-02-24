#include <iostream>
#include <random>
#include <thrust/host_vector.h>
#include <thrust/device_vector.h>

template<typename T>
struct SOMNetwork {
    size_t units, dims;
	thrust::device_vector<T> weights;
	thrust::device_vector<bool> connections;
	thrust::device_vector<unsigned> conn_ages;
	thrust::device_vector<T> errors;

    SOMNetwork(size_t units, size_t dims):
        weights(units*dims),
        connections(units*units),
        conn_ages(units*units),
        errors(units) {

    }
};

template<typename T>
void InitUnified(SOMNetwork<T> &network, const size_t units, const size_t dims) {
	// Network parameters are on unified memory
	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_real_distribution<T> dist(-2, 2);
	thrust::host_vector<T> weights(units*dims);
    thrust::host_vector<T> connections(units*units);
	for (int i = 0; i < units * dims; ++i)
		weights[i] = dist(rng);
	for (int i = 0; i < units; ++i)
		for (int j = 0; j < units; ++j)
			connections[i * units + j] = i != j;
    network.weights = weights;
    network.connections = connections;
    thrust::fill(network.conn_ages.begin(), network.conn_ages.end(), 0);
    thrust::fill(network.errors.begin(), network.errors.end(), 0);
}

template<typename T>
class SOMBase {
protected:
    const T *data;
    const size_t samples, dims, units;
    SOMNetwork<T> network;
//	const Device device;
public:
    SOMBase(const T *input_data, size_t samples, size_t dims, size_t units) :
            data(input_data), samples(samples), dims(dims), units(units), network(units, dims) {
        InitUnified(network, units, dims);
    }

    virtual ~SOMBase() {

    }

    void UpdateClusters(unsigned outlier_tolerance = 1) {

    }
};

template<typename T>
class GrowingNeuralGas : public SOMBase<T> {
public:
    GrowingNeuralGas(const T *input_data, size_t samples, size_t dims, size_t units):
        SOMBase(input_data, samples, dims, units) {

    }

    virtual void Fit(T e_epsilon, T e_lambda, const unsigned MAX_AGE, const unsigned STEP_NEW_UNIT, T ERR_DECAY_GLOBAL, const unsigned N_PASS) {
        const size_t DIM_DATA = network.dims;
        unsigned sequence = 0;
        for(unsigned p = 0; p<N_PASS; ++p) {
            std::cout<<"    Pass #"<<(p+1)<<std::endl;
            // TODO shuffle data
//            steps =
        }
    }
};

int main(int, char **) {
//    v_network_ctor(5, 5);
    GrowingNeuralGas<float> gng(nullptr, 20, 5, 16);

    return 0;
}
