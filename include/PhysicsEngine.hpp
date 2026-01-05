#pragma once

#include <vector>
#include <iostream>
#include <algorithm>
#include <cmath>
#include "Body.hpp"
#include "Constants.hpp"
#include "Octree.hpp"

namespace SolarSim {

/**
 * @brief Static physics library for gravitational calculations.
 */
class PhysicsEngine {
public:
    /**
     * @brief Calculates gravitational force between two bodies.
     */
    static void applyGravitationalForce(Body& a, Body& b) {
        Vector3 r_vec = b.position - a.position;
        double distSq = r_vec.lengthSquared() + Constants::SOFTENING_EPSILON;
        double dist = std::sqrt(distSq);
        double forceMagnitude = (Constants::G * a.mass * b.mass) / distSq;
        Vector3 force = r_vec * (forceMagnitude / dist);

        a.acceleration += force / a.mass;
        b.acceleration -= force / b.mass;
    }

    /**
     * @brief Calculates accelerations for all bodies.
     */
    static void calculateAccelerations(std::vector<Body>& bodies) {
        for (auto& body : bodies) body.resetAcceleration();
        for (size_t i = 0; i < bodies.size(); ++i) {
            for (size_t j = i + 1; j < bodies.size(); ++j) {
                applyGravitationalForce(bodies[i], bodies[j]);
            }
        }
    }

    /**
     * @brief Detects and handles inelastic collisions.
     */
    static void handleCollisions(std::vector<Body>& bodies) {
        for (size_t i = 0; i < bodies.size(); ++i) {
            for (size_t j = i + 1; j < bodies.size(); ++j) {
                double dist = (bodies[j].position - bodies[i].position).length();
                if (dist < (bodies[i].radius + bodies[j].radius)) {
                    Body& b1 = bodies[i];
                    Body& b2 = bodies[j];
                    double newMass = b1.mass + b2.mass;
                    Vector3 newPos = (b1.position * b1.mass + b2.position * b2.mass) / newMass;
                    Vector3 newVel = (b1.velocity * b1.mass + b2.velocity * b2.mass) / newMass;
                    double newRadius = std::pow(std::pow(b1.radius, 3) + std::pow(b2.radius, 3), 1.0/3.0);

                    b1.name = b1.name + "-" + b2.name;
                    b1.mass = newMass;
                    b1.radius = newRadius;
                    b1.position = newPos;
                    b1.velocity = newVel;

                    bodies.erase(bodies.begin() + j);
                    --j;
                }
            }
        }
    }

    /**
     * @brief Calculates safe adaptive timestep.
     */
    static double getAdaptiveTimestep(const std::vector<Body>& bodies, double baseDt) {
        double minDistSq = 1e18;
        for (size_t i = 0; i < bodies.size(); ++i) {
            for (size_t j = i + 1; j < bodies.size(); ++j) {
                double d2 = (bodies[j].position - bodies[i].position).lengthSquared();
                if (d2 < minDistSq) minDistSq = d2;
            }
        }
        return std::clamp(0.01 * std::sqrt(minDistSq), baseDt / 100.0, baseDt);
    }

    static void stepVerlet(std::vector<Body>& bodies, double dt) {
        for (auto& b : bodies) b.velocity += b.acceleration * (dt * 0.5);
        for (auto& b : bodies) b.updatePosition(dt);
        handleCollisions(bodies);
        calculateAccelerations(bodies);
        for (auto& b : bodies) b.velocity += b.acceleration * (dt * 0.5);
    }

    static void stepRK4(std::vector<Body>& bodies, double dt) {
        size_t n = bodies.size();
        std::vector<Vector3> kp(n), kv(n), p(n), v(n), a(n), tmp_p(n);
        std::vector<double> m(n);
        for(size_t i=0; i<n; ++i) { p[i] = bodies[i].position; v[i] = bodies[i].velocity; m[i] = bodies[i].mass; }

        auto getA = [&](const std::vector<Vector3>& pos, std::vector<Vector3>& acc) {
            for(auto& ac : acc) ac = Vector3(0,0,0);
            for(size_t i=0; i<n; ++i) for(size_t j=i+1; j<n; ++j) {
                Vector3 r = pos[j] - pos[i];
                double d3 = std::pow(r.lengthSquared() + Constants::SOFTENING_EPSILON, 1.5);
                Vector3 f = r * (Constants::G / d3);
                acc[i] += f * m[j]; acc[j] -= f * m[i];
            }
        };

        std::vector<Vector3> k1_v(n), k1_a(n), k2_v(n), k2_a(n), k3_v(n), k3_a(n), k4_v(n), k4_a(n);
        getA(p, k1_a); for(size_t i=0; i<n; ++i) k1_v[i] = v[i];
        for(size_t i=0; i<n; ++i) tmp_p[i] = p[i] + k1_v[i] * (dt*0.5);
        getA(tmp_p, k2_a); for(size_t i=0; i<n; ++i) k2_v[i] = v[i] + k1_a[i] * (dt*0.5);
        for(size_t i=0; i<n; ++i) tmp_p[i] = p[i] + k2_v[i] * (dt*0.5);
        getA(tmp_p, k3_a); for(size_t i=0; i<n; ++i) k3_v[i] = v[i] + k2_a[i] * (dt*0.5);
        for(size_t i=0; i<n; ++i) tmp_p[i] = p[i] + k3_v[i] * dt;
        getA(tmp_p, k4_a); for(size_t i=0; i<n; ++i) k4_v[i] = v[i] + k3_a[i] * dt;

        for(size_t i=0; i<n; ++i) {
            bodies[i].position += (k1_v[i] + k2_v[i]*2.0 + k3_v[i]*2.0 + k4_v[i]) * (dt/6.0);
            bodies[i].velocity += (k1_a[i] + k2_a[i]*2.0 + k3_a[i]*2.0 + k4_a[i]) * (dt/6.0);
        }
        handleCollisions(bodies);
        calculateAccelerations(bodies);
    }

    static void stepBarnesHut(std::vector<Body>& bodies, double dt, double theta = 0.5) {
        Vector3 minB(1e18, 1e18, 1e18), maxB(-1e18, -1e18, -1e18);
        for (const auto& b : bodies) {
            minB.x = std::min(minB.x, b.position.x); minB.y = std::min(minB.y, b.position.y); minB.z = std::min(minB.z, b.position.z);
            maxB.x = std::max(maxB.x, b.position.x); maxB.y = std::max(maxB.y, b.position.y); maxB.z = std::max(maxB.z, b.position.z);
        }
        double s = std::max({maxB.x - minB.x, maxB.y - minB.y, maxB.z - minB.z}) * 0.5 + 0.1;
        Vector3 mid = (minB + maxB) * 0.5;
        OctreeNode root(mid - Vector3(s,s,s), mid + Vector3(s,s,s));
        for (auto& b : bodies) root.insert(&b);

        for (auto& b : bodies) b.velocity += b.acceleration * (dt * 0.5);
        for (auto& b : bodies) b.updatePosition(dt);
        handleCollisions(bodies);
        for (auto& b : bodies) { Vector3 f(0,0,0); root.calculateForce(&b, theta, f); b.acceleration = f / b.mass; }
        for (auto& b : bodies) b.velocity += b.acceleration * (dt * 0.5);
    }

    static double calculateTotalEnergy(const std::vector<Body>& bodies) {
        double k = 0, p = 0;
        for (size_t i = 0; i < bodies.size(); ++i) {
            k += 0.5 * bodies[i].mass * bodies[i].velocity.lengthSquared();
            for (size_t j = i + 1; j < bodies.size(); ++j)
                p -= (Constants::G * bodies[i].mass * bodies[j].mass) / ((bodies[j].position - bodies[i].position).length() + 1e-4);
        }
        return k + p;
    }
};

} // namespace SolarSim
