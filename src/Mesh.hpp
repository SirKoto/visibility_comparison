#pragma once

#include <vector>
#include <cstdint>
#include <glm/glm.hpp>

class Mesh {
public:
    
    Mesh();
    ~Mesh();

    Mesh& operator=(const Mesh&o) = delete;

    void loadMesh(const char* fileName);

    void draw() const;

    void drawOnlyInstance(uint32_t instance) const;
    void drawBBoxOnlyInstance(uint32_t instance) const;

    size_t numVertices() const { return mVertices.size(); }
    size_t numFaces() const { return mFaces.size(); }

    // The object will havev this size, and it will always be with first vertex of its BBox
    // in (0,0,0)
    glm::vec3 getSize() const;

    const glm::mat4& getModelMatrix() const { return mObjectMatrix; }

    void setInstances(const std::vector<glm::vec2>& xzOffsets);
    void setBBInstances(const std::vector<glm::vec2>& xzOffsets);
    
private:
    struct VertexData
    {
        glm::vec3 pos = glm::vec3(0);
        glm::vec3 normal = glm::vec3(0);
        glm::vec2 uv = glm::vec2(0);
    };
    
    std::vector<VertexData> mVertices;
    std::vector<glm::ivec3> mFaces;
    
    glm::vec3 mMinBB, mMaxBB;

    glm::mat4 mObjectMatrix;

    uint32_t mVAO;
    uint32_t mVertexBO;
    uint32_t mIndexBO;
    uint32_t mInstanceBO;

    uint32_t mBBVAO;
    uint32_t mBBVBO;
    uint32_t mBBInstanceBO;

    uint32_t mNumInstances = 1;


    // Scale to put the object in a cube of basis at most 1x1
    void createObjectMatrix();

    void createBBoxVAO();
    
};
