#pragma once

#include <cmath>
#include <iostream>

#ifdef _MSC_VER
#define ALIGN_AS(n) __declspec(align(n))
#else
#define ALIGN_AS(n) __attribute__((aligned(n)))
#endif

namespace SolarSim {

/**
 * @brief A simple 3D vector struct for physics calculations.
 * Aligned for potential SIMD optimization.
 */
struct ALIGN_AS(16) Vector3 {
    double x, y, z;
    double padding; // Ensure 32-byte alignment for 2 doubles or 16-byte for future-proofing

    Vector3(double x = 0.0, double y = 0.0, double z = 0.0) : x(x), y(y), z(z), padding(0.0) {}

    // Vector operations
    Vector3 operator+(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }

    Vector3 operator-(const Vector3& other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }

    Vector3 operator*(double scalar) const {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }

    Vector3 operator/(double scalar) const {
        return Vector3(x / scalar, y / scalar, z / scalar);
    }

    Vector3& operator+=(const Vector3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Vector3& operator-=(const Vector3& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    Vector3& operator*=(double scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    double lengthSquared() const {
        return x * x + y * y + z * z;
    }

    double length() const {
        return std::sqrt(lengthSquared());
    }

    Vector3 normalized() const {
        double l = length();
        if (l > 0) return *this / l;
        return Vector3();
    }

    friend std::ostream& operator<<(std::ostream& os, const Vector3& v) {
        os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
        return os;
    }
};

} // namespace SolarSim
