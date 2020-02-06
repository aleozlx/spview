#include "AABBTree.h"

template<typename T>
struct Box {
    T xmin, ymin, xmax, ymax;

    Box(): Box(0, 0, 0, 0) {

    }

    Box(T _xmin, T _ymin, T _xmax, T _ymax) {
        xmin = _xmin;
        ymin = _ymin;
        xmax = _xmax;
        ymax = _ymax;
    }
};

int main(int, char **) {

}
