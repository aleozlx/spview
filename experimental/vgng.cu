#include <iostream>
#include <thrust/host_vector.h>
#include <thrust/device_vector.h>

void v_network_ctor(size_t N_UNITS, size_t DIM_DATA) {
    thrust::host_vector<int> _W(N_UNITS, DIM_DATA);
}

int main(int, char**) {
    v_network_ctor(5, 5);
    return 0;
}
