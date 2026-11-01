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


struct Quaternion {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;

    Quaternion() = default;
    Quaternion(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}

    bool IsValid() const {
        return !std::isnan(x) && !std::isnan(y) && !std::isnan(z) && !std::isnan(w) &&
               !std::isinf(x) && !std::isinf(y) && !std::isinf(z) && !std::isinf(w);
    }

    static Quaternion Identity() {
        return Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
    }

    static Quaternion Euler(float pitch, float yaw, float roll) {
        float deg2rad = 0.0174532925f * 0.5f;
        float ex = pitch * deg2rad;
        float ey = yaw * deg2rad;
        float ez = roll * deg2rad;
        float cX = std::cos(ex), sX = std::sin(ex);
        float cY = std::cos(ey), sY = std::sin(ey);
        float cZ = std::cos(ez), sZ = std::sin(ez);
        return Quaternion(
            sX * cY * cZ + cX * sY * sZ,
            cX * sY * cZ - sX * cY * sZ,
            cX * cY * sZ - sX * sY * cZ,
            cX * cY * cZ + sX * sY * sZ
        );
    }

    Vector3 ToEuler() const {
        constexpr float rad2deg = 57.295779513f;
        float sp = 2.0f * (w * x - y * z);
        float pitch = 0.0f;
        float yaw = 0.0f;
        float roll = 0.0f;

        if (std::abs(sp) > 0.9999f) {
            pitch = std::copysign(90.0f, sp);
            yaw = -2.0f * std::atan2(y, w) * rad2deg;
            roll = 0.0f;
        } else {
            pitch = std::asin(sp) * rad2deg;
            yaw = std::atan2(2.0f * (w * y + x * z), 1.0f - 2.0f * (x * x + y * y)) * rad2deg;
            roll = std::atan2(2.0f * (w * z + x * y), 1.0f - 2.0f * (x * x + z * z)) * rad2deg;
        }

        auto normalizeAngle = [](float a) {
            while (a < 0.0f) a += 360.0f;
            while (a >= 360.0f) a -= 360.0f;
            return a;
        };

        return Vector3(normalizeAngle(pitch), normalizeAngle(yaw), normalizeAngle(roll));
    }
};
static_assert(sizeof(Quaternion) == 16, "Unity Quaternion size must be 16 bytes");
}
