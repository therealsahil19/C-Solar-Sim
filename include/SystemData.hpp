#pragma once

#include <vector>
#include "Body.hpp"
#include "Vector3.hpp"

namespace SolarSim {

/**
 * @brief Helper to convert a system of bodies to use barycentric coordinates.
 * This ensures the total momentum of the system is zero.
 */
inline void convertToBarycentric(std::vector<Body>& bodies) {
    if (bodies.empty()) return;

    Vector3 totalMomentum(0, 0, 0);
    double totalMass = 0.0;

    for (const auto& body : bodies) {
        totalMomentum += body.velocity * body.mass;
        totalMass += body.mass;
    }

    // Average velocity of the center of mass
    Vector3 v_cm = totalMomentum / totalMass;

    // Subtract V_cm from all bodies
    for (auto& body : bodies) {
        body.velocity -= v_cm;
    }

    // Optional: Center the positions too
    Vector3 centerOfMass(0, 0, 0);
    for (const auto& body : bodies) {
        centerOfMass += body.position * body.mass;
    }
    centerOfMass = centerOfMass / totalMass;

    for (auto& body : bodies) {
        body.position -= centerOfMass;
    }
}

} // namespace SolarSim
