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
        enum {
            Conservative, Aggressive
        } resizeStrategy;

        unsigned allocateNode() {
            if (nextFreeNodeIndex == NodeType::NIL) {
                assert(allocatedNodeCount == nodeCapacity);
                switch (resizeStrategy) {
                    case Conservative:
                        nodeCapacity += std::max(nodeCapacity / 3, 2u);
                        break;
                    case Aggressive:
                        nodeCapacity <<= 1;
                        break;
                }
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
            unsigned treeNodeIndex = root;
            NodeType *leafNode = &nodes[leafNodeIndex];
            auto areaCache = (*nodes[treeNodeIndex] + **leafNode).Area();
            while (!nodes[treeNodeIndex].IsLeaf()) {
                const NodeType &treeNode = nodes[treeNodeIndex];
                const NodeType &leftNode = nodes[treeNode.left];
                const NodeType &rightNode = nodes[treeNode.right];

                auto newParentCost = 2 * areaCache,
                        minPushDownCost = newParentCost - 2 * treeNode.Area(),
                        area_l = (**leafNode + *leftNode).Area(),
                        area_r = (**leafNode + *rightNode).Area();
                auto cost_l = (leftNode.IsLeaf() ? area_l : area_l - leftNode.Area()) + minPushDownCost,
                        cost_r = (rightNode.IsLeaf() ? area_r : area_r - rightNode.Area()) + minPushDownCost;

                // create new parent here?
                if (newParentCost < cost_l && newParentCost < cost_r)
                    break;
                // descend
                if (cost_l < cost_r) {
                    treeNodeIndex = treeNode.left;
                    areaCache = area_l;
                } else {
                    treeNodeIndex = treeNode.right;
                    areaCache = area_r;
                }

            }

            // create a new parent node and attach the leaf and this item
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

            if (oldParentIndex == NodeType::NIL)
                root = newParentIndex;
            else {
                // the old parent was not the root and so we need to patch the left or right index to
                // point to the new node
                NodeType &oldParent = nodes[oldParentIndex];
                if (oldParent.left == leafSiblingIndex)
                    oldParent.left = newParentIndex;
                else
                    oldParent.right = newParentIndex;
            }

            // walk back up & fix heights and areas
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
            resizeStrategy = initialSize < 7 ? Aggressive : Conservative;
            nodes.resize(initialSize);
            for (unsigned nodeIndex = 0; nodeIndex < initialSize; nodeIndex++) {
                NodeType &node = nodes[nodeIndex];
                node.next = nodeIndex + 1;
            }
            nodes[initialSize - 1].next = NodeType::NIL;
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
            std::cout << "root: " << header.root << "  node count: " << header.count << std::endl;
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