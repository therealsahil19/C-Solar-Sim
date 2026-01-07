#ifndef OCTREE_HPP
#define OCTREE_HPP

#include <vector>
#include <memory>
#include <stack>
#include "Vector3.hpp"
#include "Body.hpp"
#include "Constants.hpp"

namespace SolarSim {

/**
 * @brief A node in the spatial partitioning Octree.
 * 
 * Each node represents a cubic volume in 3D space. 
 * - **Leaf Node**: Contains pointers to individual `Body` objects.
 * - **Internal Node**: Contains aggregate data (Center of Mass, Total Mass) for all bodies within its volume.
 * 
 * @note This structure is optimized for the Barnes-Hut algorithm.
 */
struct OctreeNode {
    Vector3 centerOfMass; ///< Weighted average position of all bodies in this node
    double totalMass;     ///< Sum of masses of all bodies in this node
    Vector3 minBounds;    ///< Minimum corner of the cubic volume
    double size;          ///< Side length of the cubic volume

    int children[8]; // Indices in pool, -1 if none
    Body* bodies[1]; // Using fixed size for simplicity in pooled nodes
    int numBodies;
    bool isLeaf;

    OctreeNode() : centerOfMass(0,0,0), totalMass(0), size(0), numBodies(0), isLeaf(true) {
        for (int i = 0; i < 8; ++i) children[i] = -1;
    }

    /**
     * @brief Resets the node state for reuse in the memory pool.
     * @param minB The minimum corner coordinates of the cubic volume.
     * @param s The side length of the cubic volume.
     */
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

/**
 * @brief Memory-pooled Octree implementation for performance-critical N-body simulations.
 * 
 * To avoid the high cost of dynamic memory allocation and pointer chasing during 
 * high-frequency tree builds, this class uses a contiguous pool of OctreeNode objects.
 * 
 * @perf
 * - **Heap Stability**: No `new`/`delete` calls during simulation steps.
 * - **Cache Locality**: Nodes are stored contiguously in memory, improving CPU cache hit rates.
 * 
 * @physics
 * Supports the **Barnes-Hut algorithm**, which approximates gravitational
 * forces from distant clusters as a single force from their center of mass, 
 * reducing complexity from $O(N^2)$ to $O(N \log N)$.
 */
class OctreePool {
private:
    std::vector<OctreeNode> pool;
    int nextFree;
    mutable std::vector<int> traversalStack;  ///< Reuse stack memory for iterative traversal

public:
    OctreePool(size_t initialCapacity = 1024) : nextFree(0) {
        pool.resize(initialCapacity);
        traversalStack.reserve(256);  // Pre-allocate reasonable stack depth
    }

    /**
     * @brief Resets the pool without deallocating memory.
     */
    void clear() { nextFree = 0; }

    /**
     * @brief Allocates a node from the pool.
     */
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

    /**
     * @brief Inserts a body into the appropriate octant of a node.
     * 
     * @details
     * The octant index (0-7) is determined using bit-masking on the coordinates:
     * - **Bit 0 (1)**: X-axis (0: left, 1: right)
     * - **Bit 1 (2)**: Y-axis (0: bottom, 1: top)
     * - **Bit 2 (4)**: Z-axis (0: back, 1: front)
     * 
     * For example, an index of 3 (binary 011) represents (+X, +Y, -Z).
     * 
     * @param nodeIdx Index of parent node in pool
     * @param body Pointer to body to insert
     */
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

    /**
     * @brief Calculates gravitational force on a body using an iterative tree traversal.
     * 
     * Uses the Barnes-Hut approximation:
     * If the distance $d$ between the body and node's center of mass satisfies 
     * $s/d < \theta$ (where $s$ is node size), the entire subtree is treated as a 
     * single particle at the center of mass.
     * 
     * @param rootIdx Index of the tree root in the pool
     * @param body The body to calculate forces for
     * @param theta Accuracy threshold (Openness parameter)
     * @param totalForce Output accumulator for the force vector
     */
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
