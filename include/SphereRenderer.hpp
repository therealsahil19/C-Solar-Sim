#pragma once

#include <vector>
#include <cmath>

#include "glad.h"

#include <glm/glm.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace SolarSim {

/**
 * @brief Generates and renders UV sphere geometry.
 */
class SphereRenderer {
private:
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    unsigned int indexCount = 0;
    bool initialized = false;

    // Sphere resolution
    int latitudeSegments;
    int longitudeSegments;

public:
    SphereRenderer(int latSegs = 16, int lonSegs = 16)
        : latitudeSegments(latSegs), longitudeSegments(lonSegs)
    {}

    void init() {
        if (initialized) return;

        std::vector<float> vertices;
        std::vector<unsigned int> indices;

        // Generate sphere vertices
        for (int lat = 0; lat <= latitudeSegments; ++lat) {
            float theta = (float)lat * (float)M_PI / (float)latitudeSegments;
            float sinTheta = sin(theta);
            float cosTheta = cos(theta);

            for (int lon = 0; lon <= longitudeSegments; ++lon) {
                float phi = (float)lon * 2.0f * (float)M_PI / (float)longitudeSegments;
                float sinPhi = sin(phi);
                float cosPhi = cos(phi);

                // Position
                float x = cosPhi * sinTheta;
                float y = cosTheta;
                float z = sinPhi * sinTheta;

                // Normal (same as position for unit sphere)
                float nx = x;
                float ny = y;
                float nz = z;

                // UV coordinates
                float u = 1.0f - (float)lon / (float)longitudeSegments;
                // Fix inverted texture: OpenGL 0,0 is bottom-left, but SFML loads images top-down.
                // We flip V so that lat=0 (North Pole) maps to V=0 (Row 0 of image data).
                float v = (float)lat / (float)latitudeSegments;

                // Add vertex: position (3) + normal (3) + texcoord (2)
                vertices.push_back(x);
                vertices.push_back(y);
                vertices.push_back(z);
                vertices.push_back(nx);
                vertices.push_back(ny);
                vertices.push_back(nz);
                vertices.push_back(u);
                vertices.push_back(v);
            }
        }

        // Generate indices
        for (int lat = 0; lat < latitudeSegments; ++lat) {
            for (int lon = 0; lon < longitudeSegments; ++lon) {
                int first = lat * (longitudeSegments + 1) + lon;
                int second = first + longitudeSegments + 1;

                // Two triangles per quad
                indices.push_back(first);
                indices.push_back(second);
                indices.push_back(first + 1);

                indices.push_back(second);
                indices.push_back(second + 1);
                indices.push_back(first + 1);
            }
        }

        indexCount = (unsigned int)indices.size();

        // Create OpenGL buffers
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // Texture coordinate attribute
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);

        initialized = true;
    }

    void draw() const {
        if (!initialized) return;
        glBindVertexArray(VAO);
        // Explicitly re-bind VBO to ensure attribute 0 reads from correct buffer
        // (protects against external code modifying the VAO's buffer bindings)
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void bindVAO() const {
        if (!initialized) return;
        glBindVertexArray(VAO);
    }

    void drawInstanced(unsigned int count) const {
        if (!initialized) return;
        glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0, count);
    }

    ~SphereRenderer() {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
        if (EBO) glDeleteBuffers(1, &EBO);
    }
};

} // namespace SolarSim
