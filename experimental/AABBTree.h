// ref & credits
// https://www.azurefromthetrenches.com/introductory-guide-to-aabb-tree-collision-detection/
// https://github.com/JamesRandall/SimpleVoxelEngine/blob/master/voxelEngine/src/AABBTree.h
#pragma once
#include <memory>
#include <vector>
#include <map>
#include <forward_list>
#include <algorithm>

struct AABB
{
private:
    float calculateSurfaceArea() const { return 2.0f * (getWidth() * getHeight() + getWidth()*getDepth() + getHeight()*getDepth()); }

public:
    float minX;
    float minY;
    float minZ;
    float maxX;
    float maxY;
    float maxZ;
    float surfaceArea;

    AABB() : minX(0.0f), minY(0.0f), minZ(0.0f), maxX(0.0f), maxY(0.0f), maxZ(0.0f), surfaceArea(0.0f) { }
    AABB(unsigned minX, unsigned minY, unsigned minZ, unsigned maxX, unsigned maxY, unsigned maxZ) :
            AABB(static_cast<float>(minX), static_cast<float>(minY), static_cast<float>(minZ), static_cast<float>(maxX), static_cast<float>(maxY), static_cast<float>(maxZ)) { }
    AABB(float minX, float minY, float minZ, float maxX, float maxY, float maxZ) :
            minX(minX), minY(minY), minZ(minZ), maxX(maxX), maxY(maxY), maxZ(maxZ)
    {
        surfaceArea = calculateSurfaceArea();
    }

    bool overlaps(const AABB& other) const
    {
        // y is deliberately first in the list of checks below as it is seen as more likely than things
        // collide on x,z but not on y than they do on y thus we drop out sooner on a y fail
        return maxX > other.minX &&
               minX < other.maxX &&
               maxY > other.minY &&
               minY < other.maxY &&
               maxZ > other.minZ &&
               minZ < other.maxZ;
    }

    bool contains(const AABB& other) const
    {
        return other.minX >= minX &&
               other.maxX <= maxX &&
               other.minY >= minY &&
               other.maxY <= maxY &&
               other.minZ >= minZ &&
               other.maxZ <= maxZ;
    }

    AABB merge(const AABB& other) const
    {
        return AABB(
                std::min(minX, other.minX), std::min(minY, other.minY), std::min(minZ, other.minZ),
                std::max(maxX, other.maxX), std::max(maxY, other.maxY), std::max(maxZ, other.maxZ)
        );
    }

    AABB intersection(const AABB& other) const
    {
        return AABB(
                std::max(minX, other.minX), std::max(minY, other.minY), std::max(minZ, other.minZ),
                std::min(maxX, other.maxX), std::min(maxY, other.maxY), std::min(maxZ, other.maxZ)
        );
    }

    float getWidth() const { return maxX - minX; }
    float getHeight() const { return maxY - minY; }
    float getDepth() const { return maxZ - minZ; }
};

class IAABB
{
public:
    virtual ~IAABB() = default;
    virtual AABB getAABB() const = 0;
};

#define AABB_NULL_NODE 0xffffffff

struct AABBNode
{
    AABB aabb;
    std::shared_ptr<IAABB> object;
    // tree links
    unsigned parentNodeIndex;
    unsigned leftNodeIndex;
    unsigned rightNodeIndex;
    // node linked list link
    unsigned nextNodeIndex;

    bool isLeaf() const { return leftNodeIndex == AABB_NULL_NODE; }

    AABBNode() : object(nullptr), parentNodeIndex(AABB_NULL_NODE), leftNodeIndex(AABB_NULL_NODE), rightNodeIndex(AABB_NULL_NODE), nextNodeIndex(AABB_NULL_NODE)
    {

    }
};

class AABBTree
{
private:
    std::map<std::shared_ptr<IAABB>, unsigned> _objectNodeIndexMap;
    std::vector<AABBNode> _nodes;
    unsigned _rootNodeIndex;
    unsigned _allocatedNodeCount;
    unsigned _nextFreeNodeIndex;
    unsigned _nodeCapacity;
    unsigned _growthSize;

    unsigned allocateNode();
    void deallocateNode(unsigned nodeIndex);
    void insertLeaf(unsigned leafNodeIndex);
    void removeLeaf(unsigned leafNodeIndex);
    void updateLeaf(unsigned leafNodeIndex, const AABB& newAaab);
    void fixUpwardsTree(unsigned treeNodeIndex);

public:
    AABBTree(unsigned initialSize);
    ~AABBTree();

    void insertObject(const std::shared_ptr<IAABB>& object);
    void removeObject(const std::shared_ptr<IAABB>& object);
    void updateObject(const std::shared_ptr<IAABB>& object);
    std::forward_list<std::shared_ptr<IAABB>> queryOverlaps(const std::shared_ptr<IAABB>& object) const;
};
