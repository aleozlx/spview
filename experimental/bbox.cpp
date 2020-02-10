#include <fstream>
#define SPT_DEBUG
#include "aabb.h"
#include "geometry.h"

using namespace spt::geo;

struct BBox: public Box2D<float> {
    unsigned ptr = 0;
    unsigned char marking[2] = {0xAB, 0xBA};

    BBox() = default;

    BBox(const Box2D<float> &box): Box2D<float>(box) { // NOLINT
        // This constructor is needed for implicit down-casting
    }

    BBox(float _xmin, float _ymin, float _xmax, float _ymax):
            Box2D<float>{_xmin, _ymin, _xmax, _ymax} {

    }
};

typedef AABBTree<BBox> BBoxTree;
typedef AABBTreeRO<BBox> BBoxIndex;

int main(int, char **) {
    BBox aabb[] = {
            {0,  0,  10, 20},
            {20, 30, 30, 50},
            {25, 35, 40, 55}
    };

    BBoxTree t;
    for (const auto &b : aabb) {
        t.Insert(b);
        t.Debug();
        std::cout << "======" << std::endl;
    }

    BBox z = {5, 5, 10, 20};
    auto x1 = t && z;
    for (const auto &b: x1) {
        std::cout << "box " << b << std::endl;
        std::cout << "======" << std::endl;
    }

    Vector2D<float> v = {6, 6};
    auto x2 = t && v;
    for (const auto &b: x2) {
        std::cout << "box " << b << std::endl;
        std::cout << "======" << std::endl;
    }

    {
        std::cout << "Saving tree..." << std::endl;
        BBoxIndex t2(t);
        std::fstream fout;
        fout.open("aabbtree_test.data", std::fstream::out);
        t2.Save(fout);
        fout.close();
    }

    {
        std::cout << "Loading tree..." << std::endl;
        std::fstream fin;
        fin.open("aabbtree_test.data");
        auto t3 = BBoxIndex::Load(fin);
        fin.close();

        for (const auto &b: t3 && z) {
            std::cout << "box " << b << std::endl;
            std::cout << "======" << std::endl;
        }
        for (const auto &b: t3 && v) {
            std::cout << "box " << b << std::endl;
            std::cout << "======" << std::endl;
        }
    }


}
