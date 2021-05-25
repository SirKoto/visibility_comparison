#ifndef CHCPP_HPP
#define CHCPP_HPP

#include "Mesh.hpp"

#include <queue>
#include <stack>
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

    void drawBoxesAtDepth(uint32_t depth);

    void executeCHCPP(const glm::vec3& cameraPosition, const glm::mat4& cameraMatrix);

private:

    const Mesh *mMesh = nullptr;
    const std::vector<glm::vec2>* mPositions = nullptr;

    std::unique_ptr<BVH_Node> mRoot = nullptr;

    std::queue<BVH_Node*> v_queue;
    std::queue<BVH_Node*> i_queue;
    struct Comparator{
        bool operator() (const std::pair<BVH_Node*, float_t>& a,
                         const std::pair<BVH_Node*, float_t>& b){
            return a.second > b.second;
        }
    };

    std::priority_queue<std::pair<BVH_Node*, float_t>,
                        std::vector<std::pair<BVH_Node*, float_t>>,
                        Comparator> distanceQueue;
    std::queue<BVH_Node*> queryQueue;

    void traverseNode(const glm::vec3 &cameraPosition, BVH_Node* node);
    void pushToDistanceQueue(const glm::vec3 &cameraPosition, BVH_Node* node);
    static void pullUpVisibility(BVH_Node* node);
    void handleReturnedQuery(const glm::vec3 &cameraPosition, BVH_Node* node);
    void queryIndividualNodes(BVH_Node* node);
    void issueQuery(BVH_Node* node);
    bool isQueryFinished(BVH_Node* node);
    void issueMultiQueries();
    void queryPreviouslyInvisibleNode(BVH_Node* node);

    static constexpr uint32_t MAX_BATCH_SIZE = 4;
};


class AABBox {
public:
    AABBox() { this-> reset(); }
    AABBox(glm::vec3 min, glm::vec3 max) : mMin(min), mMax(max) {}
    void reset();

    AABBox operator+(const AABBox& o) const;

    const glm::vec3& min() const { return mMin; }
    const glm::vec3& max() const { return mMax; }
    const glm::vec3 center() const { return (mMin + mMax) * 0.5f; }
private:
    glm::vec3 mMin;
    glm::vec3 mMax;
};

class BVH_Node {
public:
    BVH_Node();
    void initLeaf(uint32_t primitive, const AABBox& box);
    void initInterior(uint32_t axis, std::unique_ptr<BVH_Node>&& n0, std::unique_ptr<BVH_Node>&& n1);
    void createBBoxVAO(const Mesh* mesh);
    bool isLeaf() const { return mChildren[0] == nullptr; }
    BVH_Node* getChild0() { return mChildren[0].get(); }
    BVH_Node* getChild1() { return mChildren[1].get(); }
    const BVH_Node* getChild0() const { return mChildren[0].get(); }
    const BVH_Node* getChild1() const { return mChildren[1].get(); }
    BVH_Node* getParent() const { return mParent; }

    const AABBox& getBBox() const { return mBox; }
    const bool isVisible() { return mIsVisible; }
    void setVisible(bool visible) { mIsVisible = visible; }

    void draw() const;
    uint32_t getPrimitive() const { return mPrimitives.front(); }
    uint32_t getQuery() const { return mQuery; }
private:
    AABBox mBox;
    std::array<std::unique_ptr<BVH_Node>, 2> mChildren;
    BVH_Node* mParent = nullptr;
    uint32_t mSplitAxis;
    std::vector<uint32_t> mPrimitives;

    bool mIsVisible = false;

    uint32_t mVAO, mVBO, mVBOI;
    uint32_t mQuery;
};

#endif // CHCPP_HPP
