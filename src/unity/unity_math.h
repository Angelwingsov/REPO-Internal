#pragma once
#include <cmath>

namespace Unity {

struct Vector2 {
    float x = 0.0f;
    float y = 0.0f;

    Vector2() = default;
    Vector2(float _x, float _y) : x(_x), y(_y) {}

    Vector2 operator+(const Vector2& o) const { return { x + o.x, y + o.y }; }
    Vector2 operator-(const Vector2& o) const { return { x - o.x, y - o.y }; }
    Vector2 operator*(float s) const { return { x * s, y * s }; }
    Vector2 operator/(float s) const { return (s != 0.0f) ? Vector2{ x / s, y / s } : Vector2{}; }

    float LengthSq() const { return x * x + y * y; }
    float Length() const { return std::sqrt(LengthSq()); }
    float Distance(const Vector2& o) const { return (*this - o).Length(); }

    bool IsValid() const {
        return !std::isnan(x) && !std::isnan(y) && !std::isinf(x) && !std::isinf(y);
    }
};

struct Vector3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vector3() = default;
    Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    Vector3 operator+(const Vector3& o) const { return { x + o.x, y + o.y, z + o.z }; }
    Vector3 operator-(const Vector3& o) const { return { x - o.x, y - o.y, z - o.z }; }
    Vector3 operator*(float s) const { return { x * s, y * s, z * s }; }
    Vector3 operator/(float s) const { return (s != 0.0f) ? Vector3{ x / s, y / s, z / s } : Vector3{}; }

    float Dot(const Vector3& o) const { return x * o.x + y * o.y + z * o.z; }
    Vector3 Cross(const Vector3& o) const {
        return {
            y * o.z - z * o.y,
            z * o.x - x * o.z,
            x * o.y - y * o.x
        };
    }

    float LengthSq() const { return x * x + y * y + z * z; }
    float Length() const { return std::sqrt(LengthSq()); }
    float Distance(const Vector3& o) const { return (*this - o).Length(); }
    float DistanceSq(const Vector3& o) const { return (*this - o).LengthSq(); }

    Vector3 Normalized() const {
        float len = Length();
        return (len > 0.00001f) ? (*this / len) : Vector3{};
    }

    bool IsValid(float maxCoordinate = 25000.0f) const {
        if (std::isnan(x) || std::isnan(y) || std::isnan(z)) return false;
        if (std::isinf(x) || std::isinf(y) || std::isinf(z)) return false;
        return (std::abs(x) <= maxCoordinate && std::abs(y) <= maxCoordinate && std::abs(z) <= maxCoordinate);
    }

    bool IsZero() const {
        return (x == 0.0f && y == 0.0f && z == 0.0f);
    }
};

struct Bounds {
    Vector3 center;
    Vector3 extents;
    bool IsValid() const {
        return center.IsValid() && extents.IsValid(5) &&
            extents.x > .001f && extents.y > .001f && extents.z > .001f;
    }
};
static_assert(sizeof(Bounds) == 24, "Unity Bounds consists of center and extents Vector3 values");

struct Color {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;

    Color() = default;
    Color(float _r, float _g, float _b, float _a = 1.0f) : r(_r), g(_g), b(_b), a(_a) {}

    bool operator==(const Color& o) const {
        return r == o.r && g == o.g && b == o.b && a == o.a;
    }
    bool operator!=(const Color& o) const {
        return !(*this == o);
    }
};

}
