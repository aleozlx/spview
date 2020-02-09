#pragma once

#define AABB_NULL (UINT_MAX)

namespace spt::geo {
    template<typename T>
    struct Vector2D {
        T x = 0, y = 0;
    };

    template<typename T>
    class Box2D {
    protected:
        typedef Box2D<T> Self;
    public:
        typedef T Metric;

        T xmin = 0, ymin = 0, xmax = 0, ymax = 0;

        inline T Area() const {
            return Width() * Height();
        }

        inline T Width() const {
            return xmax - xmin;
        }

        inline T Height() const {
            return ymax - ymin;
        }

        bool operator&&(const Self &other) const {
            return xmax > other.xmin &&
                   xmin < other.xmax &&
                   ymax > other.ymin &&
                   ymin < other.ymax;
        }

        bool operator&&(const Vector2D<T> &p) const {
            return xmax >= p.x &&
                   xmin <= p.x &&
                   ymax >= p.y &&
                   ymin <= p.y;
        }

        Self operator+(const Self &other) const {
            return {
                    std::min(xmin, other.xmin), std::min(ymin, other.ymin),
                    std::max(xmax, other.xmax), std::max(ymax, other.ymax)
            };
        }

        friend std::ostream &operator<<(std::ostream &os, const Self &box) {
            os << box.xmin << " " << box.ymin << " " << box.xmax << " " << box.ymax;
            return os;
        }
    };

    template<typename BoxType>
    class AABBTreeNode {
    public:
        unsigned parent = AABB_NULL,
                left = AABB_NULL,
                right = AABB_NULL,
                next = AABB_NULL;

        AABBTreeNode() = default;

        AABBTreeNode(const BoxType &box) { // NOLINT
            SetBox(box);
        }

        bool IsLeaf() const {
            return left == AABB_NULL;
        }

        void AssignBox(const AABBTreeNode<BoxType> &_node) {
            SetBox(_node.box);
        }

        void SetBox(const BoxType &_box) {
            this->box = _box;
            this->area = box.Area();
        }

        inline const BoxType &GetBox() const {
            return box;
        }

        inline typename BoxType::Metric Area() const {
            return area;
        }

        inline const BoxType &operator*() const {
            return box;
        }

    protected:
        BoxType box;
        typename BoxType::Metric area = 0;
    };

    template<typename B>
    struct AABBTreeRO;

    template<typename BoxType>
    class AABBTree {
        friend struct AABBTreeRO<BoxType>;
    protected:
        typedef AABBTreeNode<BoxType> NodeType;
        std::vector<NodeType> _nodes;
        unsigned _rootNodeIndex;
        unsigned _allocatedNodeCount;
        unsigned _nextFreeNodeIndex;
        unsigned _nodeCapacity;

        unsigned allocateNode() {
            // if we have no free tree nodes then grow the pool
            if (_nextFreeNodeIndex == AABB_NULL) {
                assert(_allocatedNodeCount == _nodeCapacity);
                _nodeCapacity += _nodeCapacity < 3 ? 1 : _nodeCapacity / 3;
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
            NodeType *leafNode = &_nodes[leafNodeIndex];
            while (!_nodes[treeNodeIndex].IsLeaf()) {
                // because of the test in the while loop above we know we are never a leaf inside it
                const NodeType &treeNode = _nodes[treeNodeIndex];
                unsigned left = treeNode.left;
                unsigned right = treeNode.right;
                const NodeType &leftNode = _nodes[left];
                const NodeType &rightNode = _nodes[right];

                auto combinedAabb = *treeNode + **leafNode;

                typename BoxType::Metric newParentNodeCost = 2 * combinedAabb.Area();
                typename BoxType::Metric minimumPushDownCost = 2 * (combinedAabb.Area() - treeNode.Area());

                // use the costs to figure out whether to create a new parent here or descend
                typename BoxType::Metric costLeft, costRight;
                if (leftNode.IsLeaf())
                    costLeft = (**leafNode + *leftNode).Area() + minimumPushDownCost;
                else {
                    BoxType newLeftAabb = **leafNode + *leftNode;
                    costLeft = (newLeftAabb.Area() - leftNode.Area()) + minimumPushDownCost;
                }
                if (rightNode.IsLeaf())
                    costRight = (**leafNode + *rightNode).Area() + minimumPushDownCost;
                else {
                    BoxType newRightAabb = **leafNode + *rightNode;
                    costRight = (newRightAabb.Area() - rightNode.Area()) + minimumPushDownCost;
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
            unsigned newParentIndex = allocateNode();
            leafNode = &_nodes[leafNodeIndex]; // update the ptr after potential reallocation
            NodeType &leafSibling = _nodes[leafSiblingIndex];
            unsigned oldParentIndex = leafSibling.parent;
            NodeType &newParent = _nodes[newParentIndex];
            newParent.parent = oldParentIndex;
            newParent.AssignBox(**leafNode + *leafSibling);
            newParent.left = leafSiblingIndex;
            newParent.right = leafNodeIndex;
            leafNode->parent = newParentIndex;
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
            treeNodeIndex = leafNode->parent;
            fixUpwardsTree(treeNodeIndex);
        }

        void fixUpwardsTree(unsigned treeNodeIndex) {
            while (treeNodeIndex != AABB_NULL) {
                NodeType &treeNode = _nodes[treeNodeIndex];

                // every node should be a parent
                assert(treeNode.left != AABB_NULL && treeNode.right != AABB_NULL);

                // fix height and area
                const NodeType &leftNode = _nodes[treeNode.left];
                const NodeType &rightNode = _nodes[treeNode.right];
                treeNode.AssignBox(*leftNode + *rightNode);

                treeNodeIndex = treeNode.parent;
            }
        }

    public:
        typedef BoxType BoxType;

        explicit AABBTree(unsigned initialSize = 1) : _rootNodeIndex(AABB_NULL), _allocatedNodeCount(0),
                                                      _nextFreeNodeIndex(0), _nodeCapacity(initialSize) {
            if (initialSize == 0) initialSize = 1;
            _nodes.resize(initialSize);
            for (unsigned nodeIndex = 0; nodeIndex < initialSize; nodeIndex++) {
                NodeType &node = _nodes[nodeIndex];
                node.next = nodeIndex + 1;
            }
            _nodes[initialSize - 1].next = AABB_NULL;
        }

        inline size_t Count() {
            return _allocatedNodeCount;
        }

        void Insert(const BoxType &box) {
            unsigned nodeIndex = allocateNode();
            _nodes[nodeIndex].SetBox(box);
            insertLeaf(nodeIndex);
        }

        template<typename TreeType, typename Geometry>
        friend std::vector<typename TreeType::BoxType> QueryAABBTree(const TreeType *tree, const Geometry &g); // NOLINT

        template<typename Geometry>
        inline std::vector<BoxType> operator&&(const Geometry &g) const {
            return QueryAABBTree(this, g);
        }
    };

    template<typename B>
    struct AABBTreeRO {
        typedef AABBTreeNode<B> NodeType;
        typedef B BoxType;
        const unsigned _rootNodeIndex;
        const NodeType *const _nodes;

        AABBTreeRO(unsigned root, const void *data) : _rootNodeIndex(root),
                                                      _nodes(reinterpret_cast<const NodeType *>(data)) {
        }

        explicit AABBTreeRO(const AABBTree<B> &tree) : AABBTreeRO(tree._rootNodeIndex, tree._nodes.data()) { // NOLINT
        }

        template<typename TreeType, typename Geometry>
        friend std::vector<typename TreeType::BoxType> QueryAABBTree(const TreeType *tree, const Geometry &g); // NOLINT

        template<typename Geometry>
        inline std::vector<B> operator&&(const Geometry &g) const {
            return QueryAABBTree(this, g);
        }

        void Save(std::ostream &s) {
            // TODO impl serialization
        }

        void Load(std::istream &s) {
            // TODO impl deserialization
        }
    };

    template<typename TreeType, typename Geometry>
    std::vector<typename TreeType::BoxType> QueryAABBTree(const TreeType *tree, const Geometry &g) {
        std::vector<typename TreeType::BoxType> overlaps;
        std::stack<unsigned> stack;
        stack.push(tree->_rootNodeIndex);
        while (!stack.empty()) {
            unsigned nodeIndex = stack.top();
            stack.pop();
            if (nodeIndex == AABB_NULL) continue;
            const auto &node = tree->_nodes[nodeIndex];
            if (node.GetBox() && g) {
                if (node.IsLeaf())
                    overlaps.push_back(*node);
                else {
                    stack.push(node.left);
                    stack.push(node.right);
                }
            }
        }
        return overlaps;
    }

    typedef Box2D<double> Box2Dd;
    typedef Box2D<float> Box2Df;
    typedef Box2D<int> Box2Di;
    extern template struct Vector2D<double>;
    extern template struct Vector2D<float>;
    extern template struct Vector2D<int>;
    extern template class Box2D<double>;
    extern template class Box2D<float>;
    extern template class Box2D<int>;
    extern template class AABBTree<Box2Dd>;
    extern template struct AABBTreeRO<Box2Dd>;
    extern template class AABBTree<Box2Df>;
    extern template struct AABBTreeRO<Box2Df>;
    extern template class AABBTree<Box2Di>;
    extern template struct AABBTreeRO<Box2Di>;
}