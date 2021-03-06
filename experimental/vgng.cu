#include <iostream>
#include <random>
#include <vector>
#include <queue>
#include <algorithm>
#include <cmath>
#include <cassert>
#include <thrust/host_vector.h>
#include <thrust/device_vector.h>
#include <chrono>
#include <map>

#include <thrust/device_malloc_allocator.h>
#include <thrust/system_error.h>
#include <thrust/system/cuda/error.h>

//template<class T>
//class managed_allocator : public thrust::device_malloc_allocator<T> {
//public:
//    using value_type = T;
//    typedef thrust::device_ptr<T>  pointer;
//    inline pointer allocate(size_t n) {
//        value_type* result = nullptr;
//        cudaError_t error = cudaMallocManaged(&result, n*sizeof(T), cudaMemAttachGlobal);
//        if(error != cudaSuccess)
//            throw thrust::system_error(error, thrust::cuda_category(), "managed_allocator::allocate(): cudaMallocManaged");
//        return thrust::device_pointer_cast(result);
//    }
//
//    inline void deallocate(pointer ptr, size_t){
//        cudaError_t error = cudaFree(thrust::raw_pointer_cast(ptr));
//        if(error != cudaSuccess)
//            throw thrust::system_error(error, thrust::cuda_category(), "managed_allocator::deallocate(): cudaFree");
//    }
//};

#define PROFILER_BEGIN start = std::chrono::steady_clock::now();
#define PROFILER_END(N) end = std::chrono::steady_clock::now();profiler[N] += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
std::map<unsigned, unsigned long long> profiler;

//template<typename T>
//struct SOMNetwork {
//    const size_t units, dims;
//    thrust::device_vector <T> weights; // W
//    thrust::device_vector<bool> connections; // C
//    thrust::device_vector<unsigned> conn_ages; // T
//    thrust::device_vector <T> errors; // E
//
//    SOMNetwork(size_t units, size_t dims) :
//            units(units),
//            dims(dims),
//            weights(units * dims),
//            connections(units * units),
//            conn_ages(units * units),
//            errors(units) {
//    }
//
//    void Init() {
//        std::random_device dev;
//        std::mt19937 rng(dev());
//        std::uniform_real_distribution <T> dist(-2, 2);
//        thrust::host_vector <T> _weights(units * dims);
//        thrust::host_vector <T> _connections(units * units);
//        for (int i = 0; i < units * dims; ++i)
//            _weights[i] = dist(rng);
//        for (int i = 0; i < units; ++i)
//            for (int j = 0; j < units; ++j)
//                _connections[i * units + j] = i != j;
//        this->weights = _weights;
//        this->connections = _connections;
//        thrust::fill(conn_ages.begin(), conn_ages.end(), 0);
//        thrust::fill(errors.begin(), errors.end(), 0);
//    }
//};

template<typename T>
class SOMNetworkHost {
private:
    unsigned invalid=0;
    unsigned units_cap;
public:
    size_t units, dims;
    T *weights; // W
    bool *connections; // C
    bool *d_connections = nullptr;
    unsigned *conn_ages; // T
    unsigned *d_conn_ages = nullptr;
    T *errors; // E

    SOMNetworkHost(size_t units, size_t dims) :
            units(units),
            dims(dims) {
        const unsigned units2 = units * units;
        weights = new T[units * dims];
        connections = new bool[units2];
        conn_ages = new unsigned[units2];
        errors = new T[units];
        if (cudaSuccess != cudaMalloc(&d_connections, sizeof(bool) * units2))
            std::cerr << "cudaMalloc error d_connections" << std::endl;
        if (cudaSuccess != cudaMalloc(&d_conn_ages, sizeof(unsigned) * units2))
            std::cerr << "cudaMalloc error d_conn_ages" << std::endl;
    }

    SOMNetworkHost(const SOMNetworkHost<T> &other) : units(other.units), dims(other.dims) {
        From(other);
    }

//    SOMNetworkHost(const SOMNetworkHost<T> &&other) : units(other.units), dims(other.dims) {
//        weights = other.weights;
//        connections = other.connections;
//        conn_ages = other.conn_ages;
//        errors = other.errors;
//        other.invalid = 1;
//    }

    SOMNetworkHost<T>& operator=(SOMNetworkHost<T>& other) = delete;
    SOMNetworkHost<T>& operator=(SOMNetworkHost<T>&& other) {
        if (&other == this) return *this;
        weights = other.weights;
        connections = other.connections;
        conn_ages = other.conn_ages;
        errors = other.errors;
        other.invalid = 1;
        return *this;
    }

    ~SOMNetworkHost() {
        if(!invalid) {
            delete[] weights;
            delete[] connections;
            delete[] conn_ages;
            delete[] errors;
            cudaFree(d_connections);
            cudaFree(d_conn_ages);
        }
    }

    void Init() {
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_real_distribution <T> dist(-2, 2);
        for (int i = 0; i < units * dims; ++i)
            weights[i] = dist(rng);
        for (int i = 0; i < units; ++i)
            for (int j = 0; j < units; ++j)
                connections[i * units + j] = i != j;
//        std::memset(conn_ages, sizeof(unsigned) * units * units, 0);
//        std::memset(errors, sizeof(T) * units, 0);
        std::fill(conn_ages, conn_ages+ units * units, 0);
        std::fill(errors, errors+units, 0);
//        thrust::fill(conn_ages.begin(), conn_ages.end(), 0);
//        thrust::fill(errors.begin(), errors.end(), 0);
    }

    void FromNonZero(const SOMNetworkHost<T> &a, const std::vector<unsigned> &nz) {
        assert(a.units == nz.size());
        auto w = weights;
        for (int i = 0; i < units; ++i)
            for (int j = 0; j < dims; ++j)
                *w++ = a.weights[nz[i] * a.dims + j];
        auto c = connections;
        for (int i = 0; i < units; ++i)
            for (int j = 0; j < units; ++j)
                *c++ = a.connections[nz[i] * a.units + nz[j]];
        auto t = conn_ages;
        for (int i = 0; i < units; ++i)
            for (int j = 0; j < units; ++j)
                *t++ = a.conn_ages[nz[i] * a.units + nz[j]];
        auto e = errors;
        for (int i = 0; i < units; ++i)
            e[i] = a.errors[nz[i]];
    }

    void From(const SOMNetworkHost<T> &a) {
        assert(units <= a.units);
        auto w = weights;
        for (int i = 0; i < units; ++i) {
            std::memcpy(w, &a.weights[i * a.dims], sizeof(T) * dims);
            w += dims;
        }
        auto c = connections;
        for (int i = 0; i < units; ++i) {
            std::memcpy(c, &a.connections[i * a.units], sizeof(std::remove_pointer<decltype(c)>::type) * units);
            c += units;
        }
        auto t = conn_ages;
        for (int i = 0; i < units; ++i) {
            std::memcpy(t, &a.conn_ages[i * a.units], sizeof(unsigned) * units);
            t += units;
        }
        auto e = errors;
        std::memcpy(e, a.errors, sizeof(T) * units);
    }
};

template<typename T>
class Nearest2Points {
    unsigned char ct = 2;
public:
    struct {
        unsigned idx;
        T dist;
    } i0, i1;

    void push(unsigned i, T d) {
        if (ct == 0) {
            if (d >= i1.dist) return;
            if (d >= i0.dist)
                i1 = {i, d};
            else {
                i1 = i0;
                i0 = {i, d};
            }
        } else {
            if (ct == 1) {
                if (d >= i0.dist)
                    i1 = {i, d};
                else {
                    i1 = i0;
                    i0 = {i, d};
                }
            } else i0 = {i, d};
            --ct;
        }
    }
};

template<typename T, typename SOMData>
class SOMBase {
protected:
    SOMData network;

    inline void Connect(const unsigned i, const unsigned j) {
        const unsigned ij = i * network.units + j, ji = j * network.units + i;
        network.connections[ij] = 1;
        network.conn_ages[ij] = 0;
        if (i != j) {
            network.connections[ji] = 1;
            network.conn_ages[ji] = 0;
        }
    }

    inline void Disconnect(const unsigned i, const unsigned j) {
        const unsigned ij = i * network.units + j, ji = j * network.units + i;
        network.connections[ij] = 0;
        network.conn_ages[ij] = 0;
        if (i != j) {
            network.connections[ji] = 0;
            network.conn_ages[ji] = 0;
        }
    }

    void ErrorDecay(T decay) {
        for (unsigned j = 0; j < network.units; ++j)
            network.errors[j] *= decay;
    }

    template<typename P>
    void Tighten(const P &o, const unsigned winner, const T e_nearest, const T e_neibor) {
        auto w0 = network.weights + winner * network.dims;
        const auto conn = network.connections + winner * network.units;
        std::vector <T> difference;
        difference.reserve(network.dims);
        for (unsigned j = 0; j < network.dims; ++j) {
            T dx = (o[j] - w0[j]);
            difference.push_back(dx);
            w0[j] += dx * e_nearest;
        }
        for (unsigned k = 0; k < network.units; ++k) {
            if (conn[k]) {
                auto wk = network.weights + k * network.dims;
                for (unsigned j = 0; j < network.dims; ++j)
                    wk[j] += difference[j] * e_neibor;
            }
        }
    }

    template<typename P>
    T Distance2(const P &o, unsigned unit) {
        const T *w = network.weights + unit * network.dims;
        T dist = 0;
        for (unsigned j = 0; j < network.dims; ++j) {
            T diff = o[j] - w[j];
            dist += diff * diff;
        }
        return dist;
    }

    template<typename P>
    void RankUnits(const P &o, Nearest2Points<T> &q_rank) {
        for (unsigned i = 0; i < network.units; ++i) {
            T dist = Distance2(o, i);
            q_rank.push(i, dist);
        }
    }

    void UpdateError(const Nearest2Points<T> &q_rank) {
        network.errors[q_rank.i0.idx] += q_rank.i0.dist;
    }

public:
    SOMBase(unsigned units, unsigned dims) : network(units, dims) {
        network.Init();
    }

    void UpdateClusters(unsigned outlier_tolerance = 1) {

    }
};

template<typename T>
struct DistFunc : thrust::binary_function<T, T, T> {
    __device__ T

    operator()(const T &a, const T &b) const {
        T d = a - b;
        return d * d;
    }
};

__global__ void prune_conn(bool *C, unsigned *T, const unsigned n, const unsigned max_age) {
    const unsigned i = blockIdx.y * blockDim.y + threadIdx.y;
    const unsigned j = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n && j < n && i <= j) {
        const unsigned ij = i * n + j;
        if (T[ij] > max_age) {
            C[ij] = 0;
            T[ij] = 0;
            if (i != j) {
                const unsigned ji = j * n + i;
                C[ji] = 0;
                T[ji] = 0;
            }
        }
    }
//    for (unsigned i = 0; i < network.units; ++i)
//        for (unsigned j = i; j < network.units; ++j)
//            if (network.conn_ages[i * network.units + j] > params.max_age)
//                Disconnect(i, j);
//    for (unsigned i = 0; i < network.units; ++i) {
//        const auto conn = network.connections.data() + i * network.units;
//        for (unsigned j = 0; j < network.units; ++j)
//            if (conn[j] != 0) {
//                nz.push_back(i);
//                break;
//            }
//    }
}

template<typename T, typename SOMData>
class GrowingNeuralGas : public SOMBase<T, SOMData> {
public:
    struct FitParams {
        T e_nearest;
        T e_neibor;
//		T e_epsilon;
//		T e_lambda;
        unsigned max_age;
        unsigned step_new_unit;
        T err_decay_local;
        T err_decay_global;
        unsigned n_pass;
    } params;

    GrowingNeuralGas(size_t units, size_t dims, const FitParams &params) :
            SOMBase(units, dims), params(params) {

    }

    template<typename P>
    void Fit(std::vector <P> &input_data) {
//        if (cudaSuccess != cudaMalloc(&d_connections, sizeof(unsigned char) * network.connections.size()))
//            std::cerr << "cudaMalloc error d_connections" << std::endl;
//        if (cudaSuccess != cudaMalloc(&d_conn_ages, sizeof(unsigned) * network.conn_ages.size()))
//            std::cerr << "cudaMalloc error d_conn_ages" << std::endl;
//        unsigned sequence = 0;
        for (unsigned p = 0; p < params.n_pass; ++p) {
            std::cout << "    Pass #" << (p + 1) << std::endl;
            std::random_shuffle(input_data.begin(), input_data.end());
            unsigned steps = 0;
            thrust::device_vector <T> unit_dist(network.dims);
            thrust::device_vector <T> prototype_ranking(network.units);
            for (auto const &o: input_data) {
                steps += 1;
//                if (network.dims > 7) UpdateGPU(o, unit_dist, prototype_ranking);
//                else UpdateCPU(o);
                UpdateCPU(o, steps);
            }
        }
//        cudaFree(d_connections);
//        cudaFree(d_conn_ages);
    }

protected:
    template<typename P>
    void UpdateGPU(const P &o, thrust::device_vector <T> &unit_dist, thrust::device_vector <T> &prototype_ranking) {
        thrust::device_vector <T> observation(o);
        // find the nearest and the second nearest unit
        for (int i = 0; i < network.units; ++i) {
            thrust::transform(
                    observation.begin(),
                    observation.end(),
                    network.weights.begin() + i * network.dims,
                    unit_dist.begin(),
                    DistFunc<T>());
            prototype_ranking[i] = std::sqrt(
                    thrust::reduce(unit_dist.begin(), unit_dist.end(), 0, thrust::plus<T>()));
        }
//                ranked_indices = np.argsort(np.linalg.norm(W-observation[np.newaxis, ...], ord = 2, axis = 1))
//                i0 = ranked_indices[0]
//                i1 = ranked_indices[1]
    }


    template<typename P>
    void UpdateCPU(const P &o, const unsigned steps) { // when the latency outweighs bandwidth issue

        // 2. find the nearest and the second nearest unit
        Nearest2Points<T> q_rank;
        auto start = std::chrono::steady_clock::now();
        RankUnits(o, q_rank);
        const unsigned i0 = q_rank.i0.idx, i1 = q_rank.i1.idx;
        auto end = std::chrono::steady_clock::now();
        profiler[2] += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        // 3. increment the age of all edges emanating from i0
        PROFILER_BEGIN
        UpdateEdgeAges(i0);
        PROFILER_END(3)

        // 4. add the squared distance between the observation and i0 in feature space
        UpdateError(q_rank);

        // 5. move i0 and its direct topological neighbors towards the observation
        PROFILER_BEGIN
        Tighten(o, i0, params.e_nearest, params.e_neibor);
        PROFILER_END(5)

        // 6. if i0 and i1 are connected by an edge, set the age of this edge to zero
        //    if such an edge doesn't exist, create it
        Connect(i0, i1);

        // 7. remove edges with an age larger than MAX_AGE
        //    if this results in units having no emanating edges, remove them as well
        PROFILER_BEGIN
        Prune();
        PROFILER_END(7)

        // 8. if the number of steps so far is an integer multiple of parameter STEP_NEW_UNIT, insert a new unit
        PROFILER_BEGIN
        if (steps % params.step_new_unit == 0)
            Grow();
        PROFILER_END(8)

        // 9. decrease all error variables by multiplying them with a constant
        PROFILER_BEGIN
        ErrorDecay(params.err_decay_global);
        PROFILER_END(9)
    }

    void Grow() {// sequence += 1
        // 8.a determine the unit q with the maximum accumulated error
        unsigned q = ArgmaxError();
        // 8.b insert a new unit r halfway between q and its neighbor f with the largest error variable
        const auto neighbors = GetNeighbors(q);
        if (neighbors.size() > 0) {
            unsigned f = ArgmaxError(neighbors);
            SOMNetworkHost<T> new_som(network.units + 1, network.dims);
            new_som.From(network);
            network = std::move(new_som);
            const auto wq = network.weights + q * network.dims;
            const auto wf = network.weights + f * network.dims;
            const unsigned nu = new_som.units - 1;
            auto w_new = network.weights + nu * network.dims;
            for (unsigned j = 0; j < network.dims; ++j)
                w_new[j] = (wq[j] + wf[j]) * 0.5;
            // 8.c insert edges connecting the new unit r with q and f
            //     remove the original edge between q and f
            Connect(q, nu);
            Connect(f, nu);
            Disconnect(q, f);
            // 8.d decrease the error variables of q and f by multiplying them with a
            //     initialize the error variable of r with the new value of the error variable of q
            network.errors[q] *= params.err_decay_local;
            network.errors[f] *= params.err_decay_local;
            network.errors[nu] = network.errors[q];
        }
    }


    void Prune() {
        std::vector<unsigned> nz;
        nz.reserve(network.units);
        const unsigned units2 = network.units*network.units;
//        cudaMemcpy(network.d_connections, network.connections, sizeof(bool)*units2, cudaMemcpyHostToDevice);
//        cudaError_t s = cudaMemcpy(d_conn_ages, network.conn_ages.data(), sizeof(unsigned)*network.conn_ages.size(), cudaMemcpyHostToDevice);
//        unsigned *dd;
//        cudaMalloc(&dd, sizeof(unsigned) * network.conn_ages.size());
//        cudaError_t s = cudaMemcpy(dd, network.conn_ages.data(), sizeof(unsigned) * network.conn_ages.size(),
//                                   cudaMemcpyHostToDevice);
//        cudaFree(dd);
//        if (cudaSuccess != s) {
//            std::cerr << "cudaMemcpy error " << cudaGetErrorString(s) << std::endl;
//            std::cerr << d_conn_ages << " " << network.conn_ages.data() << " "
//                      << sizeof(unsigned) * network.conn_ages.size() << " " << cudaMemcpyHostToDevice << std::endl;
//        }

        for (unsigned i = 0; i < network.units; ++i)
            for (unsigned j = i; j < network.units; ++j)
                if (network.conn_ages[i * network.units + j] > params.max_age)
                    Disconnect(i, j);
        for (unsigned i = 0; i < network.units; ++i) {
            const auto conn = network.connections + i * network.units;
            for (unsigned j = 0; j < network.units; ++j)
                if (conn[j] != 0) {
                    nz.push_back(i);
                    break;
                }
        }
        if (nz.size() != network.units && nz.size() >= 2) {
//            std::cerr << "GNG Downscaling " << network.units << " -> " << nz.size() << std::endl;
            SOMNetworkHost<T> new_som(nz.size(), network.dims);
            new_som.FromNonZero(network, nz);
            network = std::move(new_som);
        }
    }

    void UpdateEdgeAges(const unsigned i0) {
        const auto conn = network.connections + i0 * network.units;
        auto ages = network.conn_ages + i0 * network.units;
        auto ages_ = network.conn_ages + i0;
        for (unsigned j = 0; j < network.units; ++j) {
            if (conn[j])
                ages[j] += 1;
            *ages_ = ages[j];
            ages_ += network.units;
        }
    }

    std::vector<unsigned> GetNeighbors(unsigned q) {
        std::vector<unsigned> ret;
        ret.reserve(network.units);
        const auto neighbors = network.connections + q * network.units;
        for (unsigned j = 0; j < network.units; ++j)
            if (neighbors[j])
                ret.push_back(j);
        return ret;
    }

    bool error_compare(unsigned i, unsigned j) {
        return network.errors[i] < network.errors[j];
    }

    unsigned ArgmaxError() {
        unsigned m = 0;
        for (int j = 0; j < network.units; ++j)
            if (error_compare(m, j))
                m = j;
        return m;
    }

    unsigned ArgmaxError(const std::vector<unsigned> &subset) {
        unsigned m = 0;
        for (auto j:subset)
            if (error_compare(m, j))
                m = j;
        return m;
    }
};

//struct Point2D {
//    float x, y;
//};
typedef std::vector<float> DataPoint;

extern std::vector <DataPoint> sample_data;

int main(int, char **) {
    GrowingNeuralGas<float, SOMNetworkHost<float>> gng(16, 2, {
            .1, .006,
            32, 500,
            .618, .995,
            8
    });

    profiler[2] = 0;
    profiler[3] = 0;
    profiler[5] = 0;
    profiler[7] = 0;
    profiler[8] = 0;
    profiler[9] = 0;

    gng.Fit(sample_data);

    for (auto i : profiler)
        std::cout << "Profiler " << i.first << " = " << i.second
                  << std::endl;
    return 0;
}


static std::vector <DataPoint> sample_data = {
        {-1.1595977041421617,   1.087081939146372},
        {-0.7927129055589076,   0.4997298359038399},
        {0.6967998577791031,    -0.5517540355982579},
        {-0.49481360466712815,  -0.6909370494469194},
        {0.878962461212664,     0.2424076443199786},
        {1.1066623378473153,    -1.237098713601287},
        {0.4320575776137298,    -0.7695604440062014},
        {-0.060418764122213704, -1.746362003882774},
        {0.5154491056371528,    -0.6117917760798748},
        {0.6353491414659783,    -1.6110512976688938},
        {-0.4115391675576673,   0.7258447003706106},
        {0.0581398784171241,    -1.7186423073978847},
        {-0.07729672200016668,  0.7496790270612913},
        {0.33982325327490576,   -0.9017395079221792},
        {0.10016864763037801,   -0.9422392644919565},
        {0.4640734414672161,    -1.6500403381607884},
        {0.7613396504485059,    -0.14033101904701858},
        {0.7816038972867297,    -1.6450891534793137},
        {0.278915735941884,     0.8135663411908913},
        {-0.46388284839591276,  -1.6100046906904861},
        {0.6548407236543732,    -0.5541427513006199},
        {-1.8862381193518956,   0.2571518295593021},
        {0.15498311380923027,   -1.6377730854315344},
        {-0.9632876257997224,   0.38918176206973515},
        {-1.0465937352515442,   -0.04374042475757455},
        {0.5954582428990252,    -0.4111553751926828},
        {-1.6757351698028264,   0.17238519525216905},
        {1.594458533320021,     -0.5859788254040638},
        {-0.09572136077861043,  0.7142055179943916},
        {-1.6013580483586245,   -0.7628819484315554},
        {-0.7301839505855002,   -0.26957480144807217},
        {1.5282960696931773,    1.2375074622208875},
        {1.5875300937325174,    1.037366432855278},
        {0.5096679194467275,    0.7418299832104212},
        {0.3916256247599675,    0.805989266105569},
        {-0.4989958884707839,   0.7210902809805313},
        {-0.5943054634531291,   0.6000302976790345},
        {-0.2954777185974535,   -0.8836022590913191},
        {-1.1075760205749239,   -1.4190624515325607},
        {-0.2631862862347671,   -0.8976660939158303},
        {0.8166496192310589,    -0.5005088186629979},
        {0.7249144630858934,    0.29407045751352767},
        {0.6073419148362635,    -0.4954528184099901},
        {0.4774830377376179,    0.730489044634026},
        {0.21278799891074424,   0.8920267049852911},
        {-0.3415398783077493,   -1.800602920717483},
        {1.1087452060651555,    1.450013666641842},
        {0.975260620925249,     0.1205080489250581},
        {-0.6486835340310323,   -0.6556067311004278},
        {1.9340323788172533,    0.022815626896339904},
        {0.7174801545647814,    -0.7348367496099576},
        {1.7431214572191687,    -0.38225507248000856},
        {-0.08293041194340789,  -1.8190188872530568},
        {0.26302723037996567,   0.9320332558536882},
        {0.5668382649546079,    0.8162255399282364},
        {-1.7965188601639124,   0.10121340772511929},
        {-0.9159645539156622,   1.4641225237513726},
        {1.4523583526791608,    0.6143321308345471},
        {0.1774995618025395,    0.9735079926200957},
        {1.7140289166736153,    0.5488910377517023},
        {-0.03069502011663513,  0.8853413390192661},
        {1.6647257442873584,    -0.4692355683043396},
        {1.727774917577526,     -0.509965062608725},
        {-1.5846245538014596,   0.7771779959921397},
        {-0.7995936203370589,   0.11672469503200235},
        {0.026411450748474095,  -0.9067474712075523},
        {0.5812422700836705,    -0.7535771821628785},
        {1.1406602820015297,    -1.2953043463690441},
        {-1.116633770596476,    -0.055092096324607674},
        {-0.3710695124767086,   -0.7114015872110949},
        {-1.57698097233017,     -0.914362767626435},
        {0.07803336053614421,   1.6381591599536414},
        {-0.7125945244421136,   -1.4413745963755475},
        {-0.4173110551771105,   -1.6303706120419623},
        {0.0179819467434202,    -0.8941054990770456},
        {1.827832846586753,     0.41717726848050424},
        {0.5260503962777018,    0.8898183324472708},
        {0.8731563764537297,    -0.6292471389371499},
        {-0.10706132473750006,  -1.8471810286339303},
        {-1.0164408550698303,   0.22476554451368919},
        {1.76824376884665,      0.006311683981212792},
        {0.6643295473019736,    -0.7516642743922964},
        {0.8940308604586323,    1.3259098717388789},
        {-0.2793970300220301,   0.8481984689261574},
        {0.8927923012712267,    0.5256786485471894},
        {-0.009077981749875645, 0.8509442571762937},
        {-0.9847978587458153,   1.384208713663043},
        {0.8831367000458967,    -1.6088934315401042},
        {-1.5578914322153916,   0.8401265277105401},
        {1.352757845790104,     -0.9873573132799964},
        {-0.7769391972407063,   -0.5880231260653053},
        {0.8445537016707624,    -0.21308685299088173},
        {-0.9010457978981401,   -0.5673558291105949},
        {-0.9220268786348156,   0.1157656740199992},
        {-0.7616784554270607,   -0.5886467000972103},
        {1.3542323262491716,    -1.2042744230054105},
        {-0.44412344087676825,  -0.7592769411314685},
        {-1.79474001430875,     0.23972363865210647},
        {0.7593049871230968,    0.3940927231794183},
        {1.6965653945689478,    -0.05773105544124134},
        {-0.7318863948319734,   1.7284115312835666},
        {0.3608719035561789,    -0.9044734774373199},
        {-1.898540787508551,    0.15527910089761354},
        {-0.45905681455283265,  1.6952567489347097},
        {-0.22917752677275907,  1.0603565202092733},
        {-0.17693422945906961,  -1.7757294443075333},
        {-0.5365277572596296,   -1.793523012232967},
        {-0.8523773185150444,   -0.17091230084100523},
        {-0.1063388072007714,   -0.8166431812721505},
        {0.9065810336437149,    0.029823940834875114},
        {-0.8715915908142615,   -0.6371411715314798},
        {1.0012872446783403,    0.4009475097851527},
        {0.7418450401598804,    -1.6924991328132217},
        {1.113345697173566,     -0.984291107360221},
        {-1.246394694658605,    1.2537500491139735},
        {-1.7381004246918499,   0.40145993353710074},
        {0.8206405650216393,    -1.4898257073547132},
        {-0.8910355321272773,   1.5180632786382369},
        {1.785171597519745,     0.3247070068079972},
        {-0.4404652823666505,   0.7272633296487656},
        {0.283053016752178,     0.7331084121316186},
        {0.9329608080465512,    1.4081090756378267},
        {0.7613082164809417,    -1.694283977990018},
        {-1.6530067918707974,   -0.10394901943548375},
        {-0.22655129116050013,  -0.9107241774275174},
        {1.822897335685237,     -0.2137706503824387},
        {-1.263797861010075,    1.3608763193661004},
        {-1.2744553974641164,   1.2195838052092358},
        {-1.1379416538504743,   -0.1718529626321699},
        {0.2549798714278495,    -0.8879071058226722},
        {0.6058803503529014,    -1.592491015872076},
        {0.9482753195341321,    0.06354528446939549},
        {-0.04536546526857198,  1.0059348311301741},
        {-1.2614626395216233,   1.2389454302419058},
        {-0.5004171843007061,   1.5876118781106436},
        {0.5132408525874999,    0.7907672062131288},
        {1.30488558271011,      1.0904468218860723},
        {1.7514604032538403,    0.8142975175639743},
        {-1.6853627335108559,   0.39364292522766875},
        {0.2133412352681049,    1.0042314302443998},
        {0.39857377729648996,   1.7334670944892532},
        {-0.08697253172371933,  0.9183214726581381},
        {1.8492608208774761,    0.3041169686310782},
        {-0.6644321020243152,   -0.31071492680651347},
        {-0.8490132134262476,   -0.3609762249021624},
        {1.7983711473164523,    0.16207043189938025},
        {0.8039805072049995,    0.37448647620470277},
        {-1.5085033011187647,   0.5908340721322015},
        {-0.2119006132748432,   0.903317107001432},
        {1.7003138350916744,    -0.4741987263535757},
        {0.756389602849865,     0.5359199403996314},
        {0.33635598008295037,   -0.6855817280072073},
        {0.8783531643008432,    -0.1755930106965527},
        {1.7191423998075066,    -0.3398476398286703},
        {1.149030098260155,     -1.4809052831486174},
        {-0.5235553659849581,   0.6914863365796869},
        {-1.830659615618929,    -0.2097223896249223},
        {-0.060928906546319675, -0.7357627401451107},
        {-1.6819033427334824,   -0.020278729592618457},
        {-1.5425237497603792,   0.9376934118910483},
        {1.6666404423521688,    0.21339242690871035},
        {1.0799509758543127,    -1.359839744830456},
        {-0.8427711231964228,   0.1229323584371641},
        {-0.8267339244610599,   -0.2372212075522128},
        {-1.3681492165119327,   -1.3894819990089382},
        {-0.4250703811162352,   0.741127684527491},
        {1.1334376985699308,    1.2481248525939506},
        {-0.03456058460979351,  0.801769874013935},
        {1.0445947387582764,    -1.6743977956407876},
        {-1.263761403256388,    -1.1662392795658176},
        {-1.2389867843153466,   1.2094596827108062},
        {-1.0554899062700898,   -0.06438168463663446},
        {-1.4269230752591344,   1.2027349775507141},
        {-1.8006148196713438,   -0.5272375843509297},
        {-1.7410017940972087,   0.7899370706867741},
        {0.7941926006618346,    -0.250330132566235},
        {0.8569458379097677,    0.22707802651547979},
        {0.34032292142061893,   -0.7926471664295829},
        {-0.7317277828115925,   0.3877271994327971},
        {0.5519299124853149,    -0.4870989737324003},
        {-0.5097136290648829,   -0.7785666534476889},
        {-0.13540408582321944,  -0.9822358586893828},
        {-0.11984964708939654,  1.790332674158321},
        {-0.8321381874025614,   0.16400303421260665},
        {0.9962142323750116,    -0.42157515141526447},
        {-0.35985623444880854,  0.8539289897749534},
        {0.5583860349381903,    -0.6525748580344211},
        {0.6874203654930394,    0.5848878912406361},
        {-1.6312118127463702,   -0.23390078538268755},
        {0.65005651500353,      1.927084776513667},
        {0.961080723081009,     -0.19900258762291168},
        {1.7171140059871215,    -0.6272062990342848},
        {1.6490429006112812,    0.8741100469923201},
        {-0.761560081905156,    -1.6155177594675638},
        {-1.654368799978529,    0.8074185813206758},
        {-0.24095642312605547,  0.8864311589747604},
        {0.14070336022670502,   -1.6517897935183918},
        {-0.867859329255833,    0.3409324621397597},
        {1.5627988193663132,    -1.0394410795256213},
        {0.1887381380657271,    -1.8172546712293514},
        {-0.7812228502407985,   -0.30136039024944666},
        {1.5483919268895199,    0.9729959559271865},
        {0.773303109798355,     1.4879894665327895},
        {1.5045146382334948,    -0.9026459724662389},
        {1.4720826655424588,    1.211976139038168},
        {-0.6903416037581472,   -0.5990351490702565},
        {-0.8431168441980355,   0.26213086003500885},
        {-1.7368087474563072,   -0.49780821303288536},
        {-0.5492071088711155,   -0.739331390182043},
        {-0.3202328001409015,   1.7630616513086084},
        {1.7156598742793547,    0.36818417288981864},
        {-0.8055140774674869,   -0.43811919354272194},
        {1.3958499957247459,    -1.0767643168081469},
        {0.31422900242202756,   1.605886336230846},
        {-0.09568266377028714,  -0.7621327730296767},
        {0.11072101413033371,   -0.929899269562031},
        {-1.6858857389849247,   0.691949075601587},
        {0.5272605625819781,    -1.7335329707170566},
        {-0.9267201800438721,   -0.0741821323812104},
        {1.0512027370041896,    -0.03990424278571026},
        {-0.15044466824142763,  -0.921362575091568},
        {0.8186382426788111,    0.4169137537266277},
        {-1.636710220408716,    0.6863151646591011},
        {-0.7167555656196342,   -1.6585834325350166},
        {0.8162876798821476,    -0.40366550186756045},
        {-0.023771527066724047, -0.8358227604165098},
        {0.9940591719089079,    1.5287198886030295},
        {-0.8741340313001709,   -0.3695134014112397},
        {-1.0948281320510826,   -0.19175646570968094},
        {1.8784085420080923,    0.3130360865038325},
        {-0.7807497758529676,   -0.23167148070135304},
        {1.7273298732826696,    0.6022789469837309},
        {-0.3843061334352944,   -0.7853883501196027},
        {0.36378411693266016,   -0.887931734597272},
        {0.9253819455122018,    -0.19678336820721742},
        {1.2204573487071273,    1.159273122907661},
        {0.7279305572476443,    -0.5410502906195869},
        {1.7754597139799366,    0.6659581787495991},
        {0.12384048096693742,   0.9118186424500493},
        {0.1435153348016388,    1.8045838889469474},
        {0.017159052076964894,  -0.9850494159770677},
        {-0.8874753899716177,   -1.4131495445573006},
        {-0.7946214743385266,   0.36585506142577434},
        {0.6565822517602201,    0.6233757616534213},
        {-0.24634735037230782,  1.7945592743616319},
        {1.5404032812530926,    -0.9595137505565627},
        {-0.8052395603009924,   1.522348279946595},
        {1.2125232083208686,    1.0923445910629},
        {-0.6622923439729841,   -0.456808491554476},
        {-1.5649123037470134,   0.9383117600164885},
        {0.061951158692790644,  -1.018582770948332},
        {0.5050721636227818,    0.6916344226134271},
        {-0.2708072008702698,   -0.8783998929534844},
        {0.7148489806975077,    0.26908957190872784},
        {1.5492953538286822,    -0.6883952419614231},
        {-0.5683291617994137,   0.8178618533101306},
        {-1.8016165004015738,   -0.696103245896733},
        {1.3728585083113056,    -1.075973342571044},
        {-0.8946796654264211,   0.12916856331143978},
        {0.8066631867268143,    1.5997948196662792},
        {0.8923353658285056,    0.2616646465278973},
        {-0.7130871449165178,   0.5037124406225161},
        {0.786346465374252,     -0.6054106785672894},
        {0.8816884081384189,    0.18677760622802125},
        {0.7757488948220035,    -0.641284010440335},
        {0.3022951955633993,    -1.6060596737317363},
        {-1.729479946072897,    0.7441870739101818},
        {0.47414390817610685,   0.8394251837881528},
        {0.44469160291381804,   0.8040349857540856},
        {-0.7382103132507933,   1.5658239560221963},
        {1.2213735668210393,    1.5185346998521865},
        {0.05417023112990263,   -0.7430724046069653},
        {1.018186400622011,     -0.08366877597077516},
        {-1.1861353361222413,   -1.2342123547773904},
        {-0.40002721910705136,  0.9371946053925254},
        {-0.8532947834773799,   0.3192548794411748},
        {-1.9527117429865264,   -0.3446992777117515},
        {-0.3258976335015472,   -0.6715979661481867},
        {0.636579975527415,     -0.5254420468803557},
        {0.740134167969948,     -0.7441631221847476},
        {1.6993041668096824,    0.7463862300139892},
        {0.031668879575472327,  0.9084401193030005},
        {0.03561783020471357,   -1.0055309198765392},
        {0.8440677804135746,    0.19945166007004742},
        {-1.2043359177721857,   -1.4077795915291562},
        {0.48262885987217374,   0.8432685050599874},
        {1.8428636929930653,    -0.2673576721550216},
        {0.5825155253085271,    0.7917157137807272},
        {-1.8084479649623033,   0.27219389664693955},
        {-1.7859406630095023,   -0.12118187083138379},
        {1.2759168304637232,    -1.3429175124037245},
        {-1.7221064171686777,   -0.46527661081784133},
        {1.6155158251184507,    -0.4350047657739305},
        {-1.120531834562191,    1.177204730949611},
        {0.9344071027260785,    -0.013260504078475607},
        {0.8074850094888306,    0.14189298179522428},
        {0.5428550404239733,    -0.6230126868413455},
        {0.39727592231125525,   0.9452116624687779},
        {0.2575848285067723,    -1.0140140316286792},
        {1.2817896398854034,    1.0635384905213798},
        {-0.10980468725065112,  0.7994696303167621},
        {0.8052353874741899,    1.7488354576626424},
        {0.26316272949462216,   1.8741716780941235},
        {1.0875568722512274,    1.2903060821222323},
        {1.2518188476026255,    -1.206053644937135},
        {-0.08296716322352053,  -0.7315384949055502},
        {0.4033300816514587,    -0.8379004819863245},
        {-0.3582415975685946,   0.9094050744787981},
        {-1.581685318045309,    0.7917248098516646},
        {-0.6949242847611499,   -1.6237628492033005},
        {-0.9400933471646093,   0.10510352647672429},
        {-0.11370015681062222,  -1.77185252202919},
        {1.456434028404356,     0.9142071282809346},
        {0.617064551831378,     -0.6922086020713256},
        {0.967110841809733,     0.38606187536068354},
        {0.42665297085368087,   0.7163165437165541},
        {0.5117494909935929,    0.7368476717488464},
        {-0.5663120642565307,   0.9219557184542708},
        {-0.8463334777480344,   -0.06865831756140296},
        {-0.44008320402222467,  0.7603654556829329},
        {1.3733861470212756,    1.1850959506432042},
        {0.8830257239381931,    -0.1519801807588165},
        {0.8652803931630555,    -0.3188705804209456},
        {1.7956698981846173,    0.08075577941327099},
        {1.830108563973892,     -0.1869959573906304},
        {1.882938125221608,     0.04770908341220429},
        {1.1443725778888405,    -1.4116721814401376},
        {-0.732728258584571,    1.6067441139916083},
        {0.9192736912072622,    0.14947226802158417},
        {-1.015532092081241,    -0.3202799457853022},
        {-0.5772814969381522,   -1.5764815833231165},
        {1.0136325879325108,    0.23920997496626376},
        {-0.10586146450562481,  0.8713166237278247},
        {-1.1399197751484835,   1.5149365738985974},
        {-1.7635863081124181,   -0.6604272203527257},
        {1.7850541336823151,    0.5978937153976943},
        {-0.8892925683851146,   -0.23944025153849913},
        {-0.37961628303767203,  -0.7897767623720188},
        {1.0636151612613227,    0.24057343069437642},
        {-1.4133760892754745,   1.0961532428007443},
        {-0.44752807137713196,  0.722061130878757},
        {-0.9454513001490992,   -0.04151174245029791},
        {0.2673414635473364,    1.8759776760463651},
        {-1.148272692886438,    1.3765371743562465},
        {-0.00484255816267827,  0.8685401666750746},
        {1.4989284299467835,    -1.065237018655329},
        {1.7448586390656653,    0.3957764602192816},
        {1.0680905519791775,    -1.3877710634323606},
        {-0.8899330680950855,   -0.3028431251364847},
        {0.32014905590081816,   0.9063187986976059},
        {-0.9325214263953918,   -0.5346367171665743},
        {0.9246155063937691,    0.047806862801129404},
        {1.4598278400696258,    -1.1090257669910613},
        {-0.3116549195781539,   -0.8171942868028208},
        {-0.4899036597512716,   -0.8032532685401265},
        {-0.2647693084539563,   -0.7349251684244773},
        {-0.3340801849383771,   0.6807701118418015},
        {1.6666397743328416,    -0.9021299996183013},
        {0.5817898000078537,    0.5410329401072365},
        {-1.8438382918880003,   0.06905725956500014},
        {0.24776540452203147,   -1.0131869399824454},
        {-1.2244453540772935,   1.350165052584559},
        {1.6227778656164167,    -0.6453862362034397},
        {0.4414911932737625,    1.7360334155010906},
        {-0.9103139064802149,   1.4944649804128203},
        {-0.3945439867267204,   -0.9541796655074263},
        {-1.7853884047280204,   -0.1363745216907093},
        {-1.4216268989205896,   -1.111467520668326},
        {0.8900825130490297,    -1.7416663430528565},
        {-0.40175405830591643,  -0.9099218927617343},
        {0.7176804507050707,    -0.5853586438626938},
        {-0.14153306652870343,  1.1041890109981587},
        {-0.2153238967224038,   1.8778683637838143},
        {0.30075489543337564,   0.9661174957279367},
        {0.6912464000935854,    0.6510831114640445},
        {-0.6422446213145486,   -1.6586549031252924},
        {-0.7104972145591593,   -0.26701465380721956},
        {0.6569165735572225,    0.5441663114799986},
        {1.4108258045448696,    0.8077712132019658},
        {0.6336627504264839,    -1.7130258002505112},
        {-0.5655461288756316,   -0.5210080219159458},
        {-0.9365005334672105,   0.140841174136626},
        {0.8336634694917917,    0.13853813949921126},
        {1.7980331805575445,    -0.44987997547999503},
        {-1.8015483632863114,   -0.6506954523673634},
        {-0.654244119027903,    0.0009999808634074611},
        {-1.1272110290881656,   1.3090556927096153},
        {0.7131924995038289,    0.6728858329515072},
        {1.6907614631730652,    0.8213357987323312},
        {1.1908594419034153,    1.0957185160693574},
        {0.5014709496729239,    0.7569220797910056},
        {-1.7985195485477878,   0.3133345024408139},
        {0.7623742472713049,    0.5510188256893874},
        {1.5646210672652785,    -0.7291757870988892},
        {-1.6036570695784573,   0.734986287828511},
        {-0.2915848697352897,   1.8080008024659073},
        {-0.539891402394998,    0.6278775763293422},
        {-0.7189970553892805,   0.407655308176638},
        {0.6478097332701701,    1.7094851588433095},
        {-0.09113955858466022,  -0.893388461665499},
        {-0.950524742681703,    0.022120590722560254},
        {-1.700625395451814,    -0.770974914495948},
        {0.726792219315455,     1.4658072347400446},
        {-1.6324739850298124,   0.6536135685861908},
        {1.2000923046300662,    1.3965663438249312},
        {-0.5728373065192252,   0.5864723391140626},
        {-0.8222077545167638,   -0.686793875300425},
        {-0.7703742145172261,   0.5317542728406899},
        {0.7906599444415667,    -0.09032191696219499},
        {-0.7512802721194297,   1.610350810371066},
        {-0.9098207655790013,   -0.41136716785043975},
        {-0.781908771894423,    1.7371512327854728},
        {0.4700740856018044,    -1.7107365109346768},
        {-1.6750324680244768,   0.5140486631065779},
        {0.8278593805920115,    -1.5234072078892849},
        {-0.932296104768762,    1.5354723717542886},
        {0.7161675158110006,    -1.544094153883269},
        {-0.5582850358963325,   -0.8432221586611904},
        {1.8298794861017935,    -0.6372547704796091},
        {1.6091056762752185,    -0.9037629819337554},
        {-1.438464528632073,    1.0704431585004193},
        {-0.6057626468559443,   0.640243013444162},
        {0.46458852927221594,   1.6833600645870137},
        {-0.5998371898680273,   0.6719430697890497},
        {-0.5017890223653819,   -0.8084243807925066},
        {1.5389535667680625,    -0.7673725802017536},
        {-1.70235623041023,     0.3799414383272283},
        {-1.5958241227602556,   0.08002488091843114},
        {0.6389640963654377,    -0.5086225632463107},
        {-0.9299995412514152,   1.520973170366836},
        {0.26826522289175203,   0.8209077768833376},
        {1.2356510708784882,    1.2253922326362012},
        {0.604143404507986,     -0.5493470060164607},
        {-1.0206000666975674,   -1.5393728416839418},
        {0.8025986156679108,    0.4534335971407797},
        {-0.7829388880791474,   -0.0012083178851033118},
        {-1.323267081967874,    1.1237156148252216},
        {-0.21842503153276263,  -0.7472657329244684},
        {-1.652795554532975,    0.5970499927300982},
        {-1.553197699556378,    -0.7417110120355633},
        {-0.8088490108130483,   0.6816227710845829},
        {1.1211292042501053,    -1.3820652012828167},
        {-1.8882596875930047,   -0.20081159223206307},
        {-0.7955884935034419,   0.39894810424123944},
        {0.44610596099207805,   -0.6906365603745185},
        {-0.6134683980098288,   -1.6200142094067858},
        {0.7766949712038983,    1.6166817243910123},
        {-1.4594604759577332,   -0.9072361262379258},
        {-0.28785737285254986,  0.7371701867472545},
        {0.10031056564578275,   -0.8763046157520161},
        {0.5756873854843777,    0.6492493099410671},
        {1.2583076185560707,    1.2066671510979132},
        {1.011898415504634,     -0.06508466348957558},
        {0.34649687643370863,   0.7667763800517197},
        {0.18907342228141671,   -0.8307484706316263},
        {0.3046171591082189,    1.8403749289139297},
        {0.024792402456480057,  0.8827107084965399},
        {-0.005636983871358595, -0.8689995136578373},
        {-0.87194181420239,     -1.5579618194963372},
        {0.8406388731187543,    0.6166051907876104},
        {-0.24996343634430307,  0.9113540236355758},
        {-1.6481897898108284,   -0.46434846620345177},
        {-1.596007546726728,    0.8843688803375575},
        {1.6320474127938482,    -0.07288249687504024},
        {0.22181167175080718,   0.9270621328279867},
        {0.9583225342541909,    -0.40174358488181394},
        {0.04770226535552572,   -0.8956175161560449},
        {-1.381557767926144,    -1.3266887408242558},
        {-1.7531208129361318,   -0.4855460045763107},
        {-1.655906379001026,    -1.2057407566158798},
        {-0.8871181924412248,   0.26703759159113727},
        {0.08275825292416572,   -0.8882870924831754},
        {1.7357113211225366,    0.06947905382989898},
        {0.6722363762236971,    1.636175990753366},
        {0.7174815388996113,    -0.24216057373969566},
        {-0.7064702448018563,   -0.6340351093501893},
        {0.81883821240061,      -0.08291992275131814},
        {0.597007781818264,     1.6866794760898},
        {1.1461426821092897,    1.5192351637645232},
        {-1.7139476469890575,   0.7658804232755184},
        {1.6730649778037685,    0.6019802293022717},
        {-0.8581727583739065,   0.3443360190278716},
        {-1.525016798883276,    -0.9900802247006536},
        {0.9825915858544261,    -0.055488601337631095},
        {0.7686220006520684,    -1.7472380023602523},
        {-1.78432611991979,     0.0806699948180913},
        {1.6248225535179133,    0.9561846984372075},
        {0.21417440982437114,   -1.7265511267506986},
        {-0.2753814039243515,   -0.8309343080948024},
        {0.5061138635392993,    -0.7787279506633232},
        {-0.5110962363491681,   -0.9281241927309335},
        {0.6222212620476092,    -0.4914075523796043},
        {0.3634237913453185,    1.6073102694621255},
        {-1.0115102639205076,   -0.33241704968038205},
        {-0.5712973500364529,   1.7542516817605138},
        {0.8465997996202672,    -0.405005407206924},
        {-1.7132381140775625,   0.663314222305211},
        {-1.5451428478237008,   -0.4019657104461496},
        {-1.8554519914487535,   -0.05077468892512498},
        {-0.10946652421960425,  0.7748126632275251},
        {0.43677337528611493,   1.6657116663756166},
        {-1.5794498022888412,   0.8350922992000311},
        {0.9618797327275734,    0.14395959263656508},
        {-0.804468435316087,    -0.2959822899069015},
        {-0.9169413479175202,   -1.5765791414302006},
        {1.3619306990597069,    -1.3193040637073785},
        {-0.6853117828101428,   -0.6888580460799157},
        {0.06718777607457393,   0.817794444477983},
        {0.6742494966087992,    1.5660628288243754},
        {0.020727006391987577,  -0.9979325490351028},
        {-0.7279831425049806,   -0.17029457547426774},
        {0.6127153391721303,    1.4890541337241971},
        {-0.6888676344159528,   0.7225543330166156},
        {0.25377148045214754,   0.9306123058300112},
        {-0.6618315789639372,   1.604139139247529},
        {-0.8167778433343093,   0.41068506260980725},
        {0.7579405301738656,    0.6285693662468534},
        {1.3535540450115084,    1.0857788377927138},
        {0.8026624830711138,    0.18738714898514244},
        {-0.37900332223856065,  0.6949873127613917},
        {0.71988130328247,      0.3193530932332627},
        {1.7913909536130275,    0.16775062170989782},
        {-1.6837363781598695,   0.20999653100416207},
        {-0.11633917241433542,  -1.8957443891410475},
        {-0.6439097712813463,   1.676040487249567},
        {0.5719232916811383,    -0.8236935517027639},
        {-1.8261836952668526,   -0.08082227129238755},
        {-0.5195939871016118,   0.7195693445653935},
        {-0.5837584129274694,   0.5390008081998939},
        {-0.7422163467210751,   -0.2960811505841449},
        {-1.5723894193170598,   -0.3666228784108356},
        {-0.6761082524307412,   -0.4679393257837363},
        {0.16386820014870354,   -1.740127550834575},
        {0.0042697303948304646, -0.8797908442499567},
        {0.49351822817345103,   0.8070320588812664},
        {1.5330384087606834,    0.728318878384824},
        {-0.2743929191674309,   -0.7021332586830166},
        {-0.8489546377531987,   0.3164289058751078},
        {-0.4958242740382418,   -1.5872490826756152},
        {0.6544605995343067,    0.435814823224479},
        {0.5966275304057158,    0.6771425889126886},
        {-0.7468627097554105,   0.5902524274225989},
        {1.530541754596373,     0.7656778754582367},
        {-0.7122964372878692,   0.33660680127403975},
        {-0.6032248415009539,   0.4420047466790174},
        {1.6830997643590562,    -0.31012030376701993},
        {1.1228169604647917,    -1.3859302964945954},
        {-0.6164795230382144,   -0.5678577278557067},
        {-0.8989932723845365,   0.059213680815079016},
        {-0.7264237062856765,   -0.5325228635638567},
        {0.3866854758645746,    0.6904676205780738},
        {-0.3896139086301447,   1.6538270711334846},
        {0.2034814171011533,    0.85032923646433},
        {1.7006561046701065,    -0.1003006028992301},
        {-1.4354897425779825,   0.9104200761179047},
        {0.08781574388043892,   0.7573417021431498},
        {1.135263178475642,     1.2586008598226555},
        {1.7675544944698436,    -0.39723915412467103},
        {-0.9647668328305777,   -1.538260316718202},
        {0.13276631512633655,   1.6885273836984227},
        {-0.24219726353261065,  -0.8760253549619526},
        {0.3697010780151528,    0.6043047844449173},
        {-1.6235509012371174,   0.6124664730653101},
        {0.775684507657217,     0.07561170852552646},
        {0.15236336287123084,   0.959649069164956},
        {0.36512988747286357,   1.7141498432304652},
        {0.7990616309946055,    -0.4416272364718867},
        {0.5077209234366145,    0.6087473491015772},
        {-1.232717854773058,    1.3765713143453422},
        {1.8489006908593983,    -0.2828203770331093},
        {-1.9307527827232396,   -0.06738886515866241},
        {-0.11355443514780689,  -0.8556174075810015},
        {-1.6667971921276215,   -0.43384393759642026},
        {0.8993682899658222,    -0.3342072432827237},
        {-0.7617793736960287,   -0.6886800836907601},
        {0.6586329633659225,    0.5304075212126752},
        {0.47741357086745917,   -0.8276234153682938},
        {-1.317951090129044,    1.025456474687355},
        {-0.49393875063018333,  -1.6138386083405618},
        {-0.8570230637140859,   -1.3104514085003285},
        {0.5916500285238315,    -0.6950872058600625},
        {0.041158786452956286,  0.8295916138049393},
        {1.0954572633495179,    1.397239407636717},
        {0.5966595099672717,    0.5407122202728613},
        {-0.1579468080456572,   -0.9349590952124878},
        {1.7064420058446133,    0.44476367933072686},
        {-0.5525592087796938,   0.7398877490497768},
        {1.6776596754163138,    -0.28159680488351774},
        {0.757102203697878,     0.3274292435232178},
        {0.6333062411835128,    -0.4847037583698993},
        {1.5991972315727325,    -0.4384911252779285},
        {-0.5871758151540327,   -0.7642549178424082},
        {1.1614993478718911,    1.5160062512214727},
        {0.3727146365134056,    -1.6722750510598916},
        {-0.4039114862996138,   -1.6500898498286578},
        {-0.5473668768433234,   -0.46215062784431893},
        {-1.590871555571803,    0.8821101101608773},
        {-0.024824999102448713, 1.6963809785654518},
        {0.031400637720193775,  -1.7597808542336932},
        {-1.7886399776891777,   -0.3303578788871949},
        {0.5726255015763912,    -0.6546800500125611},
        {-0.37746341706735675,  0.7750086218309881},
        {-0.07289719199913455,  -0.9072307842292279},
        {0.9126638602537624,    -1.4977772984257152},
        {0.5316670746937376,    -0.7154409667546903},
        {0.819651723270285,     -0.43940912337184357},
        {0.5024258016919654,    0.284121381043943},
        {0.9079672860165947,    1.6019004791771954},
        {-0.7811957861100907,   -0.00010732369991324814},
        {-0.013308643529386724, 1.9965728264266198},
        {0.40474440549150753,   -1.7041146989403722},
        {-0.19873133217781594,  -0.84750916937581},
        {-0.7580559681071054,   -0.7033002934787143},
        {1.490937760163538,     0.9513701583374419},
        {0.9021491960849936,    -1.4161637663143873},
        {-0.9572798442414504,   -0.07040222476667317},
        {0.9229896660012155,    1.7145880860510674},
        {0.8750119939637907,    -0.08636609749430778},
        {-0.5877786335146058,   -0.6581510488725978},
        {0.8737481671388244,    0.29597186064507885},
        {0.5512980869926172,    -0.7156442202687942},
        {-0.8294818108367714,   1.8262961926614836},
        {-0.4572687172390521,   -1.816549929486619},
        {0.6416479682309586,    -0.6302201458396428},
        {-0.3525346040045949,   -0.9159275269066941},
        {-0.07425115355666652,  -0.7868837136260909},
        {-1.231865981764981,    -1.5033287302661176},
        {0.8099047204329277,    -0.34297379841989595},
        {1.0369810442097087,    0.04883015946274605},
        {-0.11675755716119213,  0.8000170441612242},
        {-0.8119411141953161,   -1.6260852676298427},
        {0.9854579066859265,    -0.2840755455395714},
        {0.8839330084636574,    0.06779181364897237},
        {-1.406547479244191,    0.9024552325384201},
        {1.6073750718623216,    0.7229479354067814},
        {-0.9433098401064344,   -0.2970432579277712},
        {-0.689656605002197,    -0.609752381180013},
        {0.11712985586474406,   1.860007772132313},
        {-0.7223824217587033,   -0.5623838319379991},
        {0.31172758883836327,   -0.8348548158540999},
        {-0.5940504767022136,   0.73076391985179},
        {1.7617077935213412,    -0.02722168443957409},
        {1.133671964487368,     1.2642945791947156},
        {0.6018244214220801,    -1.6921395372101786},
        {1.2147108746761008,    1.1345487953801408},
        {-0.13161405527298436,  -0.6001175401278525},
        {1.6972425959847057,    0.5147657954127106},
        {0.8515910782991901,    0.3683863174594253},
        {-0.7791063557796524,   -0.23358375673476156},
        {0.7146115334944728,    -1.6187031014109936},
        {0.8394611525543099,    0.015400756042563154},
        {-0.8281311500147301,   0.30972200519996407},
        {-0.6465092989638547,   0.6571438350281836},
        {-0.7119442052043788,   0.5277323957359188},
        {-0.41562447902184607,  -1.6361394910779685},
        {-1.7297512425784756,   -0.18963110748921935},
        {-0.9995273571646249,   1.6647620223004997},
        {-0.25574017898576196,  -1.7320932951769503},
        {-1.403526840150645,    -1.017877910606612},
        {0.33141185389904076,   -1.8669696420641295},
        {1.0138829989949842,    -1.5750497958774947},
        {0.0786791245279245,    1.6899249507490632},
        {-0.6429107784586937,   0.6625311595296572},
        {-0.7542708411715339,   0.33112592854485784},
        {-0.8772787158999386,   1.5028163950911342},
        {0.8730676413927743,    -0.35670402117716227},
        {-1.2095918039472333,   1.3461682378636448},
        {-0.22859894638798334,  1.8423109880483637},
        {-0.6668619701450557,   0.7860604454481355},
        {-0.5105351408309813,   0.5944515176181767},
        {0.7139979720537759,    0.703053009134817},
        {-1.6851089549352667,   -0.5817800990017788},
        {0.6972217786519846,    -0.1023731076727473},
        {-0.730635886904516,    1.6798483171648706},
        {-1.658629262064745,    -0.6792338050123613},
        {1.4560369077337556,    -0.8073356222583863},
        {0.22228821657947484,   -0.7544815936050124},
        {-0.1245932319995016,   -0.9466422025200713},
        {-1.3386278459937957,   1.23636780385},
        {-0.7933253100089873,   0.2554387319882591},
        {-0.29506420066510275,  -1.646921408394017},
        {-0.7408857102325896,   0.04432735122775032},
        {-1.7644402650027264,   -0.47976169087556686},
        {-0.6487000730700195,   -0.6036030133611568},
        {-0.8216089831830072,   1.6989333554864914},
        {0.5952154905817549,    -0.7037643207094746},
        {0.28429436212951065,   0.8305559231268513},
        {-0.31852094441606826,  -1.8134907451044466},
        {1.3822783738218123,    1.0098054295790369},
        {0.8418772008544527,    0.5587166877388976},
        {1.30514981565603,      -1.1865902618864672},
        {-0.9660986650263854,   1.4745813557968102},
        {-0.5968981545487774,   -0.7270423835662414},
        {-0.08254559798937154,  1.6419744908634264},
        {0.18116782219910293,   -0.7701239799622387},
        {1.7587017330926953,    -0.6432232506193407},
        {0.4656892622072199,    0.6655626577700944},
        {-0.7295689832638795,   0.4431430709284939},
        {-0.9007295202351225,   -1.5595334040091378},
        {-0.742107195877021,    -0.6388896119467045},
        {0.9665641408760457,    0.029396703847245114},
        {0.8268238174607733,    0.0774464084110967},
        {1.3830632744627602,    -1.145403593558534},
        {-0.10155555775297445,  1.0008140196650634},
        {1.579243321455635,     -0.5391465046731746},
        {1.82820066403605,      -0.5848379516459074},
        {1.361651173855088,     -0.9105981508773842},
        {-1.0462113435143747,   1.624473572330348},
        {1.0019182560011697,    0.25089343978069995},
        {1.2603024345927802,    -1.3563088067899163},
        {1.15945757221047,      -1.402048261396423},
        {-0.9232548492471605,   -0.14577468334321889},
        {-0.021321080683212756, 1.689525651361188},
        {1.347235144284471,     1.2015556516099777},
        {0.5309145558128031,    -0.7265856706884359},
        {0.3228426935593036,    -0.8003471505964299},
        {1.7309940659088676,    0.4104967428893873},
        {0.6263008985014534,    0.5890395999486424},
        {-0.8219258693666674,   1.5714115115843281},
        {0.09240683421877521,   1.6895408670546574},
        {0.7423102754040611,    0.3780028900522026},
        {1.7305550242052425,    0.7653795336428079},
        {-1.7877967486501063,   0.2615650294225845},
        {0.5605504866366561,    0.5387718687531328},
        {0.4112091648258035,    -1.7972822296086723},
        {-0.8396720609581111,   0.37075171092050463},
        {0.6496115304095003,    -0.687302411900457},
        {-0.49945654117154276,  0.8785538875076369},
        {1.648505805308172,     0.5405177947746452},
        {-1.0812803639252098,   -1.4631974359412014},
        {0.6440225789337475,    0.5021576449132563},
        {-0.7142731848879718,   0.44732700271536946},
        {0.9086183245030319,    -0.03772567415643083},
        {-1.8408534659696731,   -0.12984742111991385},
        {0.12377402243807582,   -0.9768925988180872},
        {0.8324855063722204,    0.18862638546405827},
        {-0.3151640745148416,   0.7070088133430408},
        {0.7179021352988108,    -0.604211961156783},
        {-0.578250512163852,    0.7099448128014121},
        {-0.7914380032475034,   -0.014675638788979489},
        {-0.6998500648026439,   0.1961845905160916},
        {0.3820056252170551,    0.7171248871458703},
        {1.6214652265821072,    -0.9035419081453385},
        {-0.6588630978465663,   -0.7002852891529697},
        {0.9870678401427909,    0.3018804804886304},
        {-0.5698554711184847,   -0.7494243709615561},
        {-1.3960956120533803,   -1.2602981288580763},
        {-0.7254168307444557,   -0.40349276947589463},
        {0.648728074193677,     -1.667233740359504},
        {-0.5048690960161434,   1.6325634267300961},
        {-0.2556885184722474,   -2.012657659944922},
        {-1.2641758394107252,   1.3227971775129737},
        {0.5953611604896665,    -0.7047562183651027},
        {0.6255118104594694,    0.40861910504055293},
        {-1.6542113625834025,   0.837002051914398},
        {0.9184423968466326,    0.5460155339393483},
        {0.17329885916079069,   -0.8709138634460739},
        {-1.3054906812366565,   1.032053394918841},
        {1.0620103060821684,    0.1394418478444155},
        {1.5951210987443194,    -0.8252010367620053},
        {-0.7011228183519599,   1.7325641034468942},
        {-0.5513547090361041,   -1.6724543741315074},
        {0.22860974570999174,   -0.8327639994360905},
        {-1.0393753573245608,   -0.09059609685212833},
        {0.17535528985035512,   1.933634616567457},
        {-1.1668386773992516,   1.321770254437126},
        {-0.31589686060379946,  -0.742325388694152},
        {-0.0993520329090209,   -1.7639531788100251},
        {0.3382470531366538,    0.9381034522023944},
        {0.6353205840188522,    0.650012362468272},
        {0.9561089057175172,    -0.18955658724634591},
        {1.31568337667969,      1.1365780415871336},
        {-1.539041523817357,    -1.1314907287061782},
        {-0.8331195448261514,   1.6267810086572247},
        {-1.8606844371741331,   0.08662484975620117},
        {-0.7924108817949658,   -0.41900621708825325},
        {-1.302855092821777,    1.5666593907992976},
        {-0.7516768109143076,   -0.2343682305248465},
        {0.31874940776640176,   0.9213519866654007},
        {1.6679079900117206,    0.24846159433812928},
        {-1.4152696117885672,   0.8563861838760812},
        {-0.8537596621464926,   -0.25597936224597595},
        {0.47776893274360593,   -0.9296483250778421},
        {-0.4113714152561294,   -1.7861386931266543},
        {0.25254100617694775,   -1.5833173207516664},
        {-0.15326042889877867,  -0.8216192068886373},
        {0.8241643836283994,    -0.263441949416723},
        {1.0475448486956271,    -1.41954391671776},
        {-1.775242524371749,    0.36634159599121424},
        {-0.6361726258657243,   -1.7003961750364178},
        {-0.11069025124321817,  -0.964153202570138},
        {0.15808297943044705,   0.8029940662336007},
        {0.7799693266274637,    -1.6045511527654777},
        {0.1668098259000942,    0.748042533876408},
        {-0.25663323921960896,  1.6944137250686584},
        {-1.879102962467806,    0.06989400861132893},
        {-1.5842671929725871,   1.0368177538657308},
        {1.1163026651766395,    1.1481505936904386},
        {-0.24698480395321304,  1.7617923415211008},
        {-0.48401138751707484,  -0.5431667933893741},
        {-1.5410480632433683,   -1.0001766310271312},
        {0.7046097473965958,    -0.615166415021393},
        {-0.6570126984025988,   0.4481981063986857},
        {0.4931409050859135,    -0.5682788076600713},
        {0.20493516866810307,   0.7696874994319335},
        {1.129336792044772,     1.376554058276607},
        {-0.9435547759248105,   0.11807033587424377},
        {-0.363334099897337,    -0.9519968407352738},
        {0.6211574623961662,    1.649124524254268},
        {1.591178246477149,     -0.8164079172451015},
        {-0.8947008512967755,   0.1256693443112459},
        {-1.2228162529789695,   -1.1447935183687978},
        {0.515136963834485,     0.729752741670196},
        {0.7603368114696094,    0.40851440923206084},
        {0.3469567895928142,    -0.7532459209669433},
        {-0.25876972535090415,  1.7659728895019882},
        {0.2705238695106778,    0.9379909491001008},
        {0.4238753428950269,    1.769010637556916},
        {-0.3877261555776261,   -1.6570407844673247},
        {1.6602408728629034,    -0.7350413573726912},
        {-0.2507418490973829,   -1.0322254344969983},
        {-1.6644785008204297,   -0.67575239566263},
        {-0.7807894887880551,   -0.6853211267466923},
        {-1.470391780542587,    1.0219066568894082},
        {-0.8618105802551084,   -0.5318822688978118},
        {0.8191206773232254,    0.16885364980217596},
        {0.2143866900467713,    -1.8967739057317408},
        {-0.924073118453014,    0.08234661014969874},
        {0.7743276118835919,    -0.11049894463913193},
        {0.4868665910366169,    -0.6299876886610425},
        {0.8596457495963886,    -0.22581443795875236},
        {-0.23610623557775473,  -0.6021510594297317},
        {-0.6539930803561528,   0.6421415818387174},
        {-1.777454988940594,    0.5538636291765483},
        {0.14983653979918798,   0.6780065608499891},
        {-0.8609363437067807,   0.20151958929117553},
        {-0.7655769327051827,   -0.241146033616942},
        {0.12810004904526923,   -0.9247352375649832},
        {0.9201690447134273,    -0.18882786950066202},
        {0.5897858807545896,    -0.4686479974787969},
        {0.7397866942833787,    0.5265918320178062},
        {1.251726700091036,     -1.1965019949536408},
        {1.0448830797865885,    1.4369801329860434},
        {0.48236810132607205,   -0.6858832179875023},
        {-0.4467045331682352,   -0.6899153327337633},
        {1.599780894908165,     0.281959426007324},
        {1.7307433967678667,    0.016478011924572593},
        {-0.7871337496389003,   0.20818183772378962},
        {-1.7003092023695339,   0.06980125002044406},
        {-0.8132866949359924,   0.02261891075131109},
        {0.8607411831813914,    -1.5343068589481554},
        {0.0026409481765793824, 0.9481542397945519},
        {-0.40353237082116206,  1.7490875444868839},
        {0.9151916979479822,    1.6449489995403999},
        {0.4539970917230447,    0.6932783871804459},
        {-0.28223702420679353,  0.8125754079861648},
        {-1.806336304942954,    0.056354838811948485},
        {0.11124688411080058,   -1.8942313661352694},
        {0.5890742490771674,    -0.6435031348088142},
        {-0.37718883544024434,  -0.6983903884020165},
        {1.698861870347919,     -0.779239616870921},
        {1.6690848284292594,    0.2922146887317957},
        {-1.7524649394956993,   -0.1823515042278383},
        {0.39784370463031393,   0.7891532086270947},
        {-0.40600379263286324,  1.7164494263358092},
        {0.04548621616096276,   1.8376409171346921},
        {-0.26666570199671225,  0.8123203997065},
        {-0.13474450044656774,  0.9262788374071794},
        {0.7055472094520125,    -1.586742305960923},
        {-0.6717415401358602,   0.5448848495819983},
        {0.529800266211088,     -0.8453606269060481},
        {-0.7568405759030176,   -0.5955574882595458},
        {-1.6442460374470325,   -0.7084056686495597},
        {-1.293906930045154,    -1.1292762264249077},
        {-0.25861860520661784,  -0.9496702986838026},
        {-0.39376624956490963,  0.604695015141},
        {0.7998428614047302,    0.15171431574799182},
        {-0.7820958390291876,   -0.12060196499879729},
        {1.6991728745042827,    -0.1271232379726218},
        {0.8872801955427377,    0.5378331100186184},
        {-1.2855773781852193,   1.419977903300499},
        {1.7220528312503383,    -0.3243897818886192},
        {-0.8211456107972352,   0.1638488968890658},
        {0.17374867956308823,   -1.7769343070534338},
        {1.3417154714924935,    -1.256002524947325},
        {1.483763622795201,     0.8311942876451756},
        {-0.7168790573947516,   -0.4345002684267093},
        {-1.1630426424778875,   1.3886091244757288},
        {-0.26184410917040335,  0.9630146709393291},
        {-0.9367534188949636,   -0.04459923600954223},
        {0.056618280440249184,  -0.7881441306615992},
        {-0.5619364091767568,   0.6999334978056614},
        {0.0048139994305981745, 0.8459536822974965},
        {0.37633121472109626,   -1.7277737507702613},
        {0.33204382425994283,   1.4938849924550568},
        {0.8070948901174854,    -1.6421413944571326},
        {0.777200300926336,     1.6438685851789916},
        {-0.029637563785422382, -0.9622543403980284},
        {-0.6771245011555266,   0.4640629534826804},
        {1.0404404837584713,    -0.20481044599428788},
        {-0.9165870015662348,   0.3721655648848187},
        {-0.6628642314583447,   1.5621128013678314},
        {0.715952382208885,     0.6401574629602917},
        {-1.2468427643285562,   1.4185072003162695},
        {-0.3138555980890896,   -1.6427353475565336},
        {-1.094037357487163,    1.4445991445225104},
        {-1.480010503141354,    -0.7704811421988039},
        {0.5108710625048163,    -1.8887079727815022},
        {0.6619682453545017,    -0.5377446370066854},
        {0.541073463445788,     -0.6282205082121025},
        {0.9738473957650677,    0.4020678468311133},
        {-1.6077208743924445,   -0.9650273332051091},
        {-0.8286449104769832,   -1.5401455528387933},
        {1.1754840365945358,    -1.4024419678451456},
        {-0.4108715378648391,   0.7809124720525048},
        {-0.10533394947001304,  -0.9326202531955765},
        {0.4331924837141347,    -1.773650907481856},
        {1.5429523501158453,    0.7357588994957414},
        {1.5512490680023487,    0.52203197328992},
        {0.7060745883333676,    0.8954914734965552},
        {-1.508706096554454,    1.0960540472252784},
        {-0.8095205087665203,   -0.5268859989855762},
        {0.3986278609188972,    0.7925294194055202},
        {1.8647099958620157,    0.09491096959393648},
        {1.205614585725292,     -1.1138715333599298},
        {-0.5181161602538339,   1.7901892571970681},
        {0.3663572574315551,    -0.8118206038884064},
        {-1.9369686043481713,   -0.09885295107613054},
        {0.2761815579655608,    0.9409712810702455},
        {1.5608591025320155,    -0.6024520733341271},
        {-0.7183110382398517,   0.4474674926271062},
        {1.1718896726064176,    1.4040364189165706},
        {-1.6881697719235846,   -1.0173841368135876},
        {0.08377039506896385,   1.65937463075631},
        {-0.08339794565014849,  1.8899575394384918},
        {0.7213081350336914,    -0.5487566881091999},
        {0.9605425419303041,    1.5425295158632881},
        {0.7554030360114484,    0.4890297317158895},
        {-1.8875064214429664,   -0.7573041692584463},
        {-0.6241575733731173,   -1.699559131851832},
        {0.08836135407841189,   -1.8283890330427914},
        {-0.7260747122488921,   -0.4391886587547182},
        {0.292357295423443,     0.913698441197096},
        {-0.9496761028077703,   -0.350729269128944},
        {1.682689938907051,     -0.45984311635382297},
        {0.012488657693095184,  0.9261209354023523},
        {-1.680716517608105,    -0.5250955515500088},
        {0.2716378030419713,    0.701471622049158},
        {0.6921015210227809,    -0.10442998407252882},
        {-0.9079429326329581,   -0.2455776091039303},
        {1.1773843531608892,    1.2877006445087165},
        {0.41784971995637493,   -1.0718950973706578},
        {0.41681480634600593,   0.8982530756732222},
        {-0.0611491792516752,   0.9597401915775562},
        {0.5111041840383609,    -1.682131762602394},
        {-1.804496777594074,    -0.06864510025923123},
        {-0.8085538987673984,   0.5810253851610272},
        {-0.010546014198427217, 0.9508507794266847},
        {1.2421568643617853,    -1.1579903051990996},
        {-1.0469209156777812,   -1.4848952897834273},
        {0.7681238490297013,    0.06484162961384204},
        {-1.8176107095161884,   -0.10803266221428512},
        {1.7654147431585236,    0.40086803847795},
        {0.13132187185369307,   -0.6765901087994844},
        {-0.7193790580295791,   -0.38896833000247566},
        {1.9308730417264817,    0.0007589244681099416},
        {0.4740041641867531,    -1.6362278065111986},
        {0.20256018361635272,   0.8538440611705659},
        {0.34181880917064783,   0.8729372123085133},
        {1.5471345958372396,    -1.02977445010466},
        {-1.5237595745831016,   -1.1806932384896685},
        {-0.7911813814233373,   -1.49221978377678},
        {-1.8516457740503658,   0.4563054459007778},
        {-1.798525940137757,    -0.18435003463086846},
        {0.8784595723075196,    -0.328938235156665},
        {-0.9184847606968426,   0.3099489537876788},
        {-0.45902957516441123,  -0.6663792110180682},
        {0.8650149622902802,    0.04900171392293795},
        {0.7559262783365113,    -0.4835638398138581},
        {-0.8941848095477225,   0.32197913818020235},
        {-1.703325641386968,    0.6849981483163792},
        {-0.557278362809677,    0.8157101731519422},
        {1.4319123167591243,    1.3465486625534415},
        {0.46368745044458565,   -0.6427557430234884},
        {-0.007277558430799407, 0.9122180841066939},
        {0.34833420033781903,   -0.8666033912468589},
        {1.8226048997060253,    -0.4027177010752297},
        {1.8461054488541895,    -0.2572737792277252},
        {0.6637209766800993,    0.5611207501318624},
        {-0.7663923339982643,   -0.24259649188365293},
        {-1.6294325500487232,   -0.8164591053919087},
        {-1.7425171847963539,   0.4569341937417075},
        {-0.824712524314243,    0.6324913506700863},
        {1.5646051238297245,    0.8361825041883824},
        {0.35997630806644715,   -0.8435499028499149},
        {1.01014688266989,      -0.14680880156839743},
        {-0.44367507464695566,  0.7822193060345436},
        {1.7264986380494973,    0.1076040901161143},
        {-1.7064604415672526,   0.6055549190380483},
        {0.16838519412667358,   0.9183332975997979},
        {0.33599254635701303,   -0.9277593796300267},
        {-0.3813129565597217,   0.76033421725438},
        {0.9553825358887865,    -1.3528799266014555},
        {0.7480583377569789,    0.6672693466718947},
        {0.47556118920082796,   -0.7558823211226914},
        {-0.27294634875719165,  -0.6897182309719776},
        {-0.669987315598815,    0.4826155424610076},
        {0.03438358721287551,   0.8853153551344763},
        {1.0389069313743875,    -1.558373831974114},
        {-0.7431097479124373,   1.5884764931114193},
        {-0.2836241895067993,   1.7865385380486878},
        {-1.7257603241688941,   0.5445823075936119},
        {1.7702183283575303,    -0.47517853091906426},
        {1.5270975927943793,    -1.0195785487618956},
        {0.2308036840888804,    0.8613117450276947},
        {1.0469069006437242,    -1.3666343469391196},
        {1.0740590260730438,    1.4106195236545003},
        {-0.7101035401914819,   -0.522246547561152},
        {0.7244159237242402,    0.5311045999809314},
        {0.8833938288096767,    -0.5671517734167969},
        {0.876210090808087,     -0.025742777516860535},
        {-1.1682789774078746,   -1.2727064118666565},
        {-1.6712508932981078,   0.318617574170883},
        {-0.555615639867047,    -0.6062478007166626},
        {-1.444581945830599,    0.8392984921242527},
        {1.264767508242734,     1.411954250234887},
        {0.9049675935383332,    0.4994137560472374},
        {-0.6303374352553708,   0.5888262688930135},
        {-0.796880070940927,    0.20955803358425656},
        {0.7511099378233063,    -0.2900548853248946},
        {0.1134353459250921,    -0.793485403602855},
        {-0.13874257886135177,  1.8842311358672892},
        {0.8361004258450456,    0.3301974037025674},
        {-0.15778370394366015,  -1.7099365330549732},
        {0.11095288589090689,   -0.7872554322837015},
        {-1.186309004854786,    -1.253912562555193},
        {1.542782881987084,     -0.8818463730863291},
        {-0.7059190082252961,   -1.7226388563167188},
        {1.4121144206126721,    -0.8803616797550703},
        {-0.1677547822472405,   0.7302339413711388},
        {-0.2932358657209621,   -0.9891551013119417},
        {-0.5512251467997207,   0.7280556684643144},
        {0.7195855389972292,    -0.5974295493930977},
        {-0.2936353925357282,   1.5529863458873576},
        {1.4012503847623514,    -1.3911527496740628},
        {-1.2242897874829568,   -1.3014156918847084},
        {0.8883047903927019,    -1.4262055901865967},
        {0.05330677659891448,   -1.556677703143575},
        {-1.6967731336316179,   0.4784125528567482},
        {-0.20013449876528228,  0.8437205652518477},
        {-0.7153991090519755,   0.1361525100520285},
        {1.1698519928962672,    1.4128102984112763},
        {0.9573558279882249,    1.4767810202226275},
        {-0.80457404912266,     -0.6869808811240294},
        {0.9482026328811533,    -0.26654516096664},
        {-1.1751770652239764,   1.395845721851893},
        {-0.713978352134342,    1.8285316378260916},
        {1.5090071979295727,    0.9745794226629836},
        {-0.2506210475902469,   -1.0095695330228378},
        {-0.9176759939497636,   0.1702976770977096},
        {0.8718611726688482,    0.2133255289507419},
        {0.04788014960636064,   -0.9751314267958126},
        {-0.8038487163571831,   -1.6629624557365448},
        {-1.4441747217349563,   -1.2356701831925077},
        {0.7863785168032433,    0.06579717676017786},
        {0.915386669819278,     -1.4912889402330827},
        {0.5050496630374017,    -1.602181439381276},
        {-0.3534993497792579,   -0.8867153929624128},
        {1.6724561868752388,    -0.33628409811072224},
        {0.8190404978558004,    -1.3645990905220127},
        {-1.4273044838057627,   1.09796008703747},
        {-0.29293951738700824,  0.9214851067513057},
        {0.8846914862602118,    1.5563030548430086},
        {-0.6216389341360596,   -0.8150877687502757},
        {0.21618985827703624,   -1.7110787738814932},
        {0.784985206218328,     -0.6199632135768746},
        {1.2021927349262163,    -1.2834600338991882},
        {1.0233901395930352,    -0.031742431517070464},
        {-1.7625822223689247,   0.2120137008473465},
        {1.664003359441974,     0.7065745731775784},
        {0.06853892538879529,   -1.8984328254553753},
        {-0.3616627984321554,   -1.7199625702619434},
        {0.05254562848987752,   0.7890891484193909},
        {-1.630927530944118,    -0.3114993902521693},
        {-0.5962755069410361,   0.6929995648370196},
        {0.8348668371562721,    0.08738083936487959},
        {-0.579340007805068,    0.45050054409831025},
        {0.8399981678869696,    0.039199965808976826},
        {-1.6506174616561715,   -1.0124140473021261},
        {0.8172562423308506,    -0.49063280387969305},
        {-1.6613864798207356,   -0.2955031461350397},
        {-0.10942564378583147,  1.0480558222622258},
        {0.6465829855201969,    0.6461420096803636},
        {-0.9041725345444066,   -1.2358310393688157},
        {-1.0897818949719515,   -1.4644599565129701},
        {-0.8287592477455317,   0.13024982577328914},
        {-0.8922115466802509,   0.3859964833118546},
        {-0.7941343897046576,   -1.4878147501970194},
        {-0.7537297814756707,   0.14956666603689278},
        {-0.5058947061749718,   1.8113736932315894},
        {1.3321439653235085,    1.2239380078745745},
        {0.7827206048921211,    0.30636235932239353},
        {-0.5950825853207209,   -0.5730937391119301},
        {-0.845189291897313,    0.3887008774930933},
        {1.771518327474698,     0.2609847965599126},
        {-0.22705149607622763,  1.8302466807635722},
        {1.7378055286263947,    -0.04591258991270986},
        {-0.8029269667632899,   -0.29216816841062754},
        {-1.1270180850214992,   1.2957304801593976},
        {-1.0404734924324668,   1.375611321002607},
        {1.0445466497275242,    -0.2608675084490617},
        {0.11727595388406571,   1.6544634985687219},
        {-1.5384733270302513,   -1.0258163909044353},
        {0.004506503532839015,  0.918006674754909},
        {1.1722293857355712,    -1.2677738100053664},
        {1.2539361493694172,    1.3967345233206878},
        {1.562558106485145,     0.728680159713078},
        {0.3152757163355239,    1.101940509756533},
        {0.3598109218029332,    -0.8298326556270665},
        {-1.5687989021448694,   0.670414765190582},
        {-1.699009523067426,    -0.9229248326609335},
        {0.39212800880297055,   0.8806403092172037},
        {1.6305587336467804,    -0.8713510196991758},
        {0.18275135550971341,   0.9474914986897449},
        {-0.3067687793190762,   0.8927473921562569},
        {-0.08257527748551335,  -1.7350749711231679},
        {-0.6622921912163391,   0.39210498983399034},
        {-0.7592277045196888,   1.5432851295862187},
        {-0.3942935050486711,   0.742355971729439},
        {0.9154540975630956,    1.5327962462114384},
        {-1.7046473828057631,   -0.1951545203968779},
        {0.7766233197633968,    -0.5780505955605574},
        {1.5979392014094713,    -0.16993056425608705},
        {1.607652508220879,     0.9176133145718676},
        {-0.5406125110962353,   -1.6581482106483423},
        {-1.7086903354684624,   0.529409946025974},
        {0.6955160434224299,    -0.2057262264181398},
        {0.3752402307288385,    -1.8028549388348385},
        {0.6006310854867914,    -0.5904427794994861},
        {-1.6289074840631403,   0.7841838682944743},
        {0.0698924420186624,    1.7892739489294205},
        {0.7511079801642726,    0.5346381603777633},
        {0.028615661716293653,  0.9223602891970557},
        {0.13415501109896052,   1.7393237428249793},
        {-0.30788793153919,     -1.8728421161450715},
        {0.9395791740961117,    -0.2953787953281471},
        {0.8128397244347954,    1.4826435330002008},
        {1.7031217421162208,    -0.26013935050023},
        {0.5323786985680187,    -0.7821713689042693},
        {-0.6958081937307676,   -1.6045357937613018},
        {0.6251983419009675,    -1.6943035048081245},
        {-1.745313471571715,    -0.7376975577508744},
        {0.12263548716126607,   -0.7941512780487366},
        {1.3221756486331637,    -1.0214371453097657},
        {-1.5287525622564715,   -0.9364797772314895},
        {-1.6906189578026831,   1.022492077732009},
        {1.8245049078856577,    0.12084669740916082},
        {-0.9003418090662765,   0.1363237121143903},
        {-0.00944397198830432,  1.7723618558639171},
        {-1.579569816700203,    -0.2491556390136071},
        {0.0024614270135854344, -0.9246571732564752},
        {0.15407390168857982,   1.8213813263228031},
        {0.32677910539506294,   -1.8122574623535967},
        {-1.6325421524777999,   0.5299976277151669},
        {1.820503106896229,     0.20471696942267859},
        {-0.905787285610552,    -0.3735186635506387},
        {-0.8484571666677811,   -0.2090649203262057},
        {-0.6885456955107258,   -1.6627228492066763},
        {0.5004378847373941,    -0.5870415439210053},
        {0.2793387743007452,    0.8580580233989977},
        {0.1850478173304925,    -0.7937893049435379},
        {-0.2789951938529166,   -1.937195770571466},
        {0.3969683914058021,    0.7945798829501812},
        {0.1233774072971899,    -0.8106529812774455},
        {-1.5185993436504575,   0.8806413843676252},
        {-1.8750998347741057,   0.2067858528588975},
        {1.3202868432892487,    1.299552629674641},
        {0.6328440011966666,    -0.6403661962059272},
        {-0.832930826674872,    -0.463658938935426},
        {-0.006386163472796426, 1.0015282617709784},
        {-0.6483447254144795,   -0.7039493600468082},
        {0.8561194462172967,    1.6738071663735532},
        {-0.8540415430629427,   -0.17347472445553436},
        {-0.5590893742729545,   1.6977500514926185},
        {-0.8059308236457685,   0.7403253564851401},
        {1.5985039105381653,    0.8519036776334481},
        {0.3211389837781283,    0.8798100328040487},
        {0.32013330117565375,   -0.7355571845917583},
        {-0.9963691914723265,   -1.3967285963058347},
        {1.5803215248318843,    1.065120330293154},
        {0.18878437208777538,   0.8801185316223011},
        {0.6430752899083677,    -0.30225779465133273},
        {-0.8040116338613458,   0.042503874832881976},
        {0.7045851197204749,    -0.40911564714653664},
        {0.39648403013897554,   -0.8570114446305794},
        {-0.5610682091498809,   0.6722217859825194},
        {-0.6239342825203888,   0.5596710478717015},
        {-0.47799518758000503,  -0.7147013080035586},
        {1.7591458707348244,    -0.2882540440859486},
        {-1.2085899784150569,   -1.454505153158128},
        {-0.3329239268187873,   0.7921864174296702},
        {0.6040016389658321,    -1.7093981868099886},
        {1.793285992252206,     -0.07539003404253179},
        {-0.21453719672862592,  0.6511267979029093},
        {-1.6429231209205928,   -0.3931613644696081},
        {-0.8111081237558724,   1.6591649714150793},
        {-0.41696993702063334,  -0.7115752033097164},
        {0.6585813174361862,    0.4379329814091057},
        {-0.6842544907641446,   0.7756480953730416},
        {-0.8939647659534021,   1.4052255874340935},
        {-0.365693941452224,    0.9591099518027428},
        {0.14769320617430481,   -0.9628288326383716},
        {-0.6398442948854216,   -0.6735843597569546},
        {-1.747425375162454,    -0.3822304785730517},
        {-0.2817470151880604,   -0.8975034549169818},
        {0.24801258403648027,   1.750649690029722},
        {0.03706773395692881,   0.9620939625389672},
        {-0.8716199820224653,   -0.29229507989039183},
        {1.691163575873018,     0.1999460979224772},
        {1.15913166804291,      -1.121366189496245},
        {-0.7504325097066031,   -0.2948687015652657},
        {-0.4363176522985456,   1.7389560525655088},
        {-0.49128622939511885,  0.7472683866472934},
        {0.23415608818908706,   -1.6452785661373184},
        {-0.5200678721715581,   0.8569791910532489},
        {0.9487484150418595,    -0.16198801571280788},
        {1.6772623274519172,    0.7007212024481376},
        {1.8862178511393421,    -0.3468341331594473},
        {0.28691102181742023,   1.78612532017231},
        {-1.7726534864144543,   -0.6688301434942472},
        {-0.6204031954200415,   1.7312453888524464},
        {-0.9545763775487273,   -0.14597036726376494},
        {-0.6777881640364822,   0.7013616937635173},
        {-0.7982686143355007,   0.19760353343659898},
        {-0.8219212679834608,   -0.2990732213718376},
        {1.5712753436406828,    0.5895966024233337},
        {1.212593432480895,     1.2402214563625145},
        {-0.07001582966282434,  -0.9030621704546208},
        {0.5743099634086346,    0.6265115865624721},
        {1.529521437444283,     -1.0961716727709208},
        {-0.75714578576919,     0.20672350148172697},
        {0.0728348506676223,    -0.9996546637620827},
        {1.2934212390617998,    1.3597912830797587},
        {0.27419283446961235,   -0.847014909748074},
        {0.27449931032542824,   -0.9555636639585624},
        {-1.5271276783431924,   -0.7974830012391709},
        {0.7405265908365253,    0.42755255422449895},
        {0.7381086542320662,    1.63613464826239},
        {-0.22835375156071838,  1.7552605103848393},
        {1.5082158617382475,    0.9182406468629197},
        {0.4361785299246939,    0.7682876868583095},
        {0.7281939183798463,    -0.7532527857003047},
        {-1.1197271510749758,   -1.4474782460104485},
        {-0.8617744191941615,   -0.19968336047348556},
        {0.26908983054015956,   -1.6923423514282165},
        {-0.7307565265972635,   0.16320494980391204},
        {1.2128465252507676,    -1.2591280842822747},
        {1.7826459022390109,    0.7849252265673058},
        {-0.1802611695620948,   0.8066318445396152},
        {0.44316649516090206,   -0.6370826783849387},
        {-1.3086198514394496,   -1.304921611737718},
        {-1.5873780277055325,   0.5250035526776842},
        {-1.767950742615227,    0.5797834863853459},
        {-0.026377043548027643, 1.7155808306752303},
        {-0.9004482440221369,   -1.36380521627743},
        {0.7002084031558976,    0.5318752039585758},
        {-0.6678299464328493,   0.5819735733925808},
        {1.0371876385346137,    -1.4751073430138506},
        {-0.5092681137901708,   0.7305845484540991},
        {-0.7876024977045544,   -0.23155936057952287},
        {0.16613507428107843,   0.910477527063182},
        {0.6031326085437683,    -0.7758192576190134},
        {0.2758949421320316,    -1.6490739559222527},
        {0.18024388818268897,   -1.685791850495495},
        {0.7282964721501288,    1.6023852539059233},
        {-0.6266756962627723,   0.3589887933810536},
        {0.9335468145369231,    0.017155007523799008},
        {-0.5491895702110398,   1.8258658143838142},
        {1.7683663722170224,    -0.3826235352183755},
        {0.22835414550747585,   -0.9040919148263586},
        {-0.19694446835303558,  0.8932877821843578},
        {-0.35414982467080475,  0.5906465744894442},
        {-1.3191328187701972,   1.0534441261121255},
        {-1.5048227928773035,   1.0099826487444572},
        {-0.3192791607676057,   -1.7358372068259398},
        {0.4405640879991809,    1.6853464166144079},
        {-1.1744054381268632,   -1.2502323377657547},
        {0.8866326201323178,    0.02504171705164777},
        {0.8528019828528157,    0.31316022753138906},
        {-0.4257121600593402,   -0.850108429626083},
        {1.1373663393198465,    0.1964698162014882},
        {0.13946972594021562,   0.7779167771769607},
        {-0.8634714203354573,   0.2746895329845589},
        {-1.355992519979177,    1.0868279350679342},
        {-0.7019718351699271,   0.49573642153334485},
        {-0.6282162946384975,   -0.6818559493445833},
        {0.946952339073398,     1.3917457139757603},
        {-0.16732559989011364,  -0.884371317041773},
        {-1.8093888468602433,   0.3496002850936644},
        {0.24112003917641828,   -0.6465457384701268},
        {-1.525089971826815,    -0.9420204844408363},
        {0.16466247150908356,   0.9154939239243283},
        {-0.9887208410779282,   1.4613916218457341},
        {-1.0155043337956893,   1.2598490657750836},
        {0.746445703443426,     0.28123058791543376},
        {-0.8722271124041419,   0.4162480299289282},
        {-0.9604853954118168,   1.5362852024969822},
        {-0.19994237914802102,  -1.9747218755217704},
        {0.3432637490722837,    -1.7703511196485855},
        {-0.37042106650839535,  1.7363125998152442},
        {-0.9904581286949862,   0.04659847400333188},
        {0.9245898685513595,    -1.6307589310450967},
        {-0.5556006817593209,   -0.5093331689788777},
        {1.3961403389592277,    -1.043743468621825},
        {1.5145492385999975,    -1.031271182299323},
        {0.06410366532190587,   -0.976110244180666},
        {1.6514874930317878,    -0.78683568862704},
        {-0.5510451325413241,   0.8553130189954992},
        {-1.7271836557599056,   0.4743612325503485},
        {0.11265304515770957,   0.7337286143309537},
        {-1.7818615607505905,   -0.5786111885617403},
        {-0.10242016464579294,  0.7381473989628217},
        {-1.311052674291831,    -1.2957539397580535},
        {0.23017619693583977,   -1.690034136592399},
        {-0.7240530795647785,   0.2794999111257145},
        {-0.34417132733014805,  -1.6618779142032727},
        {0.8886181636440106,    -0.15323822238321622},
        {-1.7597453205632863,   0.5396818355801695},
        {-0.6979720458781434,   -0.46596613961383127},
        {0.5778143446084972,    1.3606266832974077},
        {0.9029401040858676,    -0.2981020607085061},
        {-0.6441741030921584,   0.5037192862930491},
        {0.20311790045438075,   0.8291165628305844},
        {-0.9559529264014677,   0.16231942528076296},
        {0.5449133024575056,    -0.8534572464029262},
        {0.8213128178627839,    0.5262110506832334},
        {-1.7361109814319315,   0.38582315020663016},
        {1.8057757227615878,    -0.09433875933822909},
        {0.13824308448979183,   0.9544273100852704},
        {-0.7766231547350821,   -0.6544298954229489},
        {-0.9943878404764869,   1.2589622365571456},
        {0.6148714089136015,    -1.8602668805702616},
        {0.6409074915866333,    0.4166097623581398},
        {-0.1464102976140198,   -0.8876376565257825},
        {0.6476222531574026,    0.7339879325875419},
        {-1.697566682658748,    -0.6633575047985526},
        {1.66688356161185,      -0.7612988672302116},
        {0.10636529094953255,   -1.68264490824855},
        {-0.34695689037555094,  -0.8469805941173842},
        {0.6642899077223259,    1.7080180886418435},
        {-0.0896119121027898,   -0.8320672732956276},
        {-1.2049245367465646,   1.1171160768199682},
        {0.6719137847832272,    0.7633182173177572},
        {0.6881488338737584,    -0.04327557671349168},
        {0.581637006557306,     0.4577193797649204},
        {0.9005796692924362,    0.2289534870926109},
        {-0.3190406717655754,   -1.7344797439426254},
        {-0.8564159074487052,   -0.2414947295359827},
        {-0.5296024402612939,   -0.5552304937274546},
        {0.9521057997959378,    0.08866952115091761},
        {-1.7701966847370025,   -0.3093478919982523},
        {-1.5152789987191162,   -0.8215708632864956},
        {-0.8724497933948643,   -1.59506588060636},
        {-0.6390419968994954,   0.6267186948059832},
        {-0.24996116051155914,  -0.8087347740120839},
        {0.7468553503495602,    -1.535369135852056},
        {-0.2589765426898178,   0.9155048996359825},
        {-1.2363063512350019,   -1.342851109364984},
        {0.8080258140999277,    -0.7428192951497836},
        {-0.8656087844204038,   0.2215522282244926},
        {-1.7802240340947235,   -0.18644079405810385},
        {-0.74342052582783,     0.8948539868823098},
        {1.741535431626289,     -0.6965913075556843},
        {0.6251831271288731,    -0.5339585094725601},
        {-0.9357302431296114,   1.3961957049987666},
        {0.38153820078968287,   -0.8720957338162502},
        {0.5135638127532087,    1.6363083160998406},
        {-0.11752947502991774,  -1.0268273313949168},
        {1.4179994220295526,    1.2196399323978144},
        {-1.885207293101282,    0.14587668748586302},
        {0.5750987433584057,    0.6152218065687672},
        {0.5992065640364217,    -1.5860341760166325},
        {-0.8238789205143028,   1.446533038809267},
        {1.04266354969215,      -0.042342613354294234},
        {0.1151498080358502,    0.9653301200407617},
        {0.569410929307418,     -1.4331727754952368},
        {0.4368105947802843,    0.7106495218606339},
        {1.2209371798182425,    -1.3684633027699697},
        {-0.7401091384826175,   -0.38933500588741643},
        {1.674293933831842,     -0.31093575061079987},
        {0.21556249570522146,   1.7563550568370665},
        {0.03130552899915105,   1.7991814581424774},
        {-1.6744437999628365,   0.903705385899058},
        {-0.5539420257140509,   0.8093814507673696},
        {-1.545581670695577,    -1.022028472530296},
        {-0.4825752754598775,   -0.7793012960143804},
        {1.0079979670803385,    -0.026130630099923162},
        {0.03777416940996365,   -1.8420016984097498},
        {-1.3057697599163267,   -1.1267673199637955},
        {-1.6551258706325627,   0.6180322797186212},
        {-0.5633689780595537,   -1.7549860401794906},
        {0.49228417593778157,   0.6791389812793402},
        {0.02738997863509739,   -1.7264019140141322},
        {-1.356587884396311,    1.2607471382274924},
        {-0.5005810406874427,   1.737875248832495},
        {-0.758924069657672,    -0.536922296399063},
        {-1.711385559419461,    0.8229188449408396},
        {-0.7475933753810761,   1.5318872874252927},
        {-0.506948841390935,    0.7662827280589528},
        {1.1843092551827303,    -1.4307704537619856},
        {1.4884616793164323,    -0.9664629532056289},
        {0.28391626409759585,   -1.0298955076572962},
        {1.6290923494676743,    -0.9044965324678288},
        {0.5914526182742447,    0.8502336893645895},
        {0.33164239271478224,   -0.751679318126527},
        {-1.6288679819108047,   -0.6881583748091609},
        {0.46961318991086043,   0.6562125229594942},
        {0.8104662997757505,    -0.17475498198567874},
        {0.3741348851566926,    -0.8819567693848777},
        {1.048940051209624,     1.5371735918702587},
        {-1.738549514480991,    -0.6634906328090858},
        {0.9813101931186021,    1.648206425632573},
        {0.8203950103632924,    0.5273524782390674},
        {-0.6842641270962665,   -0.3247603574199445},
        {-1.6383938417180706,   0.010921055074283037},
        {-0.06258202450594605,  -1.6512691481387507},
        {-1.9167335182391119,   0.2175781231441189},
        {-0.1077734671872573,   0.7971952383843901},
        {1.6283720610375718,    0.879385579005061},
        {0.7160603228339206,    1.5049237294292392},
        {0.6897714811344681,    1.6730122413771418},
        {-1.0434387696735072,   1.554837704102853},
        {0.14369550023420238,   1.661950299623726},
        {0.9282225245707587,    -1.6744720188210194},
        {0.6273127224503222,    1.716439722948603},
        {1.5982192817584084,    0.44109691047196764},
        {-0.578471105084617,    0.7181426775976106},
        {-0.7436631833519728,   0.21863067706503106},
        {-1.4857071518076999,   -0.9002048183955293},
        {-1.5822402613099766,   -0.7955726565645924},
        {1.397871496817146,     -1.0772977747831758},
        {0.8553352867484575,    0.0727385278719895},
        {-0.9176753277600451,   0.05482121898314945},
        {1.1788369076179532,    1.3261455113432787},
        {1.4629248517533593,    1.1201214788773326},
        {-0.9881730328096432,   0.07889256290887578},
        {0.9030472283138473,    -1.4811312269397379},
        {0.44236446720670825,   -0.5698927447875564},
        {-1.4232343345554956,   -1.0436469108342292},
        {-1.8332140465144482,   0.06894998297813089},
        {-1.5778178415644457,   0.5078739614792837},
        {0.7074273224016634,    0.28101611695549444},
        {-0.5802863696280344,   -0.9237792525604277},
        {1.7879397088758853,    -0.30779650231707545},
        {1.8730574202858983,    -0.5758828438730761},
        {0.11532611330597797,   -1.8305584902131793},
        {1.6474447826098841,    0.5991596531786866},
        {-0.79507181121165,     -0.5368877964735197},
        {1.8401532771425912,    0.3301670527221259},
        {-0.6168218222394252,   -0.3366443804012789},
        {-0.8414019339377679,   -1.5483420703498336},
        {-0.8373898443469233,   -1.4914177333455372},
        {-0.9545615347239317,   1.367008524011948},
        {1.6368528717358568,    -0.3176787265640692},
        {-1.3230070707388881,   1.128324506127081},
        {0.42352519739255706,   1.876860200172847},
        {-0.8859969733442965,   1.5331606704680445},
        {-0.3385234053998189,   0.9009948911089145},
        {0.3218001757489996,    1.6785056563765115},
        {-1.735765332511378,    0.21276688402111724},
        {-0.23192227126334833,  -0.8785247437750454},
        {-0.024121752034458387, 0.8592836023690176},
        {-1.5092081382981215,   0.7602617166780212},
        {0.760068674549989,     -0.5511036038333857},
        {-1.3916844669228603,   -1.2594349224954908},
        {0.7214092541454515,    -0.5306842797301629},
        {-1.4777678919749482,   -1.0855742331976617},
        {0.8533892295099837,    1.383054517478315},
        {-0.5097347995300743,   -1.5506909649179714},
        {0.8549277626480405,    -0.4172040320199207},
        {0.18210526042567962,   0.9485045384645818},
        {-0.3458950403278313,   -0.9272349577553686},
        {1.5824877018329657,    0.6876133147288148},
        {-0.7873847741786583,   0.36323400613324963},
        {1.0311496553753978,    -0.1642496674952665},
        {-0.754484545930785,    -0.6880514302449856},
        {1.526939789042259,     0.7708993605730327},
        {0.4028997783206244,    -0.6952968245214856},
        {0.36564805441823567,   0.8750675436748612},
        {-0.39673643645826845,  0.7230591329129871},
        {-1.398514359269427,    1.2012718478519082},
        {1.6179509782663561,    0.26537500105947565},
        {1.7893434492002582,    0.2591688741460461},
        {-0.8590336056942829,   -1.6672554938144584},
        {1.0522801175546488,    -0.02685348991178546},
        {0.35160705826708816,   1.8837307479532102},
        {-0.8332059134911888,   0.801771298557762},
        {0.015035046028212953,  1.7653247828076817},
        {0.6210623751001202,    0.6442448458894321},
        {0.9762566292019204,    1.4012662048132258},
        {-0.7793554878945819,   -0.43260430068718353},
        {-1.6891997526062026,   -0.36807523807501574},
        {-0.7603612490822914,   -0.23453877379386132},
        {1.7889024741707171,    -0.31381353700619136},
        {-1.793299810410306,    -0.3235773183366259},
        {0.9723810310175027,    0.2397224179818388},
        {0.9317485955919911,    -0.22995072415701698},
        {1.7243638053153545,    -0.6008931495335657},
        {0.02172672310866067,   -1.8720295487142338},
        {1.7313976655752934,    0.6983233423136014},
        {-0.9260331042558384,   0.17941947385293938},
        {-0.40322911661539823,  -0.6734908847836161},
        {-1.6352556698969818,   0.13980686794756844},
        {-1.4574510218713639,   1.241179809813594},
        {-0.9385800355832827,   1.3326668654564886},
        {-0.8621520774349533,   -0.24811112392000867},
        {-0.6521992863182448,   0.5362187045943543},
        {-0.18727556681971105,  -0.7452498732521211},
        {0.7972558183517916,    -1.609011200740579},
        {-0.4810491962698272,   1.5995231093357918},
        {0.20096949637874528,   -0.827384101405714},
        {1.5411699616952526,    0.6426034107880757},
        {-0.12677688179295152,  0.7394861908972963},
        {-1.1981776483278945,   0.9293963881631698},
        {0.2048397193623398,    0.7676473388641963},
        {-1.3189440127169234,   1.3019198542678883},
        {-1.0035251599203623,   0.3518336092555224},
        {0.5439009143616108,    0.5519148541214198},
        {-0.633888967514745,    -0.7057001776308117},
        {-0.7684174977747943,   1.7397555105672895},
        {-0.48428578456931,     -0.8254309780052922},
        {0.8119953459529586,    -0.540247173790164},
        {-0.8788693542905982,   0.38625368549876354},
        {0.7877331290623554,    -0.2105800736378649},
        {-1.0845201650535996,   1.5519940855891783},
        {-0.2699921696101655,   -0.8636434558410907},
        {-0.31653476057388835,  0.876073355673684},
        {-0.8384487876059957,   -0.14718998677187092},
        {-0.8404216589133732,   -0.12898037329767684},
        {1.7395892527894807,    -0.8114933971327479},
        {-1.4050154367596903,   1.237823407641654},
        {-1.740614851838191,    0.4675560694354611},
        {-0.11023670594445657,  1.6642628289654589},
        {1.6336777612726066,    0.6966166054554696},
        {0.24275448431184032,   -0.7384477470048623},
        {0.09655551690615137,   0.7821437323280246},
        {0.2775891555077956,    -0.9632568082554901},
        {0.8054774841698991,    -0.4967104636673526},
        {-0.7628498435805185,   -0.18673977531873842},
        {-0.12878664386220268,  0.9708235669727119},
        {-0.6774711730075287,   -0.6198479245687248},
        {1.7838901599665427,    0.3506289814422108},
        {-1.466775075809208,    -1.2075145414511674},
        {-1.5901652976748863,   0.9130211986612785},
        {-0.34617406138995793,  0.7091686581351383},
        {0.1330910307696729,    -1.8524731249943351},
        {-0.4471729145608934,   0.7085372964916368},
        {0.713445940379725,     -0.49812949372373644},
        {1.575648464983691,     0.8960738909959788},
        {-0.43623919880560535,  -1.775465156844996},
        {-1.8299318044979214,   0.251964412315237},
        {-1.307691301266293,    -0.8851032313856247},
        {-0.7726186549763145,   -1.5981492541524027},
        {0.6851317331399132,    -1.597212775930065},
        {-0.4001230604172703,   1.7317881095244916},
        {-1.2270115376011272,   -1.0376278284661102},
        {0.2580470723984244,    -0.7635232114298183},
        {-0.9859855849604114,   -1.4978617842077708},
        {-1.6953070218993225,   -0.15018777472532444},
        {-0.20197731287692056,  -1.7573355919643994},
        {-1.370670244527672,    -1.1453342108599243},
        {-1.6012208505293715,   -0.939255310294679},
        {-1.277804722877509,    -1.2319175571555876},
        {-1.4856453323243302,   1.0130720714021324},
        {0.18890450597302055,   1.9544740290731084},
        {-0.5616840657022342,   -0.5932323576578936},
        {-1.6222663094040053,   -0.6600263144317723},
        {0.43253556484884637,   1.5515452529839044},
        {0.482996165139692,     0.80793703452171},
        {-0.3017553801498362,   -0.7302339243839795},
        {1.4425401524233514,    1.0984957631647905},
        {0.680395424237057,     1.4939658075369717},
        {-0.03255493891245008,  -1.8314071667414102},
        {-0.7715797679413322,   -0.49176944252425436},
        {1.7093972553107049,    0.7637601966947731},
        {-0.6007619304832945,   -1.7609046936795363},
        {0.05635554613535627,   0.9440898598131393},
        {0.7400896962514681,    1.7184291635667317},
        {0.5484344702106299,    -0.5903994288264016},
        {0.8555093192404989,    -0.2847393581107201},
        {0.635156442283529,     -0.7881959315332908},
        {0.42962953524511616,   0.7134120529120348},
        {-0.28438508819201597,  1.6886446616048623},
        {-0.7242767358679774,   0.5755986962444868},
        {0.48530612388701105,   -0.5696227480603779},
        {0.5524998871828095,    -0.5434633997503282},
        {0.7732849451615427,    0.5128002405352925},
        {-1.6310374200402855,   -0.7012198106592399},
        {-0.4005040172218612,   1.753047246384738},
        {-0.6751219115613829,   0.5188810121450117},
        {0.7462044799793598,    -1.6782124630038584},
        {0.222937673775012,     1.7290064689422981},
        {-0.26796492631024127,  0.6567329276207591},
        {0.6172749798544435,    0.5908692037360257},
        {0.6389787147454219,    1.5350638519219606},
        {-1.3026728927619287,   -1.1595622246686372},
        {0.659542902010183,     1.7167388926879863},
        {1.324493227792788,     1.2051785860742694},
        {1.421290702005172,     1.079812327489718},
        {-0.8813140582108057,   0.3692300635881745},
        {0.7225725024066278,    -0.37291063676021413},
        {-0.4383729067501009,   -0.4999765959953148},
        {0.9135039629944839,    -0.07093481952905359},
        {-0.7246262770043382,   0.6117245804682542},
        {0.5372880489084996,    -1.742834737028655},
        {1.0622424539592659,    -1.567976894851789},
        {0.3590788746020305,    -1.7689625149755226},
        {-0.6340400793767956,   -0.9223408322468756},
        {0.6778510084457097,    -0.7317537074799304},
        {-0.2152488613787881,   0.7628769984546973},
        {0.766006285290589,     -0.4728040819490467},
        {0.15593962451110074,   -0.9896564634752966},
        {0.07680043373187098,   -1.6341114727747372},
        {0.1922683939215016,    -0.9586272951309711},
        {-0.1465687374736268,   -0.8553013512589954},
        {-1.5851250915601345,   -0.45795200422690985},
        {-0.9862059692517762,   -1.3161027766510272},
        {0.831810516364401,     -0.3546539386006149},
        {0.17924300543110205,   0.9522668731593616},
        {1.548325911586523,     0.54883999471352},
        {1.0381773606042946,    -1.4054848460816063},
        {-0.7863625991555558,   0.21595343603628878},
        {-0.08408896545673462,  0.8043271738657795},
        {-0.020015385178647684, 1.7351398030975604},
        {0.20256074061713691,   -1.894434619128864},
        {0.2320319321880833,    1.746125491165245},
        {0.35789590004633276,   -0.7727350068689173},
        {0.6170238975013107,    0.7158495029985289},
        {0.6867449969842826,    0.37853985914924004},
        {-0.5556807937707191,   -0.6264977837345503},
        {0.8509887035604891,    -1.4490051989271293},
        {-1.0282519918365973,   -1.3238510975418893},
        {-0.787614100494252,    -0.34941898073962674},
        {-0.27474042587668607,  1.7678252116052655},
        {0.8321607630640037,    -0.3029638209164341},
        {-0.6219874806367034,   0.5770302173067614},
        {0.3268101995124877,    0.9523304073628016},
        {-0.9867868510321521,   -1.5734903914022869},
        {-0.04232949534647494,  -0.9105612230050578},
        {-0.6434013172434928,   0.4611356712397149},
        {0.6500096985968773,    -0.5132585938112304},
        {0.5528845451729666,    1.7152066482389778},
        {1.8045393328478425,    0.29808024090331064},
        {0.9130198010322875,    1.5322023749507965},
        {0.02462708994659325,   0.8442833881288359},
        {0.7324034469529133,    -1.4081283434488698},
        {-1.667242796498492,    -0.681681863452142},
        {-0.6317394797819657,   -1.7092214121928115},
        {-1.5517306741782295,   0.7650447492302631},
        {1.696998865600338,     -0.27595565169744224},
        {0.6452052464875472,    1.5777394101755338},
        {0.42501975177353435,   1.7004963694269055},
        {-0.8683413445394643,   -1.6121117445249222},
        {-0.7389235004493133,   0.4311718011928139},
        {-0.13785726993179806,  1.799329243675757},
        {0.7559102614489016,    -0.4819960306345399},
        {0.5665386845803931,    0.45994026599868265},
        {-0.2997205028652897,   -0.7724816460662335},
        {0.8061149991438732,    -0.4290745868504593},
        {1.662978277576288,     0.0930011696494224},
        {-0.8278905790855069,   -0.36808878369848125},
        {-1.6414716046784947,   -0.36999274908778323},
        {1.4334987367431011,    -0.7645504577851469},
        {0.9535369382000373,    -0.2809421616122123},
        {1.7271771239204763,    0.3775784135167826},
        {-1.7380340134472059,   0.07520224920593396},
        {-0.8628997012617265,   -0.3423725298336831},
        {1.1178182737945193,    -1.296483571262661},
        {1.4118523944648496,    0.7128409727144505},
        {1.0849605478513147,    -1.4247018462566279},
        {0.7792268510137395,    0.2649806614604299},
        {-1.6452434680266639,   0.16330368258385655},
        {1.459363081921916,     0.7602406160589249},
        {-0.5870082976061315,   1.630610873092232},
        {0.609543446839948,     0.6010280374251797},
        {1.711578638858996,     -0.789662277441176},
        {-1.5915277367912828,   -0.5601134562782861},
        {-0.5880959681552299,   1.7391065984205767},
        {-1.7329285601834366,   0.03352485733814806},
        {-0.7652687893266688,   0.338318420252834},
        {0.45148718136112326,   -0.7567871960811058},
        {-0.7142480962432267,   -0.39590891564105346},
        {-0.1046553721554977,   0.9086572046070232},
        {1.562463672166712,     -1.1075996696268504},
        {0.2728379355832067,    0.8109119256982285},
        {-0.4759710198880737,   0.7395375383758924},
        {1.192470077821499,     1.2508219907753368},
        {0.4818716597892252,    0.6979557117147568},
        {0.008429037480913476,  -0.7895581076436717},
        {-1.7655369239950103,   0.6117662471703902},
        {-0.48592920167767384,  -0.7380045927023356},
        {0.27975481435203414,   -0.9623851041090322},
        {-0.7418364857327737,   -1.560916150920248},
        {-0.8289093197359848,   0.2481127224012021},
        {0.9678682653611537,    0.18589906006957324},
        {-0.32910203121802134,  -0.8472116061654503},
        {0.6603439902073402,    0.4818736982905861},
        {0.5998110096590107,    1.6164367987666148},
        {1.0210975231059032,    -0.42952318020250885},
        {-0.8593636599070537,   -0.08123270240019172},
        {-0.7520372593104805,   -0.5131730571466839},
        {1.4229337710612402,    1.1085520818019827},
        {0.9524783991871401,    -0.004859974687445884},
        {0.8579495360632027,    -0.47606920577286366},
        {-1.1329722291724666,   -1.296302202745143},
        {0.5896498001385956,    0.7108290355273277},
        {1.1932659329771318,    -1.4260686642502056},
        {1.3840655323404458,    1.0115334006438148},
        {-1.2396722430485902,   -0.9552974953484914},
        {0.27028609573034496,   -1.8448733699177933},
        {1.3306702280952958,    -0.9929165958619169},
        {-1.2332749676969992,   1.1397925874053747},
        {-0.6202783166339042,   1.7712100456331257},
        {-1.6154837997403164,   -0.9523446275118408},
        {0.8685928738735853,    -0.34454980485332065},
        {1.687549520337255,     -0.67294423086464},
        {-0.34190465339827325,  0.7910451759999871},
        {-1.44024232617296,     1.1136096914176599},
        {0.792595328533323,     0.5998546731882844},
        {-0.4695835010533599,   -0.7387241418279447},
        {0.5747806085468721,    -0.6556006867286028},
        {-1.1023455472472774,   1.4381930269516727},
        {0.4715182903788961,    -0.908845411531311},
        {0.9377076451202196,    -0.46859023097938457},
        {0.6636339262772221,    -1.7887074954199607},
        {0.8472231815748102,    -0.4772937468458716},
        {0.9533724366846401,    -0.10537033046171154},
        {-0.20995505007657575,  -0.7363144790737988},
        {-0.8222178936023203,   0.17316103738141697},
        {-1.536996843592765,    -1.0219596006156342},
        {-0.6170314720823177,   -0.6429647841233453},
        {-0.3756844699202588,   1.7957573029027034},
        {-1.6415563916297786,   -0.2823036299471662},
        {-0.8066232996410222,   0.3310444712144414},
        {-0.3899856065089369,   -1.7124265587162846},
        {0.7199382491160348,    0.6101505957952424},
        {0.7280907378043384,    0.39344959450815076},
        {-0.7258376650473913,   0.33871397541986703},
        {0.8689630297207726,    1.5763130799767922},
        {-0.9834911668855596,   -0.22335570579968445},
        {-0.9358857550823566,   -0.059689196800101076},
        {-1.6605938249429697,   -0.025089034589077405},
        {-0.7853053475494755,   0.29244519205499725},
        {-0.7061390621957497,   -1.5969229018781939},
        {0.6318503739207575,    -1.6882971046526285},
        {0.7466023979528398,    -0.0755759982455062},
        {-1.015546795915956,    -1.4991144276601769},
        {0.36309199769164,      -0.8470836309661649},
        {-1.5425293597813936,   0.5671462019085961},
        {-0.5328321562723022,   1.7630720952277725},
        {0.7272907231165585,    -0.34312999673869904},
        {-0.19657029608583712,  -0.9391883258890884},
        {1.6031739485257823,    0.6818797131324155},
        {0.7093179908998413,    0.7398783510122382},
        {-0.11339730039991472,  -0.7552407751009566},
        {-0.132482833878779,    0.9550319768480396},
        {-0.9106379746758276,   -0.20346793722955808},
        {1.4467600277194332,    -0.9211546903913213},
        {1.8685441686919704,    -0.22724585023647753},
        {-0.2011805970537513,   0.8499831188833054},
        {-0.3312155070046436,   -1.8650319317156114},
        {-0.4442947276666981,   1.670837753916144},
        {-0.4817618167427365,   -0.8327916639431729},
        {0.7376528738854768,    0.6698201425535432},
        {-0.45827650181252255,  0.9522468591024352},
        {-0.15141856007739385,  -1.0611481956477342},
        {1.6206057959651619,    -0.02499453993808511},
        {1.8258984494225081,    -0.03408586126547213},
        {-1.2343426516948857,   -1.2023907665197007},
        {-0.4299394213140577,   -0.8841035987339325},
        {-1.2486897693585775,   1.099608978029403},
        {1.8236621629456693,    -0.1651930712047208},
        {1.4996908806572045,    0.7493785526755123},
        {1.2875811892017572,    -1.3008063924755826},
        {-1.2327065029201763,   -1.1934325529483754},
        {0.4142251510374392,    0.5551294934065588},
        {-0.8042699718990951,   -0.2036576217656445},
        {-0.8477281212580038,   -0.49810399602582267},
        {-0.9317365097061684,   -1.573009830047215},
        {0.9954141899432515,    0.11191080303780651},
        {-1.89021296933598,     -0.47199839823385076},
        {-0.7157773248405583,   1.7560455193838562},
        {1.3328270156865551,    -1.3975086665523158},
        {-0.448867132241348,    -0.6286371028948489},
        {0.8551771519629529,    -0.15025885397066757},
        {0.1883759365189358,    0.7283993891473918},
        {-1.8054690157406201,   -0.022974617747978474},
        {-1.5961169894733933,   -1.0671651876239192},
        {0.054663273516050125,  -0.9783532636252766},
        {-0.053732560661006935, 1.603040231083802},
        {-0.7278692511470719,   0.4172341038267035},
        {0.7753407356769781,    -0.38693681605656793},
        {0.6460753725347174,    0.4750367211833773},
        {-0.2752592381957838,   -0.8317908224332837},
        {1.7767789145892463,    -0.030706240539931488},
        {0.4105312730471404,    0.7959748004209877},
        {1.8385162829255426,    0.35092431527275025},
        {0.5100351607849903,    -1.697679987414588},
        {-0.7724399052208998,   -0.47175693405806884},
        {-0.08377758004106053,  -0.9643375948276242},
        {-1.6387866958711075,   -0.7260836039363429},
        {0.5991841813904737,    -0.5384464128241278},
        {-0.9802689115433528,   0.2856607176926759},
        {-0.07056815426358842,  -1.712509176610784},
        {-0.7082962658731529,   -0.5010877896344921},
        {-1.6135977430010906,   -0.0915237333731662},
        {-0.41074665492443657,  -1.8779803494549936},
        {0.07210172420476407,   0.900806511217817},
        {-1.013083540442793,    -1.4846664378255052},
        {1.859596978167556,     0.12267141009011968},
        {-1.2555318254504158,   -1.3293919872327147},
        {-1.7701481212470875,   -0.3772938557797345},
        {0.6630816189201415,    -0.5318798376774296},
        {-0.07613077056679694,  0.9608585221639366},
        {-0.917013253763032,    0.5515984235615402},
        {0.7002210963371553,    0.2804354621998655},
        {-0.11362292457492593,  -1.794561577932898},
        {-1.7178315563912323,   0.09291274396659166},
        {0.5151432775089767,    -0.8624674102506878},
        {-0.42799451229789287,  -0.7632520585627908},
        {1.121635419228318,     1.5149272779249936},
        {-1.3038690857943749,   -1.5129451353488896},
        {0.23734729534131868,   0.8829839155923557},
        {-0.4392546112928487,   -0.8260104976855743},
        {1.3271506983948615,    -0.9884476101604571},
        {-0.4738427634513493,   -1.6810532860889764},
        {1.3639301585066341,    1.125248571521444},
        {1.3241097222906515,    -1.3453799222329683},
        {1.8699775355219361,    0.24283955547082803},
        {-0.4249767142208226,   -1.7881692205229411},
        {0.5000196923638283,    -0.6421970330761598},
        {-0.7251145458054231,   -0.34843661746389404},
        {0.03841922554426365,   0.9269788427886737},
        {1.6031965597189308,    0.21293612210055643},
        {1.2896218182556314,    1.480412441891707},
        {-0.3136006366070601,   -1.9880591500787514},
        {-0.7356561320865264,   -1.6494049970027915},
        {-0.8616629867049007,   -0.0843543357141684},
        {-0.3466600369296351,   0.7242276827386595},
        {-1.3257119709514127,   1.3394605264414308},
        {0.7844120272520397,    1.6025033982099632},
        {0.6268890026331437,    1.686273365532221},
        {-0.8300726276975428,   0.5303919328647585},
        {0.05940422548855813,   0.8647592691688475},
        {-1.8017315678860297,   -0.6028427238858534},
        {-0.13826979817618112,  0.8078143516909618},
        {1.6353858678815316,    0.701278945113061},
        {0.36585701357600275,   1.8457584396543918},
        {0.8084962990075535,    1.4772951131089376},
        {0.7511064785325926,    0.23126436552455526},
        {1.1965720660339938,    1.2532088439003732},
        {0.43875367703894635,   1.802834878850335},
        {-0.7336531646979455,   -0.5225342511594425},
        {1.4763013925616117,    -0.9338351794544539},
        {0.7136586685915968,    0.3133499604988838},
        {0.2662484096389517,    -0.7905854878762077},
        {0.5378483210504299,    0.7103070216079359},
        {-0.07581280909024364,  0.8730146549023239},
        {0.7239779575252164,    0.5994162526498554},
        {-0.908810128524002,    0.3107751706148599},
        {1.8198536529024234,    -0.19370043521248967},
        {0.716445431888193,     -0.4233537543323943},
        {0.7513300745868488,    0.5582657146300982},
        {-1.0291402569253303,   -1.308293512001906},
        {0.5781786206947337,    -0.7621742292830431},
        {-1.446195167456949,    -1.1331565152055465},
        {1.5699460128011398,    0.7922468031947076},
        {-0.7973089449032654,   -1.5376883111173836},
        {0.6487571364129081,    -0.6964199491515423},
        {0.24258188979898615,   -0.8601621612579466},
        {-0.7400171829657355,   0.5299483348788494},
        {0.31179956293345185,   0.809774103483379},
        {-0.8259898519769163,   0.47312790434667285},
        {0.002281589909258107,  -0.9375354235561268},
        {1.4486893149089772,    -0.9344543241782769},
        {0.04539623518728371,   -0.7990078028425421},
        {-1.0271336436327936,   -1.44505099365713},
        {0.2525126175215805,    1.6169666012310244},
        {0.37015366152207424,   1.6899748004026434},
        {1.109921067312107,     1.4238912335311045},
        {-1.3570466025152599,   1.0627549813427872},
        {-0.28056496570298034,  0.7213777573886603},
        {-0.31406105599826756,  -0.6955287972496039},
        {0.9267765415319966,    0.22099435654799907},
        {1.641076847399453,     0.960979651878014},
        {1.6924987533273168,    -0.18446469853778932},
        {1.1821239676860567,    -1.2417304578614266},
        {1.2562670662799766,    -1.3408970391494002},
        {1.6010921469905006,    -0.7265212405935584},
        {-0.23501108247114283,  0.9074953173934359},
        {0.045755133491097354,  -1.6419109902440843},
        {0.5965734807360319,    0.6783267943126364},
        {-1.681423468764255,    -0.07957072987752332},
        {-0.8306804087207845,   -0.25550547792459155},
        {-0.9163845996574629,   0.10419781305002289},
        {0.7468129285254349,    -0.39285194696447284},
        {-0.9497397328759208,   0.3767146157155505},
        {-0.5550670658706206,   0.5879203112117557},
        {-0.5224982971475381,   -0.7069698703944733},
        {-0.6403059313325485,   0.585428022315202},
        {-0.8400869028148975,   0.06735364178402549},
        {1.365438824095418,     -0.9436998571125403},
        {-0.24421546876509104,  -0.8063321896425173},
        {-0.6217880558023061,   0.7780598554791276},
        {1.1694586722694822,    1.4042863046576388},
        {1.8495944564428606,    0.04463295038739844},
        {-1.274056029540979,    1.208926035722476},
        {-1.3593765537416789,   -1.0489397196601922},
        {-0.47015120450203307,  -0.7954023323028979},
        {0.4033221515664416,    1.8651192560439647},
        {-0.9733982827891543,   -0.09696872967231829},
        {-0.41353743777573043,  -1.7236773871398103},
        {-1.6987837040546283,   0.5632584128304413},
        {-0.7257971376395764,   -1.494782987482454},
        {-1.2237414866089245,   -1.2372526020004004},
        {0.4512948909183599,    0.7703731911938642},
        {-0.6620536768455525,   0.729865326349718},
        {0.8291234376296308,    0.26352991819960636},
        {-1.398481208460391,    -1.1000152708792126},
        {-0.82122267045074,     1.7552990982680674},
        {-0.9525409334927902,   -0.1116080300790966},
        {1.1627015654461232,    -1.4211403460754775},
        {1.81000506789075,      0.5007508584319968},
        {-0.1181185188130827,   1.8000877782352223},
        {-0.6842565322304445,   -0.5422689668314719},
        {1.731530065717539,     0.0644821440733667},
        {-1.6488803889752905,   -0.6418358696272386},
        {-0.8398260564161637,   -0.037234295356736447},
        {0.5249607620386496,    0.7290327813723396},
        {-0.8596093064832454,   -0.4454388843823997},
        {-0.11904175166586281,  0.9512389602480478},
        {1.5930403089268321,    -0.7562838089131029},
        {-1.756131254194635,    0.48307379414475893},
        {0.8169634858176698,    -0.41804698044661925},
        {-0.8761151350855848,   0.13294101304911532},
        {-0.31755959046018745,  -0.970808830258246},
        {0.021246300798679618,  -0.9250482494671362},
        {-0.8104416085005134,   -0.2614615742698596},
        {-0.5139160567008876,   0.7381535043640615},
        {-1.143378186353952,    -1.2318529678299974},
        {-0.4096191322648946,   1.6255841722053934},
        {1.5754346142034754,    0.5206435097558472},
        {0.19421291934819565,   -1.751589068503901},
        {0.7687883919154022,    0.781890036037896},
        {1.7481423545505672,    0.7035118370688854},
        {0.7008750553092545,    -1.6647358409655235},
        {1.149151687615931,     -1.3456647471076464},
        {1.7832975340854977,    0.009462905091116061},
        {0.465604735798541,     1.6630373023706537},
        {1.5603067317046397,    -0.7875882949341807},
        {-0.2136418884457299,   1.854230502425764},
        {-0.8855714679628734,   0.01643284392171146},
        {0.3495896176691054,    0.9915415390852014},
        {1.500274983885805,     -1.087345896785945},
        {0.323120286142052,     -1.7834082787383683},
        {-0.7353976806158034,   1.7400306646157206},
        {-0.7896780217608466,   -0.24243740572588393},
        {-0.6737413313530762,   -1.6371866215769573},
        {-1.6276441161852493,   -0.8412936157719308},
        {0.6920479623838837,    -0.11593041769955036},
        {-1.7672930102737465,   0.42612044314692454},
        {-0.6112108911284253,   1.665740688482972},
        {1.6266656107619397,    -0.4783539765409309},
        {-0.1183042838117454,   -1.813819816020563},
        {-0.05465838071352791,  1.9244439609921027},
        {1.3981532710512425,    -0.8991677071124654},
        {-0.904589038770348,    -1.4873968532345443},
        {-0.05531316193679505,  0.8805248370338669},
        {-1.435131641891589,    1.190516057760702},
        {0.05055659284858103,   0.8140304601632152},
        {0.9865550027885603,    -0.19255736487279498},
        {-0.874075452970872,    -0.10164127415592089},
        {-0.7225388094437887,   -0.7058794400503472},
        {0.016777248310457633,  -0.9005878130822993},
        {-0.8275184436494315,   0.17651520399190954},
        {-0.03349026080433691,  1.7960430409673322},
        {0.3138775786162085,    -0.7353243081175509},
        {0.43222251652246046,   -0.726763914865163},
        {0.2617222348859123,    -0.945953645445422},
        {1.615430944833001,     -0.22426329376796103},
        {0.6670249609347891,    1.6370204392832417},
        {0.8580736992569137,    0.27966537013474063},
        {-2.0226112710370487,   -0.36659699648589406},
        {0.8404687295780051,    0.13945602577942212},
        {0.9701755802582894,    -1.663958138107189},
        {0.8650895752163945,    0.14464310933124228},
        {-1.3078334024457192,   -1.2253684016143926},
        {1.388284813199568,     0.8832525068556051},
        {-0.19642867933019217,  0.9756302219550628},
        {0.2142979062038431,    1.8520561641395912},
};
