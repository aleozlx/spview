#include <iostream>
#include <vector>
#include <stack>
#include <algorithm>
#include <cassert>
#include <limits>
#include "bbox.h"

namespace spt::geo {
    template struct Vector2D<double>;
    template struct Vector2D<float>;
    template struct Vector2D<int>;
    template class Box2D<double>;
    template class Box2D<float>;
    template class Box2D<int>;
    template class AABBTree<Box2Dd>;
    template struct AABBTreeRO<Box2Dd>;
    template class AABBTree<Box2Df>;
    template struct AABBTreeRO<Box2Df>;
    template class AABBTree<Box2Di>;
    template struct AABBTreeRO<Box2Di>;
}
