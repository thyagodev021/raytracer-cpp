#pragma once
#include <algorithm>
#include <cmath>

struct Vec3 {
    double x = 0, y = 0, z = 0;
    constexpr Vec3() = default;
    constexpr Vec3(double x, double y, double z) : x(x), y(y), z(z) {}

    constexpr Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    constexpr Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    constexpr Vec3 operator*(const Vec3& o) const { return {x * o.x, y * o.y, z * o.z}; }
    constexpr Vec3 operator*(double t) const { return {x * t, y * t, z * t}; }
    constexpr Vec3 operator/(double t) const { return {x / t, y / t, z / t}; }
    constexpr Vec3 operator-() const { return {-x, -y, -z}; }
    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }

    double length() const { return std::sqrt(x * x + y * y + z * z); }
    Vec3 normalized() const { return *this / length(); }
};

constexpr Vec3 operator*(double t, const Vec3& v) { return v * t; }
constexpr double dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
constexpr Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
constexpr Vec3 reflect(const Vec3& v, const Vec3& n) { return v - n * (2.0 * dot(v, n)); }
