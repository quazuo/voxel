#ifndef MYGE_VEC_H
#define MYGE_VEC_H

#include <valarray>
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

struct Vec2 {
    double x{}, y{};

    [[maybe_unused]] [[nodiscard]]
    glm::vec2 toGlm() const {
        return {x, y};
    }

    [[maybe_unused]] [[nodiscard]]
    bool isNear(const Vec2 other, const double eps = 0.01) const {
        return fabs(x - other.x) < eps &&
               fabs(y - other.y) < eps;
    }

    [[maybe_unused]] [[nodiscard]]
    Vec2 operator+(Vec2 other) const {
        return {x + other.x, y + other.y};
    }
};

struct Vec3 {
    double x{}, y{}, z{};

    [[maybe_unused]] [[nodiscard]]
    double dist(const Vec3 v) const {
        double distSquared = (v.x - x) * (v.x - x) +
                             (v.y - y) * (v.y - y) +
                             (v.z - z) * (v.z - z);
        return std::sqrt(distSquared);
    }

    [[maybe_unused]] [[nodiscard]]
    double magnitude() const {
        return dist({0, 0, 0});
    }

    [[maybe_unused]] [[nodiscard]]
    double dot(const Vec3 v) const {
        return (x * v.x) + (y * v.y) + (z * v.z);
    }

    [[maybe_unused]] [[nodiscard]]
    glm::vec3 toGlm() const {
        return {x, y, z};
    }

    [[maybe_unused]] [[nodiscard]]
    bool isNear(const Vec3 other, const double eps = 0.01) const {
        return fabs(x - other.x) < eps &&
               fabs(y - other.y) < eps &&
               fabs(z - other.z) < eps;
    }

    [[maybe_unused]] [[nodiscard]]
    Vec3 operator*(double coeff) const {
        return {x * coeff, y * coeff, z * coeff};
    }
};

struct Vec4 {
    double r{}, g{}, b{}, a{};

    [[maybe_unused]] [[nodiscard]]
    glm::vec4 toGlm() const {
        return {r, g, b, a};
    }
};


#endif //MYGE_VEC_H
