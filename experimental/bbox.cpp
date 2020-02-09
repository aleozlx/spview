#include <iostream>
#include <memory>
#include <vector>
#include <map>
#include <forward_list>
#include <algorithm>
#include <cassert>

template<typename T>
struct Box2D {
    typedef T Metric;
    typedef Box2D<T> Self;

    T xmin = 0, ymin = 0, xmax = 0, ymax = 0;

    Box2D() = default;

    Box2D(T _xmin, T _ymin, T _xmax, T _ymax) {
        xmin = _xmin;
        ymin = _ymin;
        xmax = _xmax;
        ymax = _ymax;
    }

    [[nodiscard]] inline T Area() const {
        return Width() * Height();
    }

    [[nodiscard]] inline T Width() const {
        return xmax - xmin;
    }

    [[nodiscard]] inline T Height() const {
        return ymax - ymin;
    }

    [[nodiscard]] bool overlaps(const Self& other) const
    {
        return xmax > other.xmin &&
               xmin < other.xmax &&
               ymax > other.ymin &&
               ymin < other.ymax;
    }

    [[nodiscard]] bool contains(const Self& other) const
    {
        return other.xmin >= xmin &&
               other.xmax <= xmax &&
               other.ymin >= ymin &&
               other.ymax <= ymax;
    }

    [[nodiscard]] Self merge(const Self& other) const
    {
        return Self(
                std::min(xmin, other.xmin), std::min(ymin, other.ymin),
                std::max(xmax, other.xmax), std::max(ymax, other.ymax)
        );
    }

    [[nodiscard]] Self intersection(const Self& other) const
    {
        return Self(
                std::max(xmin, other.xmin), std::max(ymin, other.ymin),
                std::min(xmax, other.xmax), std::min(ymax, other.ymax)
        );
    }
};

const unsigned AABB_NULL = 0xffffffff;

template<typename B>
struct AABBTreeNode {
    typedef B BoxType;
    BoxType box;
    typename BoxType::Metric area;
    unsigned parent = AABB_NULL,
            left = AABB_NULL,
            right = AABB_NULL,
            next = AABB_NULL;

    AABBTreeNode() = default;

    AABBTreeNode(const B &box) { // NOLINT
        this->area = box.Area();
    }

    [[nodiscard]] bool isLeaf() const {
        return left == AABB_NULL;
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

    unsigned allocateNode() {
        // if we have no free tree nodes then grow the pool
        if (_nextFreeNodeIndex == AABB_NULL) {
            assert(_allocatedNodeCount == _nodeCapacity);

            _nodeCapacity += _growthSize;
            _nodes.resize(_nodeCapacity);
            for (unsigned nodeIndex = _allocatedNodeCount; nodeIndex < _nodeCapacity; nodeIndex++) {
                NodeType &node = _nodes[nodeIndex];
                node.next = nodeIndex + 1;
            }
            _nodes[_nodeCapacity - 1].next = AABB_NULL;
            _nextFreeNodeIndex = _allocatedNodeCount;
        }

        unsigned nodeIndex = _nextFreeNodeIndex;
        NodeType &allocatedNode = _nodes[nodeIndex];
        allocatedNode.parent = AABB_NULL;
        allocatedNode.left = AABB_NULL;
        allocatedNode.right = AABB_NULL;
        _nextFreeNodeIndex = allocatedNode.next;
        _allocatedNodeCount++;

        return nodeIndex;
    }


    void deallocateNode(unsigned nodeIndex) {
        AABBNode &deallocatedNode = _nodes[nodeIndex];
        deallocatedNode.next = _nextFreeNodeIndex;
        _nextFreeNodeIndex = nodeIndex;
        _allocatedNodeCount--;
    }

    void insertLeaf(unsigned leafNodeIndex) {
        // make sure we're inserting a new leaf
        assert(_nodes[leafNodeIndex].parent == AABB_NULL);
        assert(_nodes[leafNodeIndex].left == AABB_NULL);
        assert(_nodes[leafNodeIndex].right == AABB_NULL);

        // if the tree is empty then we make the root the leaf
        if (_rootNodeIndex == AABB_NULL) {
            _rootNodeIndex = leafNodeIndex;
            return;
        }

        // search for the best place to put the new leaf in the tree
        // we use surface area and depth as search heuristics
        unsigned treeNodeIndex = _rootNodeIndex;
        NodeType &leafNode = _nodes[leafNodeIndex];
        while (!_nodes[treeNodeIndex].isLeaf()) {
            // because of the test in the while loop above we know we are never a leaf inside it
            const NodeType &treeNode = _nodes[treeNodeIndex];
            unsigned left = treeNode.left;
            unsigned right = treeNode.right;
            const NodeType &leftNode = _nodes[left];
            const NodeType &rightNode = _nodes[right];

            auto combinedAabb = treeNode.box.merge(leafNode.box);

            float newParentNodeCost = 2.0f * combinedAabb.Area();
            float minimumPushDownCost = 2.0f * (combinedAabb.Area() - treeNode.box.Area());

            // use the costs to figure out whether to create a new parent here or descend
            float costLeft;
            float costRight;
            if (leftNode.isLeaf())
                costLeft = leafNode.box.merge(leftNode.box).Area() + minimumPushDownCost;
            else {
                typename NodeType::BoxType newLeftAabb = leafNode.box.merge(leftNode.box);
                costLeft = (newLeftAabb.Area() - leftNode.box.Area()) + minimumPushDownCost;
            }
            if (rightNode.isLeaf())
                costRight = leafNode.box.merge(rightNode.box).Area() + minimumPushDownCost;
            else {
                typename NodeType::BoxType newRightAabb = leafNode.box.merge(rightNode.box);
                costRight = (newRightAabb.Area() - rightNode.box.Area()) + minimumPushDownCost;
            }

            // if the cost of creating a new parent node here is less than descending in either direction then
            // we know we need to create a new parent node, errrr, here and attach the leaf to that
            if (newParentNodeCost < costLeft && newParentNodeCost < costRight)
                break;

            // otherwise descend in the cheapest direction
            if (costLeft < costRight)
                treeNodeIndex = left;
            else
                treeNodeIndex = right;

        }

        // the leafs sibling is going to be the node we found above and we are going to create a new
        // parent node and attach the leaf and this item
        unsigned leafSiblingIndex = treeNodeIndex;
        NodeType &leafSibling = _nodes[leafSiblingIndex];
        unsigned oldParentIndex = leafSibling.parent;
        unsigned newParentIndex = allocateNode();
        NodeType &newParent = _nodes[newParentIndex];
        newParent.parent = oldParentIndex;
        newParent.box = leafNode.box.merge(
                leafSibling.box); // the new parents box is the leaf box combined with it's siblings box
        newParent.left = leafSiblingIndex;
        newParent.right = leafNodeIndex;
        leafNode.parent = newParentIndex;
        leafSibling.parent = newParentIndex;

        if (oldParentIndex == AABB_NULL) {
            // the old parent was the root and so this is now the root
            _rootNodeIndex = newParentIndex;
        } else {
            // the old parent was not the root and so we need to patch the left or right index to
            // point to the new node
            NodeType &oldParent = _nodes[oldParentIndex];
            if (oldParent.left == leafSiblingIndex)
                oldParent.left = newParentIndex;
            else
                oldParent.right = newParentIndex;
        }

        // finally we need to walk back up the tree fixing heights and areas
        treeNodeIndex = leafNode.parent;
        fixUpwardsTree(treeNodeIndex);
    }

    void removeLeaf(unsigned leafNodeIndex) {
        // if the leaf is the root then we can just clear the root pointer and return
        if (leafNodeIndex == _rootNodeIndex) {
            _rootNodeIndex = AABB_NULL;
            return;
        }

        AABBNode &leafNode = _nodes[leafNodeIndex];
        unsigned parent = leafNode.parent;
        const AABBNode &parentNode = _nodes[parent];
        unsigned grandParentNodeIndex = parentNode.parent;
        unsigned siblingNodeIndex =
                parentNode.left == leafNodeIndex ? parentNode.right : parentNode.left;
        assert(siblingNodeIndex != AABB_NULL); // we must have a sibling
        AABBNode &siblingNode = _nodes[siblingNodeIndex];

        if (grandParentNodeIndex != AABB_NULL) {
            // if we have a grand parent (i.e. the parent is not the root) then destroy the parent and connect the sibling to the grandparent in its
            // place
            AABBNode &grandParentNode = _nodes[grandParentNodeIndex];
            if (grandParentNode.left == parent) {
                grandParentNode.left = siblingNodeIndex;
            } else {
                grandParentNode.right = siblingNodeIndex;
            }
            siblingNode.parent = grandParentNodeIndex;
            deallocateNode(parent);

            fixUpwardsTree(grandParentNodeIndex);
        } else {
            // if we have no grandparent then the parent is the root and so our sibling becomes the root and has it's parent removed
            _rootNodeIndex = siblingNodeIndex;
            siblingNode.parent = AABB_NULL;
            deallocateNode(parent);
        }

        leafNode.parent = AABB_NULL;
    }

//    void updateLeaf(unsigned leafNodeIndex, const AABB &newAaab);

    void fixUpwardsTree(unsigned treeNodeIndex) {
        while (treeNodeIndex != AABB_NULL) {
            NodeType &treeNode = _nodes[treeNodeIndex];

            // every node should be a parent
            assert(treeNode.left != AABB_NULL && treeNode.right != AABB_NULL);

            // fix height and area
            const NodeType &leftNode = _nodes[treeNode.left];
            const NodeType &rightNode = _nodes[treeNode.right];
            treeNode.box = leftNode.box.merge(rightNode.box);

            treeNodeIndex = treeNode.parent;
        }
    }

public:
    explicit AABBTree(unsigned initialSize) : _rootNodeIndex(AABB_NULL), _allocatedNodeCount(0),
                                              _nextFreeNodeIndex(0), _nodeCapacity(initialSize),
                                              _growthSize(initialSize) {
        _nodes.resize(initialSize);
        for (unsigned nodeIndex = 0; nodeIndex < initialSize; nodeIndex++) {
            NodeType &node = _nodes[nodeIndex];
            node.next = nodeIndex + 1;
        }
        _nodes[initialSize - 1].next = AABB_NULL;
    }

    ~AABBTree() =
    default;

    void Insert(const std::shared_ptr<NodeType> &node) {
        unsigned nodeIndex = allocateNode();
        insertLeaf(nodeIndex);
//        _objectNodeIndexMap[object] = nodeIndex;
    }

    void Remove(const std::shared_ptr<NodeType> &node) {
        unsigned nodeIndex = _objectNodeIndexMap[object];
        removeLeaf(nodeIndex);
        deallocateNode(nodeIndex);
        _objectNodeIndexMap.erase(object);
    }

    std::forward_list<std::shared_ptr<NodeType>> queryOverlaps(const std::shared_ptr<NodeType> &object) const;

    void Debug() {
        auto p = this->_rootNodeIndex;
        while(p != AABB_NULL) {
            const auto &box = _nodes[p].box;
            std::cout<<"box@"<<p<<" "<<box.xmin<<" "<<box.ymin<<" "<<box.xmax<<" "<<box.ymax<<std::endl;
            p =  _nodes[p].next;
        }
    }
};

typedef Box2D<float> Box2Df;

int main(int, char **) {
    Box2Df a(0, 0, 100, 200),
            b(50, 60, 300, 400);

    AABBTree<AABBTreeNode<Box2Df>> t(2);
    t.Insert(std::make_shared<AABBTreeNode<Box2Df>>(a));
    t.Insert(std::make_shared<AABBTreeNode<Box2Df>>(b));
    t.Debug();
}
