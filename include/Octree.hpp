#pragma once

#include <vector>
#include <memory>
#include "Vector3.hpp"
#include "Body.hpp"

namespace SolarSim {

/**
 * @brief An Octree node for Barnes-Hut optimization.
 */
struct OctreeNode {
    Vector3 centerOfMass;
    double totalMass;
    Vector3 minBounds, maxBounds;
    double size;

    std::vector<Body*> bodies; // If leaf node
    std::unique_ptr<OctreeNode> children[8];
    bool isLeaf;

    OctreeNode(Vector3 minB, Vector3 maxB) 
        : centerOfMass(0,0,0), totalMass(0), minBounds(minB), maxBounds(maxB), isLeaf(true) {
        size = maxBounds.x - minBounds.x;
        for (int i = 0; i < 8; ++i) children[i] = nullptr;
    }

    void insert(Body* body) {
        if (isLeaf) {
            if (bodies.empty()) {
                bodies.push_back(body);
                totalMass = body->mass;
                centerOfMass = body->position;
            } else {
                subdivide();
                for (auto* b : bodies) insertIntoChild(b);
                bodies.clear();
                insertIntoChild(body);
            }
        } else {
            insertIntoChild(body);
            // Update center of mass
            centerOfMass = (centerOfMass * totalMass + body->position * body->mass) / (totalMass + body->mass);
            totalMass += body->mass;
        }
    }

    void insertIntoChild(Body* body) {
        Vector3 mid = (minBounds + maxBounds) * 0.5;
        int idx = 0;
        if (body->position.x >= mid.x) idx |= 1;
        if (body->position.y >= mid.y) idx |= 2;
        if (body->position.z >= mid.z) idx |= 4;

        if (!children[idx]) {
            Vector3 cMin = minBounds, cMax = maxBounds;
            if (idx & 1) cMin.x = mid.x; else cMax.x = mid.x;
            if (idx & 2) cMin.y = mid.y; else cMax.y = mid.y;
            if (idx & 4) cMin.z = mid.z; else cMax.z = mid.z;
            children[idx] = std::make_unique<OctreeNode>(cMin, cMax);
        }
        children[idx]->insert(body);
    }

    void subdivide() {
        isLeaf = false;
    }

    void calculateForce(Body* body, double theta, Vector3& totalForce) {
        if (isLeaf) {
            if (!bodies.empty() && bodies[0] != body) {
                Vector3 r_vec = bodies[0]->position - body->position;
                double distSq = r_vec.lengthSquared() + 1e-4; // Softening
                double forceMag = (39.478 * body->mass * bodies[0]->mass) / distSq;
                totalForce += r_vec.normalized() * forceMag;
            }
        } else {
            double dist = (centerOfMass - body->position).length();
            if (size / dist < theta) {
                Vector3 r_vec = centerOfMass - body->position;
                double distSq = r_vec.lengthSquared() + 1e-4;
                double forceMag = (39.478 * body->mass * totalMass) / distSq;
                totalForce += r_vec.normalized() * forceMag;
            } else {
                for (int i = 0; i < 8; ++i) {
                    if (children[i]) children[i]->calculateForce(body, theta, totalForce);
                }
            }
        }
    }
};

} // namespace SolarSim
