#ifndef MYGE_VEC_H
#define MYGE_VEC_H

#include <valarray>
#include <sstream>
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

struct Vec2 {
    double x{}, y{};

    [[nodiscard]]
    glm::vec2 toGlm() const {
        return {x, y};
    }

    [[nodiscard]]
    std::string toString() const {
        std::stringstream ss;
        ss << "(" << x << ", " << y << ")";
        return ss.str();
    }

    [[nodiscard]]
    bool isNear(const Vec2 other, const double eps = 0.01) const {
        return fabs(x - other.x) < eps &&
               fabs(y - other.y) < eps;
    }

    [[nodiscard]]
    Vec2 operator+(Vec2 other) const {
        return {x + other.x, y + other.y};
    }
};

struct Vec3 {
    double x{}, y{}, z{};

    [[nodiscard]]
    static Vec3 fromGlm(glm::vec3 other) {
        return {(double) other.x, (double) other.y, (double) other.z};
    }

    [[nodiscard]]
    std::string toString() const {
        std::stringstream ss;
        ss << "(" << x << ", " << y << ", " << z << ")";
        return ss.str();
    }

    [[nodiscard]]
    double dist(const Vec3 v) const {
        double distSquared = (v.x - x) * (v.x - x) +
                             (v.y - y) * (v.y - y) +
                             (v.z - z) * (v.z - z);
        return std::sqrt(distSquared);
    }

    [[nodiscard]]
    double magnitude() const {
        return dist({0.0, 0.0, 0.0});
    }

    [[nodiscard]]
    double dot(const Vec3 v) const {
        return (x * v.x) + (y * v.y) + (z * v.z);
    }

    [[nodiscard]]
    glm::vec3 toGlm() const {
        return {x, y, z};
    }

    [[nodiscard]]
    bool isNear(const Vec3 other, const double eps = 0.01) const {
        return fabs(x - other.x) < eps &&
               fabs(y - other.y) < eps &&
               fabs(z - other.z) < eps;
    }

    // operators

    [[nodiscard]]
    Vec3 operator+(Vec3 other) const {
        return {x + other.x, y + other.y, z + other.z};
    }

    Vec3& operator+=(Vec3 other) {
        *this = *this + other;
        return *this;
    }

    [[nodiscard]]
    Vec3 operator-(Vec3 other) const {
        return {x - other.x, y - other.y, z - other.z};
    }

    Vec3& operator-=(Vec3 other) {
        *this = *this - other;
        return *this;
    }

    [[nodiscard]]
    Vec3 operator*(double coeff) const {
        return {x * coeff, y * coeff, z * coeff};
    }
};

struct Vec4 {
    double r{}, g{}, b{}, a{};

    [[nodiscard]]
    glm::vec4 toGlm() const {
        return {r, g, b, a};
    }
};


#endif //MYGE_VEC_H
