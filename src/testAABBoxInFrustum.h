#ifndef TESTAABBOXINFRUSTUM_H
#define TESTAABBOXINFRUSTUM_H

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

inline bool testAABBoxInFrustum(const glm::vec3& min, const glm::vec3& max, const glm::mat4& MVP) {

    uint32_t posX = 0, negX = 0,
             posY = 0, negY = 0,
             negZ = 0;

    bool inZ = true;

    // test all 8 corners
    for(uint32_t i = 0; i < 8; ++i) {
        bool c0 = i & 0b1;
        bool c1 = i & 0b10;
        bool c2 = i & 0b100;
        glm::vec3 interp(c0, c1, c2);
        glm::vec4 corner = glm::vec4( min * (1.0f - interp) + max * interp, 1.0f);

        corner = MVP * corner;

        posX += corner.x >  corner.w ? 1 : 0;
        negX += corner.x < -corner.w ? 1 : 0;
        posY += corner.y >  corner.w ? 1 : 0;
        negY += corner.y < -corner.w ? 1 : 0;
        negZ += corner.z < 0 ? 1 : 0;

        bool inZ_ = corner.z <= corner.w && corner.z >= 0;
        inZ &= inZ_;
        bool inFrustum = std::abs(corner.x) <= corner.w &&
                std::abs(corner.y) <= corner.w &&
                inZ_;


        if(inFrustum) {
            return true;
        }
    }

    if((inZ || (negZ != 0 && negZ != 8)) && posX && negX && (posY != 8 && posY != 8)) {
        return true;
    }
    if((inZ || (negZ != 0 && negZ != 8)) && posY && negY && (posX != 8 && posX != 8)) {
        return true;
    }

    return false;
}

#endif // TESTAABBOXINFRUSTUM_H
