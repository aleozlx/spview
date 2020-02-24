#include <iostream>
#include <random>
#include <thrust/host_vector.h>
#include <thrust/device_vector.h>

template<typename T>
struct SOMNetwork {
    size_t units, dims;
    T *weights = nullptr; // W
    bool *connections = nullptr; // C
    unsigned *conn_ages = nullptr; // T
    T *errors = nullptr; // E
};

template<typename T>
void AllocCPU(SOMNetwork<T> &network, size_t units, size_t dims) {
    network.units = units;
    network.dims = dims;
    network.weights = new T[units * dims];
    network.connections = new bool[units * units];
    network.conn_ages = new unsigned[units * units];
    network.errors = new T[units];
}

template<typename T>
void AllocGPU(SOMNetwork<T> &network, size_t units, size_t dims) {
	network.units = units;
	network.dims = dims;
	cudaMallocManaged(&network.weights, units*dims * sizeof(T));
	cudaMallocManaged(&network.connections,units * units*sizeof(bool));
	cudaMallocManaged(&network.conn_ages,units * units*sizeof(unsigned));
	cudaMallocManaged(&network.errors,units*sizeof(T));
}

template<typename T>
void DeallocCPU(SOMNetwork<T> &network) {
    delete[] network.weights;
    delete[] network.connections;
    delete[] network.conn_ages;
    delete[] network.errors;
}

template<typename T>
void DeallocGPU(SOMNetwork<T> &network) {
	cudaFree(network.weights);
	cudaFree(network.connections);
	cudaFree(network.conn_ages);
	cudaFree(network.errors);
}

template<typename T>
void InitUnified(SOMNetwork<T> &network, const size_t units, const size_t dims) {
	// Network parameters are on unified memory
	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_real_distribution<T> dist(-2, 2);
	for (int i = 0; i < units * dims; ++i)
		network.weights[i] = dist(rng);
	for (int i = 0; i < units; ++i)
		for (int j = 0; j < units; ++j)
			network.connections[i * units + j] = i != j;
	for (int i = 0; i < units * units; ++i)
		network.conn_ages = 0;
	for (int i = 0; i < units; ++i)
		network.errors = 0;
}

template<typename T>
void InitCPU(SOMNetwork<T> &network, const size_t units, const size_t dims) {
    AllocCPU(network, units, dims);
	InitUnified(network, units, dims);
}

template<typename T>
void InitGPU(SOMNetwork<T> &network, const size_t units, const size_t dims) {
	AllocGPU(network, units, dims);
	InitUnified(network, units, dims);
}

enum Device { CPU = 1, GPU = 2 };

template<typename T>
class SOMBase {
protected:
    const T *data;
    const size_t samples, dims, units;
    SOMNetwork<T> network;
	const Device device;
public:
    SOMBase(const T *input_data, size_t samples, size_t dims, size_t units, Device dev=GPU) :
            data(input_data), samples(samples), dims(dims), units(units), device(dev) {
		switch (device) {
		case CPU: InitCPU(network, units, dims); break;
		case GPU: InitGPU(network, units, dims); break;
		}
    }

    virtual ~SOMBase() {
		switch (device) {
		case CPU: DeallocCPU(network); break;
		case GPU: DeallocGPU(network); break;
		}
    }

    void UpdateClusters(unsigned outlier_tolerance = 1) {

    }
};

template<typename T>
class GrowingNeuralGas : public SOMBase<T> {
public:
    GrowingNeuralGas(const T *input_data, size_t samples, size_t dims, size_t units, Device dev = GPU):
        SOMBase(input_data, samples, dims, units, dev) {

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
