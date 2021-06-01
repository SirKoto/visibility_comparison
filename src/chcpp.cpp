#include "chcpp.hpp"

#include <limits>
#include <math.h>
#include <algorithm>
#include <glad/glad.h>

#include "testAABBoxInFrustum.h"

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
                        newNode->createBBoxVAO(mMesh);
                    }
                } else {
                    if(j + 1 == yRes) {
                        newNode = std::move(nodes[i * yRes + j]);
                    } else {
                        newNode->initInterior(mergingX,
                                             std::move(nodes[i * yRes + j]),
                                             std::move(nodes[i * yRes + j + 1]));
                        newNode->createBBoxVAO(mMesh);
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

void drawBoxesAtDepthIntern(uint32_t actualD, uint32_t maxD, const BVH_Node* node) {
    if(node->isLeaf() || actualD > maxD){
        return;
    }

    if(actualD == maxD) {
        node->draw();
        return;
    }

    drawBoxesAtDepthIntern(actualD + 1, maxD, node->getChild0());
    drawBoxesAtDepthIntern(actualD + 1, maxD, node->getChild1());

}

void ChcPP::drawBoxesAtDepth(uint32_t depth)
{
    drawBoxesAtDepthIntern(0, depth, mRoot.get());
}

void ChcPP::executeCHCPP(const glm::vec3 &cameraPosition, const glm::mat4 &cameraMatrix)
{
    // we asume that all the queues are already empty
    pushToDistanceQueue(cameraPosition, mRoot.get());

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);

    while(!distanceQueue.empty() || !queryQueue.empty()) {
        while(!queryQueue.empty()) {
            if(isQueryFinished(queryQueue.front())) {
                BVH_Node* node = queryQueue.front();
                queryQueue.pop();
                handleReturnedQuery(cameraPosition, node);
            } else if(!v_queue.empty()){
                assert(!v_queue.empty());
                issueQuery(v_queue.front());
                v_queue.pop();
            }
        } // end while !queryQueue.empty()

        if(!distanceQueue.empty()) {
            BVH_Node* node = distanceQueue.top().first;
            distanceQueue.pop();
            if(testAABBoxInFrustum(node->getBBox().min(), node->getBBox().max(), cameraMatrix)) {
                // if not was visible...
                if(!node->isVisible()) {
                    queryPreviouslyInvisibleNode(node);
                } else {
                    if(node->isLeaf()) { //TODO: query reasonable
                        v_queue.push(node);
                    }
                    traverseNode(cameraPosition, node);
                }
            }
        }

        if(distanceQueue.empty()) {
            issueMultiQueries();
        }

    } // end while !distanceQueue.empty() || !queryQueue.empty()

    while(!v_queue.empty()) {
        issueQuery(v_queue.front());
        v_queue.pop();
    }
    //glFinish();

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    for(uint32_t i : mRenderQueue) {
        mMesh->drawOnlyInstance(i);
    }
    mRenderQueue.clear();
    glFinish();
}

void ChcPP::traverseNode(const glm::vec3 &cameraPosition, BVH_Node *node)
{
    if(node->isLeaf()){
        mRenderQueue.push_back(node->getPrimitive());
    } else {
        pushToDistanceQueue(cameraPosition, node->getChild0());
        pushToDistanceQueue(cameraPosition, node->getChild1());
        node->setVisible(false);
    }
}

void ChcPP::pushToDistanceQueue(const glm::vec3 &cameraPosition, BVH_Node *node)
{
    glm::vec3 centerToCamera = cameraPosition - node->getBBox().center();
    float_t d = glm::dot(centerToCamera, centerToCamera);
    distanceQueue.push({node, d});
}

void ChcPP::pullUpVisibility(BVH_Node *node)
{
    while (node != nullptr && !node->isVisible()) {
        node->setVisible(true);
        node = node->getParent();
    }
}

void ChcPP::queryIndividualNodes(BVH_Node *node)
{
    if(node->isLeaf()) {
        issueQuery(node);
    } else {
        queryIndividualNodes(node->getChild0());
        queryIndividualNodes(node->getChild1());
    }
}

void ChcPP::issueQuery(BVH_Node *node)
{
    glBeginQuery(GL_ANY_SAMPLES_PASSED_CONSERVATIVE, node->getQuery());
    if(node->isLeaf()){
        mMesh->drawBBoxOnlyInstance(node->getPrimitive());
    } else {
        node->draw();
    }
    glEndQuery(GL_ANY_SAMPLES_PASSED_CONSERVATIVE);
    queryQueue.push(node);
}

bool ChcPP::isQueryFinished(BVH_Node *node)
{
    int32_t res;
    glGetQueryObjectiv(node->getQuery(), GL_QUERY_RESULT_AVAILABLE, &res);
    return res != 0;
}

void ChcPP::issueMultiQueries()
{
    while(!i_queue.empty()) {
        issueQuery(i_queue.front());
        i_queue.pop();
    }
    glFlush();
}

void ChcPP::queryPreviouslyInvisibleNode(BVH_Node *node)
{
    i_queue.push(node);

    if(i_queue.size() >= MAX_BATCH_SIZE) {
        issueMultiQueries();
    }
}

void ChcPP::handleReturnedQuery(const glm::vec3 &cameraPosition, BVH_Node *node)
{
    uint32_t query = node->getQuery();
    uint32_t samplePassed;
    glGetQueryObjectuiv(query, GL_QUERY_RESULT, &samplePassed);

    if(samplePassed) {
        // if node.size() > 1
        if(!node->isLeaf()) {
            // query individual nodes. Multiquery failed
            queryIndividualNodes(node);
        } else {
            if(!node->isVisible()) {
                traverseNode(cameraPosition, node);
            }
            pullUpVisibility(node);
        }
    } else{
        node->setVisible(false);
    }

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
    glGenVertexArrays(1, &mVAO);
    glGenBuffers(1, &mVBO);
    glGenBuffers(1, &mVBOI);
    glGenQueries(1, &mQuery);
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

    mChildren[0]->mParent = this;
    mChildren[1]->mParent = this;
}

void BVH_Node::createBBoxVAO(const Mesh *mesh)
{
    mesh->createBBoxVAOModelTransform(mVAO, mVBO, mVBOI, mBox.min(), mBox.max());
}

void BVH_Node::draw() const
{
    glBindVertexArray(this->mVAO);

    glDrawArraysInstanced(GL_TRIANGLES, 0, 36, 1);

    glBindVertexArray(0);
}
