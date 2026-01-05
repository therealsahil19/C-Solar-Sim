#ifndef OCTREE_HPP
#define OCTREE_HPP

#include <vector>
#include <memory>
#include <stack>
#include "Vector3.hpp"
#include "Body.hpp"
#include "Constants.hpp"

namespace SolarSim {

struct OctreeNode {
    Vector3 centerOfMass;
    double totalMass;
    Vector3 minBounds;
    double size;

    int children[8]; // Indices in pool, -1 if none
    Body* bodies[1]; // Using fixed size for simplicity in pooled nodes
    int numBodies;
    bool isLeaf;

    OctreeNode() : centerOfMass(0,0,0), totalMass(0), size(0), numBodies(0), isLeaf(true) {
        for (int i = 0; i < 8; ++i) children[i] = -1;
    }

    void reset(Vector3 minB, double s) {
        minBounds = minB;
        size = s;
        centerOfMass = Vector3(0,0,0);
        totalMass = 0;
        numBodies = 0;
        isLeaf = true;
        for (int i = 0; i < 8; ++i) children[i] = -1;
    }
};

class OctreePool {
private:
    std::vector<OctreeNode> pool;
    int nextFree;
    mutable std::vector<int> traversalStack;  // Pre-allocated for performance

public:
    OctreePool(size_t initialCapacity = 1024) : nextFree(0) {
        pool.resize(initialCapacity);
        traversalStack.reserve(256);  // Pre-allocate reasonable stack depth
    }

    void clear() { nextFree = 0; }

    int allocate(Vector3 minB, double size) {
        if (nextFree >= (int)pool.size()) {
            pool.resize(pool.size() * 2);
        }
        int idx = nextFree++;
        pool[idx].reset(minB, size);
        return idx;
    }

    OctreeNode& operator[](int idx) { return pool[idx]; }
    const OctreeNode& operator[](int idx) const { return pool[idx]; }

    void insert(int nodeIdx, Body* body) {
        if (pool[nodeIdx].isLeaf) {
            if (pool[nodeIdx].numBodies == 0) {
                pool[nodeIdx].bodies[0] = body;
                pool[nodeIdx].numBodies = 1;
                pool[nodeIdx].totalMass = body->mass;
                pool[nodeIdx].centerOfMass = body->position;
            } else {
                Body* existingBody = pool[nodeIdx].bodies[0];
                pool[nodeIdx].isLeaf = false;
                pool[nodeIdx].numBodies = 0;
                insertIntoChild(nodeIdx, existingBody);
                insertIntoChild(nodeIdx, body);
            }
        } else {
            insertIntoChild(nodeIdx, body);
            // Update center of mass
            OctreeNode& node = pool[nodeIdx];
            node.centerOfMass = (node.centerOfMass * node.totalMass + body->position * body->mass) / (node.totalMass + body->mass);
            node.totalMass += body->mass;
        }
    }

    void insertIntoChild(int nodeIdx, Body* body) {
        double halfSize = pool[nodeIdx].size * 0.5;
        Vector3 mid = pool[nodeIdx].minBounds + Vector3(halfSize, halfSize, halfSize);
        
        int idx = 0;
        if (body->position.x >= mid.x) idx |= 1;
        if (body->position.y >= mid.y) idx |= 2;
        if (body->position.z >= mid.z) idx |= 4;

        if (pool[nodeIdx].children[idx] == -1) {
            Vector3 cMin = pool[nodeIdx].minBounds;
            if (idx & 1) cMin.x += halfSize;
            if (idx & 2) cMin.y += halfSize;
            if (idx & 4) cMin.z += halfSize;
            int childIdx = allocate(cMin, halfSize);
            pool[nodeIdx].children[idx] = childIdx;
        }
        insert(pool[nodeIdx].children[idx], body);
    }

    void calculateForceIterative(int rootIdx, Body* body, double theta, Vector3& totalForce) const {
        traversalStack.clear();
        traversalStack.push_back(rootIdx);

        while (!traversalStack.empty()) {
            int nodeIdx = traversalStack.back();
            traversalStack.pop_back();
            const OctreeNode& node = pool[nodeIdx];

            if (node.isLeaf) {
                if (node.numBodies > 0 && node.bodies[0] != body) {
                    Vector3 r = node.bodies[0]->position - body->position;
                    double d2 = r.lengthSquared() + 1e-4;
                    double invD3 = 1.0 / (d2 * std::sqrt(d2));
                    totalForce += r * (Constants::G * body->mass * node.bodies[0]->mass * invD3);
                }
            } else {
                double dist = (node.centerOfMass - body->position).length();
                if (node.size / dist < theta) {
                    Vector3 r = node.centerOfMass - body->position;
                    double d2 = r.lengthSquared() + 1e-4;
                    double invD3 = 1.0 / (d2 * std::sqrt(d2));
                    totalForce += r * (Constants::G * body->mass * node.totalMass * invD3);
                } else {
                    for (int i = 0; i < 8; ++i) {
                        if (node.children[i] != -1) traversalStack.push_back(node.children[i]);
                    }
                }
            }
        }
    }
};

} // namespace SolarSim

#endif
