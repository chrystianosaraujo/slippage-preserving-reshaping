#pragma once

#include <glm/glm.hpp>

struct Material {
    Material() {};

    Material(glm::fvec3 d, glm::fvec3 a, glm::fvec3 s, float shin)
    : Kd(d), Ka(a), Ks(s), shininess(shin)
    {}

    glm::fvec3 Kd = glm::fvec3(0.9f, 0.5f, 0.3f);
    glm::fvec3 Ka = glm::fvec3(0.9f, 0.5f, 0.3f);
    glm::fvec3 Ks = glm::fvec3(0.29f, 0.25f, 0.25f);
    float shininess = 100.0f;
};
