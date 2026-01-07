#pragma once

#include <deque>
#include "Vector3.hpp"

namespace SolarSim {

/**
 * @brief Represents a celestial body in the simulation.
 */
class Body {
public:
    std::string name;
    double mass;    // in Solar Masses
    double radius;  // in AU (or scaled for visualization)
    Vector3 position; // in AU
    Vector3 velocity; // in AU/Year
    Vector3 acceleration; // in AU/Year^2

    // Orbital trail for visualization
    std::deque<Vector3> trail;
    static constexpr size_t MAX_TRAIL_POINTS = 500;

    // Visual & Physical Rotation
    double rotationAngle; // in degrees
    double rotationSpeed; // degrees per year
    double axialTilt;     // in degrees
    
    // Parent body name (for moons orbiting planets)
    std::string parentName;  // Empty for planets/Sun, set for moons

    Body(const std::string& name, double mass, double radius, 
         Vector3 pos = Vector3(), Vector3 vel = Vector3())
        : name(name), mass(mass), radius(radius), 
          position(pos), velocity(vel), acceleration(0, 0, 0),
          rotationAngle(0), rotationSpeed(0), axialTilt(0), parentName("") {}

    /**
     * @brief Resets current acceleration to zero. 
     */
    void resetAcceleration() {
        acceleration = Vector3(0, 0, 0);
    }

    /**
     * @brief Updates rotation angle based on speed and timestep.
     */
    void updateRotation(double dt) {
        rotationAngle += rotationSpeed * dt;
        rotationAngle = std::fmod(rotationAngle, 360.0);
        if (rotationAngle < 0.0) rotationAngle += 360.0;
    }

    /**
     * @brief Updates position and adds current position to trail.
     */
    void updatePosition(double dt) {
        position += velocity * dt;
        updateRotation(dt);
        
        // Update trail every few steps (simplified: every update here)
        trail.push_back(position);
        if (trail.size() > MAX_TRAIL_POINTS) {
            trail.pop_front();
        }
    }

    /**
     * @brief Simple velocity update based on acceleration and timestep.
     */
    void updateVelocity(double dt) {
        velocity += acceleration * dt;
    }
};

} // namespace SolarSim
