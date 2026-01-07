#pragma once

#include <vector>
#include <iostream>
#include <algorithm>
#include <cmath>
#include "Body.hpp"
#include "Constants.hpp"
#include "Octree.hpp"

#include <immintrin.h>

namespace SolarSim {

/**
 * @brief Static physics library for gravitational calculations.
 */
class PhysicsEngine {
public:
    /**
     * @brief Calculates gravitational force between two bodies using Newton's Law of Universal Gravitation.
     * 
     * Formula: F = G * (m1 * m2) / r^2
     * Acceleration a = F / m
     * 
     * @param a Body 1
     * @param b Body 2
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
     * @brief Calculates accelerations for all bodies with optimized cache access.
     * 
     * Optimization notes:
     * - Extracts position components to local variables to reduce struct access
     * - Accumulates accelerations locally before writing back
     * - Computes invDist once and derives invDist3 from it (faster than 1/sqrt^3)
     * 
     * @note This is an O(N^2) implementation. For large N, use Barnes-Hut.
     */
    static void calculateAccelerations(std::vector<Body>& bodies) {
        for (auto& body : bodies) body.resetAcceleration();
        
        const size_t n = bodies.size();
        for (size_t i = 0; i < n; ++i) {
            Body& bi = bodies[i];
            // Cache position components locally for better cache performance
            const double xi = bi.position.x;
            const double yi = bi.position.y;
            const double zi = bi.position.z;
            const double mi = bi.mass;
            
            // Local accumulator for acceleration
            double axi = 0.0, ayi = 0.0, azi = 0.0;
            
            for (size_t j = i + 1; j < n; ++j) {
                Body& bj = bodies[j];
                const double mj = bj.mass;
                
                // Compute delta components
                const double dx = bj.position.x - xi;
                const double dy = bj.position.y - yi;
                const double dz = bj.position.z - zi;
                
                // Distance calculation with softening
                const double distSq = dx*dx + dy*dy + dz*dz + Constants::SOFTENING_EPSILON;
                const double invDist = 1.0 / std::sqrt(distSq);
                const double invDist3 = invDist * invDist * invDist;
                const double f = Constants::G * invDist3;
                
                // Accumulate forces (scaled by respective masses)
                const double fx = dx * f;
                const double fy = dy * f;
                const double fz = dz * f;
                
                axi += fx * mj;
                ayi += fy * mj;
                azi += fz * mj;
                
                bj.acceleration.x -= fx * mi;
                bj.acceleration.y -= fy * mi;
                bj.acceleration.z -= fz * mi;
            }
            
            // Write back accumulated acceleration
            bi.acceleration.x += axi;
            bi.acceleration.y += ayi;
            bi.acceleration.z += azi;
        }
    }

    /**
     * @brief Detects and handles inelastic collisions using momentum conservation.
     * 
     * When two bodies collide (distance < radius_sum), they merge into one.
     * New mass M = m1 + m2
     * New velocity V = (m1*v1 + m2*v2) / M
     * New radius R = (r1^3 + r2^3)^(1/3)  -- Perserving volume
     */
    static void handleCollisions(std::vector<Body>& bodies) {
        for (size_t i = 0; i < bodies.size(); ++i) {
            for (size_t j = i + 1; j < bodies.size(); ++j) {
                double distSq = (bodies[j].position - bodies[i].position).lengthSquared();
                double radiusSum = bodies[i].radius + bodies[j].radius;
                if (distSq < (radiusSum * radiusSum)) {
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
     * @brief Calculates a safe adaptive timestep based on the proximity of bodies.
     * 
     * The timestep is scaled such that bodies moving at high speeds during close 
     * encounters are integrated with higher temporal resolution.
     * 
     * @details
     * We calculate the minimum squared distance between any two bodies. The safe
     * timestep is proportional to the square root of this distance (the collision time):
     * 
     * $$dt_{adaptive} = C \cdot \sqrt{min(r_{ij}^2)}$$
     * 
     * where $C$ is a safety constant (typically 0.01).
     * 
     * @param bodies Collection of celestial bodies
     * @param baseDt The maximum allowable timestep (usually configured by user)
     * @returns A clamped timestep value [baseDt * 0.01, baseDt]
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

    /**
     * @brief Integrates system state using the Velocity Verlet algorithm.
     * 
     * Verlet is a symplectic integrator, meaning it preserves phase-space volume
     * and exhibits excellent long-term energy conservation compared to non-symplectic
     * methods like Euler.
     * 
     * @details
     * The algorithm follows these steps:
     * 1. Update positions: $r(t+dt) = r(t) + v(t)dt + \frac{1}{2}a(t)dt^2$
     * 2. Compute half-step velocity: $v(t+\frac{dt}{2}) = v(t) + \frac{1}{2}a(t)dt$
     * 3. Compute new acceleration: $a(t+dt)$ from $r(t+dt)$
     * 4. Compute full-step velocity: $v(t+dt) = v(t+\frac{dt}{2}) + \frac{1}{2}a(t+dt)dt$
     * 
     * @param bodies Collection of celestial bodies
     * @param dt Timestep in years
     */
    static void stepVerlet(std::vector<Body>& bodies, double dt) {
        for (auto& b : bodies) b.velocity += b.acceleration * (dt * 0.5);
        for (auto& b : bodies) b.updatePosition(dt);
        handleCollisions(bodies);
        calculateAccelerations(bodies);
        for (auto& b : bodies) b.velocity += b.acceleration * (dt * 0.5);
    }

    /**
     * @brief Integrates system state using 4th-order Runge-Kutta (RK4).
     * 
     * RK4 provides a balance between computational cost and high-order accuracy ($O(dt^4)$).
     * It samples the derivatives at four points within the timestep to produce a 
     * weighted average gradient.
     * 
     * @details
     * For a state $y = [pos, vel]$ and $dy/dt = f(t, y)$:
     * - $k_1 = f(t, y)$
     * - $k_2 = f(t + \frac{dt}{2}, y + k_1 \frac{dt}{2})$
     * - $k_3 = f(t + \frac{dt}{2}, y + k_2 \frac{dt}{2})$
     * - $k_4 = f(t + dt, y + k_3 dt)$
     * - $y(t+dt) = y(t) + \frac{dt}{6}(k_1 + 2k_2 + 2k_3 + k_4)$
     * 
     * @param bodies Collection of celestial bodies
     * @param dt Timestep in years
     */
    static void stepRK4(std::vector<Body>& bodies, double dt) {
        size_t n = bodies.size();
        std::vector<Vector3> kp(n), kv(n), p(n), v(n), a(n), tmp_p(n);
        std::vector<double> m(n);

        for(size_t i=0; i<n; ++i) { p[i] = bodies[i].position; v[i] = bodies[i].velocity; m[i] = bodies[i].mass; }

        auto getA = [&](const std::vector<Vector3>& pos, std::vector<Vector3>& acc) {
            for(auto& ac : acc) ac = Vector3(0,0,0);
            for(size_t i=0; i<n; ++i) for(size_t j=i+1; j<n; ++j) {
                Vector3 r = pos[j] - pos[i];
                double distSq = r.lengthSquared() + Constants::SOFTENING_EPSILON;
                double invDist3 = 1.0 / (distSq * std::sqrt(distSq));
                double f_over_r = Constants::G * invDist3;
                Vector3 force_scaled = r * f_over_r;
                acc[i] += force_scaled * m[j]; acc[j] -= force_scaled * m[i];
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

    /**
     * @brief Optimizes force calculation using the Barnes-Hut algorithm (O(N log N)).
     * 
     * For large N simulations, direct O(N^2) gravity is too slow. Barnes-Hut
     * partitions space into an Octree. For distant clusters of bodies, we calculate
     * the force from the cluster's center of mass rather than individual bodies.
     * 
     * @logic
     * 1. Rebuild Octree from current body positions.
     * 2. Calculate COM (Center of Mass) and Total Mass for every node.
     * 3. For each body, traverse tree:
     *    - If node is far enough ($s/d < \theta$), apply approximation.
     *    - Otherwise, recurse into children.
     * 
     * @param bodies Collection of celestial bodies
     * @param dt Timestep in years
     * @param theta Accuracy parameter ($\theta$); lower is more accurate (typically 0.5)
     */
    static void stepBarnesHut(std::vector<Body>& bodies, double dt, double theta = 0.5) {
        static OctreePool pool;
        pool.clear();

        Vector3 minB(1e18, 1e18, 1e18), maxB(-1e18, -1e18, -1e18);
        for (const auto& b : bodies) {
            minB.x = std::min(minB.x, b.position.x); minB.y = std::min(minB.y, b.position.y); minB.z = std::min(minB.z, b.position.z);
            maxB.x = std::max(maxB.x, b.position.x); maxB.y = std::max(maxB.y, b.position.y); maxB.z = std::max(maxB.z, b.position.z);
        }
        double s = std::max({maxB.x - minB.x, maxB.y - minB.y, maxB.z - minB.z}) * 0.5 + 0.1;
        Vector3 mid = (minB + maxB) * 0.5;
        
        int rootIdx = pool.allocate(mid - Vector3(s,s,s), s * 2.0);
        for (auto& b : bodies) pool.insert(rootIdx, &b);

        for (auto& b : bodies) b.velocity += b.acceleration * (dt * 0.5);
        for (auto& b : bodies) b.updatePosition(dt);
        handleCollisions(bodies);
        for (auto& b : bodies) { 
            Vector3 f(0,0,0); 
            pool.calculateForceIterative(rootIdx, &b, theta, f); 
            b.acceleration = f / b.mass; 
        }
        for (auto& b : bodies) b.velocity += b.acceleration * (dt * 0.5);
    }

    /**
     * @brief Calculates the total mechanical energy (Kinetic + Potential) of the system.
     * 
     * Used for verifying simulation stability and energy conservation.
     * Potential energy includes a softening factor to prevent singularities.
     * 
     * @param bodies Collection of celestial bodies
     * @returns Total energy in solar-scale units
     */
    static double calculateTotalEnergy(const std::vector<Body>& bodies) {
        double k = 0, p = 0;
        for (size_t i = 0; i < bodies.size(); ++i) {
            k += 0.5 * bodies[i].mass * bodies[i].velocity.lengthSquared();
            for (size_t j = i + 1; j < bodies.size(); ++j)
                p -= (Constants::G * bodies[i].mass * bodies[j].mass) / ((bodies[j].position - bodies[i].position).length() + Constants::SOFTENING_EPSILON);
        }
        return k + p;
    }
};

} // namespace SolarSim
