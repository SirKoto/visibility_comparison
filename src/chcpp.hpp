#ifndef CHCPP_HPP
#define CHCPP_HPP

#include "Mesh.hpp"

#include <queue>
#include <glm/glm.hpp>
#include <array>
#include <memory>

class AABBox;
class BVH_Node;

class ChcPP
{
public:
    ChcPP() = default;

    void setMesh(const Mesh* mesh) { mMesh = mesh; }
    void setPositions(const std::vector<glm::vec2>* positions) { mPositions = positions; }

    void buildBVH();

private:

    const Mesh *mMesh = nullptr;
    const std::vector<glm::vec2>* mPositions = nullptr;

    std::unique_ptr<BVH_Node> mRoot = nullptr;
};


class AABBox {
public:
    AABBox() { this-> reset(); }
    AABBox(glm::vec3 min, glm::vec3 max) : mMin(min), mMax(max) {}
    void reset();

    AABBox operator+(const AABBox& o) const;

    const glm::vec3& min() const { return mMin; }
    const glm::vec3& max() const { return mMax; }
private:
    glm::vec3 mMin;
    glm::vec3 mMax;
};

class BVH_Node {
public:
    BVH_Node();
    void initLeaf(uint32_t primitive, const AABBox& box);
    void initInterior(uint32_t axis, std::unique_ptr<BVH_Node>&& n0, std::unique_ptr<BVH_Node>&& n1);

    bool isLeaf() const { return mChildren[0] == nullptr; }
private:
    AABBox mBox;
    std::array<std::unique_ptr<BVH_Node>, 2> mChildren;
    uint32_t mSplitAxis;
    std::vector<uint32_t> mPrimitives;
};

#endif // CHCPP_HPP
