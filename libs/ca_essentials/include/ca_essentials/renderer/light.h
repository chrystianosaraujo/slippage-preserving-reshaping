#pragma once

#include <glm/glm.hpp>

struct Light {
    Light() {}

    Light(const glm::fvec4& pos)
    : position(pos)
    { }

    Light(const glm::fvec3& d, const glm::fvec3& a, const glm::fvec3& s, const glm::fvec4& pos)
    : Ld(d), La(a), Ls(s), position(pos)
    {}

    static const Light& default_light() {
        static Light l;
        return l;
    }

    glm::fvec3 Ld = glm::fvec3(0.67f, 0.67f, 0.67f);
    glm::fvec3 Ls = glm::fvec3(0.00f, 0.00f, 0.00f);
    glm::fvec3 La = glm::fvec3(0.35f, 0.43f, 0.48f);
    glm::fvec4 position = glm::fvec4(1.0f, 1.0f, 0.0f, 1.0f);
};
