
#include "Mesh.hpp"
#include <happly.h>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>


Mesh::Mesh()
{
    glGenVertexArrays(1, &mVAO);
    glGenBuffers(1, &mVertexBO);
    glGenBuffers(1, &mIndexBO);
    glGenBuffers(1, &mInstanceBO);

    glGenVertexArrays(1, &mBBVAO);
    glGenBuffers(1, &mBBVBO);
    glGenBuffers(1, &mBBInstanceBO);
}

Mesh::~Mesh()
{
    glDeleteBuffers(1, &mVertexBO);
    glDeleteBuffers(1, &mIndexBO);
    glDeleteBuffers(1, &mInstanceBO);
    glDeleteVertexArrays(1, &mVAO);

    glDeleteBuffers(1, &mBBVBO);
    glDeleteBuffers(1, &mBBInstanceBO);
    glDeleteVertexArrays(1, &mBBVAO);
}

void Mesh::loadMesh(const char* fileName) {
 
    mVertices.clear();
    mFaces.clear();
    
    happly::PLYData plyIn(fileName);
    
    
    const std::string vertexElementName = "vertex";
    happly::Element& vertEl = plyIn.getElement(vertexElementName);

    std::vector<float> xPos = vertEl.getProperty<float>("x");
    std::vector<float> yPos = vertEl.getProperty<float>("y");
    std::vector<float> zPos = vertEl.getProperty<float>("z");

    bool hasNormal = vertEl.hasProperty("nx") && vertEl.hasProperty("ny") && vertEl.hasProperty("nz");
    std::vector<float> nx;
    std::vector<float> ny;
    std::vector<float> nz;
    bool hasUvs = vertEl.hasProperty("s") && vertEl.hasProperty("t");
    std::vector<float> u;
    std::vector<float> v;


    if(hasNormal){
        nx = vertEl.getProperty<float>("nx");
        ny = vertEl.getProperty<float>("ny");
        nz = vertEl.getProperty<float>("nz");
    }
    if(hasUvs){
        u = vertEl.getProperty<float>("s");
        v = vertEl.getProperty<float>("t");
    }

    const auto f = plyIn.getFaceIndices<uint32_t>();
    
    
    uint32_t numVert = (uint32_t)xPos.size(), numFaces = (uint32_t)f.size();
    
    mVertices.reserve(numVert);
    mFaces.reserve(numFaces);
    
    for(uint32_t i = 0; i < numVert; ++i) {
        VertexData d;
        d.pos = glm::vec3(xPos[i], yPos[i], zPos[i]);
        if(hasNormal){
            d.normal = glm::vec3(nx[i], ny[i], nz[i]);
        }
        if(hasUvs){
            d.uv = glm::vec2(u[i], v[i]);
        }

        mVertices.push_back(d);
    }
    
    for(const auto& ff : f){
        if(ff.size() != 3){
            throw std::runtime_error("Loading face with a number of vertices different than 3");
        }
        mFaces.push_back(glm::ivec3(ff[0], ff[1], ff[2]));
    }

    // Compute bounding box
    mMinBB = glm::vec3( std::numeric_limits<float>::infinity());
    mMaxBB = glm::vec3(-std::numeric_limits<float>::infinity());
    for(const VertexData& v : mVertices) {
        mMinBB = glm::min(mMinBB, v.pos);
        mMaxBB = glm::max(mMaxBB, v.pos);
    }

    // Upload data to gpu
    glBindVertexArray(mVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBO);
    glBufferData(GL_ARRAY_BUFFER,
                mVertices.size() * sizeof(VertexData),
                mVertices.data(),
                GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, pos));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, normal));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, uv));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, mInstanceBO);
    glm::vec2 tmp(0.0f);
    glBufferData(GL_ARRAY_BUFFER,
                sizeof(tmp),
                &tmp.x,
                GL_DYNAMIC_DRAW);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);

    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 mFaces.size() * sizeof(glm::ivec3),
                 mFaces.data(),
                 GL_STATIC_DRAW);

    glBindVertexArray(0);

    createObjectMatrix();
    createBBoxVAO();

}

void Mesh::createBBoxVAO()
{
    glBindVertexArray(mBBVAO);
  // vertices
  float vertices[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f,

        0.0f, 0.0f,  1.0f,
        1.0f, 0.0f,  1.0f,
        1.0f, 1.0f,  1.0f,
        1.0f, 1.0f,  1.0f,
        0.0f, 1.0f,  1.0f,
        0.0f, 0.0f,  1.0f,

        0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 1.0f,

        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f,

        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 0.0f,

        0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f,
      };

    for(uint32_t i = 0; i < 36; ++i) {
        glm::vec3 v(vertices[i * 3 + 0], vertices[i * 3 + 1], vertices[i * 3 + 2]);

        v = mMinBB + v * (mMaxBB - mMinBB);

        vertices[i * 3 + 0] = v.x;
        vertices[i * 3 + 1] = v.y;
        vertices[i * 3 + 2] = v.z;
    }

    glBindBuffer(GL_ARRAY_BUFFER, mBBVBO);
    glBufferData(GL_ARRAY_BUFFER,
               sizeof(vertices),
               vertices,
               GL_STATIC_DRAW
               );

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0, // location
                        3, GL_FLOAT, // Type
                        GL_FALSE,
                        3 * sizeof(float_t), // stride
                        0
                        );

    glBindBuffer(GL_ARRAY_BUFFER, mBBInstanceBO);
    glm::vec2 tmp(0.0f);
    glBufferData(GL_ARRAY_BUFFER,
              sizeof(tmp),
              &tmp.x,
              GL_STATIC_DRAW);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);

    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    glBindVertexArray(0);
}

void Mesh::draw() const
{
    glBindVertexArray(mVAO);

    glDrawElementsInstanced(GL_TRIANGLES, mFaces.size() * 3, GL_UNSIGNED_INT, 0, mNumInstances);

    glBindVertexArray(0);
}

void Mesh::drawOnlyInstance(uint32_t instance) const
{
    glBindVertexArray(mVAO);

    glDrawElementsInstancedBaseInstance(GL_TRIANGLES, mFaces.size() * 3, GL_UNSIGNED_INT, 0, 1, instance);

    glBindVertexArray(0);
}

void Mesh::drawBBoxOnlyInstance(uint32_t instance) const
{
    glBindVertexArray(mBBVAO);

    glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 36, 1, instance);

    glBindVertexArray(0);
}

glm::vec3 Mesh::getSize() const
{
    float scaleFactor = 1.0f / std::max(mMaxBB.x - mMinBB.x, mMaxBB.z - mMinBB.z);
    return scaleFactor * (mMaxBB - mMinBB);
}

void Mesh::setInstances(const std::vector<glm::vec2> &xzOffsets)
{
    mNumInstances = (uint32_t) xzOffsets.size();

    glBindVertexArray(mVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mInstanceBO);
    glBufferData(GL_ARRAY_BUFFER,
                sizeof(glm::vec2) * mNumInstances,
                xzOffsets.data(),
                GL_DYNAMIC_DRAW);

    glBindVertexArray(0);

}

void Mesh::setBBInstances(const std::vector<glm::vec2> &xzOffsets)
{
    glBindVertexArray(mBBVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mBBInstanceBO);
    glBufferData(GL_ARRAY_BUFFER,
                sizeof(glm::vec2) * xzOffsets.size(),
                xzOffsets.data(),
                GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void Mesh::createObjectMatrix()
{
    mObjectMatrix = glm::mat4(1.f);
    float scaleFactor = 1.0f / std::max(mMaxBB.x - mMinBB.x, mMaxBB.z - mMinBB.z);

    mObjectMatrix = glm::translate(mObjectMatrix, glm::vec3(0.05f));
    mObjectMatrix = glm::scale(mObjectMatrix, glm::vec3(0.9f));
    mObjectMatrix = glm::scale(mObjectMatrix, glm::vec3(scaleFactor));
    mObjectMatrix = glm::translate(mObjectMatrix, -mMinBB);
}
