#pragma once
#include <iostream>
#include <vector>
#include <stack>
#include <algorithm>
#include <cassert>
#include <limits>

namespace spt::geo {
    template<typename BoxType>
    class AABBTreeNode {
    public:
        static const unsigned NIL = UINT_MAX;
        unsigned parent = NIL,
                left = NIL,
                right = NIL,
                next = NIL;

        AABBTreeNode() = default;

        AABBTreeNode(const BoxType &box) { // NOLINT
            SetBox(box);
        }

        bool IsLeaf() const {
            return left == NIL;
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
    class AABBTreeRO;

    template<typename BoxType>
    class AABBTree {
        friend class AABBTreeRO<BoxType>;

    protected:
        typedef AABBTreeNode<BoxType> NodeType;
        std::vector<NodeType> nodes;
        unsigned root;
        unsigned allocatedNodeCount;
        unsigned nextFreeNodeIndex;
        unsigned nodeCapacity;

        unsigned allocateNode() {
            // if we have no free tree nodes then grow the pool
            if (nextFreeNodeIndex == NodeType::NIL) {
                assert(allocatedNodeCount == nodeCapacity);
                nodeCapacity += nodeCapacity < 3 ? 1 : nodeCapacity / 3;
                nodes.resize(nodeCapacity);
                for (unsigned nodeIndex = allocatedNodeCount; nodeIndex < nodeCapacity; nodeIndex++) {
                    NodeType &node = nodes[nodeIndex];
                    node.next = nodeIndex + 1;
                }
                nodes[nodeCapacity - 1].next = NodeType::NIL;
                nextFreeNodeIndex = allocatedNodeCount;
            }

            unsigned nodeIndex = nextFreeNodeIndex;
            NodeType &allocatedNode = nodes[nodeIndex];
            allocatedNode.parent = NodeType::NIL;
            allocatedNode.left = NodeType::NIL;
            allocatedNode.right = NodeType::NIL;
            nextFreeNodeIndex = allocatedNode.next;
            allocatedNodeCount++;
            return nodeIndex;
        }

        void insertLeaf(unsigned leafNodeIndex) {
            // make sure we're inserting a new leaf
            assert(nodes[leafNodeIndex].parent == NodeType::NIL);
            assert(nodes[leafNodeIndex].left == NodeType::NIL);
            assert(nodes[leafNodeIndex].right == NodeType::NIL);

            // if the tree is empty then we make the root the leaf
            if (root == NodeType::NIL) {
                root = leafNodeIndex;
                return;
            }

            // search for the best place to put the new leaf in the tree
            // we use surface area and depth as search heuristics
            unsigned treeNodeIndex = root;
            NodeType *leafNode = &nodes[leafNodeIndex];
            while (!nodes[treeNodeIndex].IsLeaf()) {
                // because of the test in the while loop above we know we are never a leaf inside it
                const NodeType &treeNode = nodes[treeNodeIndex];
                unsigned left = treeNode.left;
                unsigned right = treeNode.right;
                const NodeType &leftNode = nodes[left];
                const NodeType &rightNode = nodes[right];

                auto combinedAabb = *treeNode + **leafNode;

                typename BoxType::Metric newParentNodeCost = 2.0f * combinedAabb.Area();
                typename BoxType::Metric minimumPushDownCost = 2.0f * (combinedAabb.Area() - treeNode.Area());

                // use the costs to figure out whether to create a new parent here or descend
                typename BoxType::Metric costLeft, costRight;
                if (leftNode.IsLeaf())
                    costLeft = (**leafNode + *leftNode).Area() + minimumPushDownCost;
                else {
                    auto newLeftAabb = **leafNode + *leftNode;
                    costLeft = (newLeftAabb.Area() - leftNode.Area()) + minimumPushDownCost;
                }
                if (rightNode.IsLeaf())
                    costRight = (**leafNode + *rightNode).Area() + minimumPushDownCost;
                else {
                    auto newRightAabb = **leafNode + *rightNode;
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
            leafNode = &nodes[leafNodeIndex]; // update the ptr after potential reallocation
            NodeType &leafSibling = nodes[leafSiblingIndex];
            unsigned oldParentIndex = leafSibling.parent;
            NodeType &newParent = nodes[newParentIndex];
            newParent.parent = oldParentIndex;
            newParent.SetBox(**leafNode + *leafSibling);
            newParent.left = leafSiblingIndex;
            newParent.right = leafNodeIndex;
            leafNode->parent = newParentIndex;
            leafSibling.parent = newParentIndex;

            if (oldParentIndex == NodeType::NIL) {
                // the old parent was the root and so this is now the root
                root = newParentIndex;
            } else {
                // the old parent was not the root and so we need to patch the left or right index to
                // point to the new node
                NodeType &oldParent = nodes[oldParentIndex];
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
            while (treeNodeIndex != NodeType::NIL) {
                NodeType &treeNode = nodes[treeNodeIndex];

                // every node should be a parent
                assert(treeNode.left != NodeType::NIL && treeNode.right != NodeType::NIL);

                // fix height and area
                const NodeType &leftNode = nodes[treeNode.left];
                const NodeType &rightNode = nodes[treeNode.right];
                treeNode.SetBox(*leftNode + *rightNode);

                treeNodeIndex = treeNode.parent;
            }
        }

    public:
        typedef BoxType BoxType;

        explicit AABBTree(unsigned initialSize = 1) : root(NodeType::NIL), allocatedNodeCount(0),
                                                      nextFreeNodeIndex(0), nodeCapacity(initialSize) {
            if (initialSize == 0) initialSize = 1;
            nodes.resize(initialSize);
            for (unsigned nodeIndex = 0; nodeIndex < initialSize; nodeIndex++) {
                NodeType &node = nodes[nodeIndex];
                node.next = nodeIndex + 1;
            }
            nodes[initialSize - 1].next = NodeType::NIL;
        }

        inline size_t Count() {
            return allocatedNodeCount;
        }

        void Insert(const BoxType &box) {
            unsigned nodeIndex = allocateNode();
            nodes[nodeIndex].SetBox(box);
            insertLeaf(nodeIndex);
        }

        template<typename TreeType, typename Geometry>
        friend std::vector<typename TreeType::BoxType> QueryAABBTree(const TreeType *tree, const Geometry &g); // NOLINT

        template<typename Geometry>
        inline std::vector<BoxType> operator&&(const Geometry &g) const {
            return QueryAABBTree(this, g);
        }
#ifdef SPT_DEBUG
        void Debug() const {
        std::stack<unsigned> s;
        s.push(this->root);
        while (!s.empty()) {
            unsigned p = s.top();
            s.pop();
            if (p == NodeType::NIL)
                continue;
            const auto &node = nodes[p];
            const auto &box = *node;
            std::cout << (node.IsLeaf() ? "box@" : "node@") << p << " " << box << std::endl;
            if (!node.IsLeaf()) {
                s.push(node.left);
                s.push(node.right);
            }
        }
    }
#endif
    };

    template<typename B>
    class AABBTreeRO {
    protected:
        struct AABBTreeROHeader {
            unsigned root = 0;
            size_t count = 0;
        };
        typedef AABBTreeNode<B> NodeType;
        typedef B BoxType;
        union { // NOLINT
            unsigned root; // alias root=header.root
            AABBTreeROHeader header;
        };

        const NodeType *nodes;
        bool ownData;

    private:
        AABBTreeRO(const AABBTreeROHeader &&_header, const void *data, bool _ownData = false) :
                header(_header),
                nodes(reinterpret_cast<const NodeType *>(data)),
                ownData(_ownData) {
        }

    public:
        explicit AABBTreeRO(const AABBTree<B> &tree) :
                AABBTreeRO(AABBTreeROHeader{tree.root, tree.allocatedNodeCount}, tree.nodes.data()) {
        }

        AABBTreeRO(const AABBTreeRO<B> &tree) :
                AABBTreeRO(AABBTreeROHeader(tree.header), tree.nodes) {
        }

        AABBTreeRO(AABBTreeRO<B> &&tree) noexcept :
                AABBTreeRO(std::move(tree.header), tree.nodes, tree.ownData) {
            tree.ownData = false;
        }

        virtual ~AABBTreeRO() {
            if (ownData)
                delete[] nodes;
        }

        template<typename TreeType, typename Geometry>
        friend std::vector<typename TreeType::BoxType> QueryAABBTree(const TreeType *tree, const Geometry &g); // NOLINT

        template<typename Geometry>
        inline std::vector<B> operator&&(const Geometry &g) const {
            return QueryAABBTree(this, g);
        }

        void Save(std::ostream &s) const {
            s.write(reinterpret_cast<const char *>(&header), sizeof(header));
            s.write(reinterpret_cast<const char *>(nodes), sizeof(NodeType) * header.count);
        }

        static AABBTreeRO<B> Load(std::istream &s) {
            typename AABBTreeRO<B>::AABBTreeROHeader header;
            s.read(reinterpret_cast<char *>(&header), sizeof(header));
            auto nodes = new NodeType[header.count];
            std::cout<<"root: "<<header.root<<"  node count: "<<header.count<<std::endl;
            s.read(reinterpret_cast<char *>(nodes), sizeof(NodeType) * header.count);
            return AABBTreeRO<B>(std::move(header), nodes, true);
        }
    };

    template<typename TreeType, typename Geometry>
    std::vector<typename TreeType::BoxType> QueryAABBTree(const TreeType *tree, const Geometry &g) {
        std::vector<typename TreeType::BoxType> overlaps;
        std::stack<unsigned> stack;
        stack.push(tree->root);
        while (!stack.empty()) {
            unsigned nodeIndex = stack.top();
            stack.pop();
            if (nodeIndex == typename TreeType::NodeType::NIL) continue;
            const auto &node = tree->nodes[nodeIndex];
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
}