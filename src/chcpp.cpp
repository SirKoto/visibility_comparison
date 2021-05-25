#include "chcpp.hpp"

#include <limits>
#include <math.h>
#include <algorithm>

void ChcPP::buildBVH()
{
    assert(mMesh != nullptr && mPositions != nullptr);

    uint32_t resolution = std::sqrt(mPositions->size());
    uint32_t yRes = resolution;
    uint32_t xRes = resolution;
    std::vector<std::unique_ptr<BVH_Node>> nodes;
    nodes.reserve(mPositions->size());
    const glm::vec3 size = mMesh->getSize();
    for(uint32_t i = 0; i < mPositions->size(); ++i){
        glm::vec3 min((*mPositions)[i].x, 0, (*mPositions)[i].y);
        AABBox box(min, min + size);
        nodes.emplace_back( std::make_unique<BVH_Node>() );
        nodes.back()->initLeaf(i, box);
    }

    std::vector<std::unique_ptr<BVH_Node>> nodesNext; nodesNext.reserve(nodes.size() * 3 / 4);
    bool mergingX = true;
    while(nodes.size() != 1) {
        assert(!nodes.empty());

        for(uint32_t i = 0; i < xRes; i += (mergingX ? 2 : 1)) {
            for(uint32_t j = 0; j < yRes; j += (!mergingX ? 2 : 1)) {
                std::unique_ptr<BVH_Node> newNode = std::make_unique<BVH_Node>();
                if(mergingX) {
                    if(i + 1 == xRes) {
                        newNode = std::move(nodes[i * yRes + j]);
                    } else {
                        newNode->initInterior(mergingX,
                                             std::move(nodes[i * yRes + j]),
                                             std::move(nodes[(1 + i) * yRes + j]));
                    }
                } else {
                    if(j + 1 == yRes) {
                        newNode = std::move(nodes[i * yRes + j]);
                    } else {
                        newNode->initInterior(mergingX,
                                             std::move(nodes[i * yRes + j]),
                                             std::move(nodes[i * yRes + j + 1]));
                    }
                }
                nodesNext.push_back(std::move(newNode));
            } // endfor j
        } // endfor i

        if(mergingX) {
            xRes = (xRes / 2) + (xRes % 2 == 0 ? 0 : 1);
        } else {
            yRes = (yRes / 2) + (yRes % 2 == 0 ? 0 : 1);
        }

        nodes.clear();
        std::swap(nodes, nodesNext);
        mergingX = !mergingX;
    }

    // Assign root
    mRoot = std::move(nodes.front());
}

void AABBox::reset()
{
    mMin =  std::numeric_limits<decltype(mMin)>::infinity();
    mMax = -std::numeric_limits<decltype(mMax)>::infinity();
}

AABBox AABBox::operator+(const AABBox &o) const
{
    AABBox r;
    r.mMin = glm::min(mMin, o.mMin);
    r.mMax = glm::max(mMax, o.mMax);
    return r;
}

BVH_Node::BVH_Node()
{
    mChildren = {nullptr, nullptr};
}

void BVH_Node::initLeaf(uint32_t primitive, const AABBox &box)
{
    mChildren = {nullptr, nullptr};
    mPrimitives = { primitive };
    mBox = box;
}

void BVH_Node::initInterior(uint32_t axis, std::unique_ptr<BVH_Node>&& n0, std::unique_ptr<BVH_Node>&& n1)
{
    mSplitAxis = axis;
    mChildren = {std::move(n0), std::move(n1)};
    mBox = mChildren[0]->mBox + mChildren[1]->mBox;

    mPrimitives.clear();
    mPrimitives.insert(mPrimitives.end(), mChildren[0]->mPrimitives.begin(), mChildren[0]->mPrimitives.end());
    mPrimitives.insert(mPrimitives.end(), mChildren[1]->mPrimitives.begin(), mChildren[1]->mPrimitives.end());

    std::sort(mPrimitives.begin(), mPrimitives.end());
}
