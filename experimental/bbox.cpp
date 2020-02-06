#include <memory>
#include <vector>
#include <map>
#include <forward_list>
#include <algorithm>

template<typename T>
struct Box2D {
    T xmin, ymin, xmax, ymax;

    Box2D() : Box2D(0, 0, 0, 0) {

    }

    Box2D(T _xmin, T _ymin, T _xmax, T _ymax) {
        xmin = _xmin;
        ymin = _ymin;
        xmax = _xmax;
        ymax = _ymax;
    }

    inline T Area() const {
        return Width() * Height();
    }

    inline T Width() const {
        return xmax - xmin;
    }

    inline T Height() const {
        return ymax - ymin;
    }
};

const unsigned AABB_NULL = 0xffffffff;

template<typename BoxType, typename AreaType>
struct AABBTreeNode {
    BoxType box;
    AreaType area;
    unsigned parent = AABB_NULL,
            left = AABB_NULL,
            right = AABB_NULL,
            next = AABB_NULL;

    [[nodiscard]] bool isLeaf() const {
        return leftNodeIndex == AABB_NULL;
    }
};

template<typename NodeType>
class AABBTree {
protected:
    std::vector<NodeType> _nodes;
    unsigned _rootNodeIndex;
    unsigned _allocatedNodeCount;
    unsigned _nextFreeNodeIndex;
    unsigned _nodeCapacity;
    unsigned _growthSize;

    unsigned allocateNode();

    void deallocateNode(unsigned nodeIndex);

    void insertLeaf(unsigned leafNodeIndex);

    void removeLeaf(unsigned leafNodeIndex);

//    void updateLeaf(unsigned leafNodeIndex, const AABB &newAaab);

    void fixUpwardsTree(unsigned treeNodeIndex);

public:
    AABBTree(unsigned initialSize);

    ~AABBTree();

//    void insertObject(const std::shared_ptr<IAABB> &object);
//
//    void removeObject(const std::shared_ptr<IAABB> &object);
//
//    std::forward_list<std::shared_ptr<IAABB>> queryOverlaps(const std::shared_ptr<IAABB> &object) const;
};

int main(int, char **) {

}
