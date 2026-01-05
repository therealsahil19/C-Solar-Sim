#pragma once

#include "glad.h"

#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <vector>
#include <map>
#include <cmath>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Body.hpp"
#include "Constants.hpp"
#include "Camera3D.hpp"
#include "ShaderProgram.hpp"
#include "SphereRenderer.hpp"

namespace SolarSim {

/**
 * @brief OpenGL 3D rendering engine with Phong lighting.
 */
class GraphicsEngine {
private:
    sf::RenderWindow& window;
    Camera3D camera;
    SphereRenderer sphereRenderer;
    
    ShaderProgram planetShader;
    ShaderProgram sunShader;
    ShaderProgram trailShader;
    
    std::map<std::string, sf::Color> bodyColors;
    std::map<std::string, unsigned int> glTextures;
    
    // Trail rendering
    unsigned int trailVAO = 0, trailVBO = 0;
    
    // Lighting parameters
    float ambientStrength = 0.15f;
    float specularStrength = 0.3f;
    float shininess = 32.0f;
    
    bool initialized = false;
    std::string shaderPath;

    unsigned int loadTextureFromFile(const std::string& path) {
        sf::Image image;
        if (!image.loadFromFile(path)) {
            std::cerr << "Failed to load texture: " << path << std::endl;
            return 0;
        }
        
        unsigned int textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.getSize().x, image.getSize().y,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, image.getPixelsPtr());
        glGenerateMipmap(GL_TEXTURE_2D);
        
        return textureID;
    }

public:
    GraphicsEngine(sf::RenderWindow& win, const std::string& basePath = "") 
        : window(win), shaderPath(basePath.empty() ? "shaders/" : basePath + "/shaders/")
    {
        bodyColors["Sun"] = sf::Color::Yellow;
        bodyColors["Earth"] = sf::Color(100, 149, 237);
        bodyColors["Mars"] = sf::Color(193, 68, 14);
        bodyColors["Jupiter"] = sf::Color(216, 202, 157);
        bodyColors["Saturn"] = sf::Color(225, 216, 168);
        bodyColors["Mercury"] = sf::Color(169, 169, 169);
        bodyColors["Venus"] = sf::Color(255, 198, 73);
        bodyColors["Uranus"] = sf::Color(209, 231, 231);
        bodyColors["Neptune"] = sf::Color(63, 84, 186);
        bodyColors["Moon"] = sf::Color(169, 169, 169);
        bodyColors["Pluto"] = sf::Color(205, 186, 172);
        bodyColors["Ceres"] = sf::Color(155, 155, 155);
        bodyColors["Eris"] = sf::Color(235, 235, 235);
        bodyColors["Makemake"] = sf::Color(200, 150, 120);
        bodyColors["Haumea"] = sf::Color(180, 180, 200);
    }

    bool init() {
        if (initialized) return true;
        
        // Load shaders
        if (!planetShader.loadFromFiles(shaderPath + "planet.vert", shaderPath + "planet.frag")) {
            std::cerr << "Failed to load planet shader" << std::endl;
            return false;
        }
        
        if (!sunShader.loadFromFiles(shaderPath + "planet.vert", shaderPath + "sun.frag")) {
            std::cerr << "Failed to load sun shader" << std::endl;
            return false;
        }
        
        if (!trailShader.loadFromFiles(shaderPath + "trail.vert", shaderPath + "trail.frag")) {
            std::cerr << "Failed to load trail shader" << std::endl;
            return false;
        }
        
        // Initialize sphere renderer
        sphereRenderer.init();
        
        // Load textures
        auto loadTex = [&](const std::string& name, const std::string& file) {
            unsigned int tex = loadTextureFromFile("textures/" + file);
            if (tex) glTextures[name] = tex;
        };
        
        loadTex("Sun", "sun.jpg");
        loadTex("Earth", "earth.jpg");
        loadTex("Mars", "mars.jpg");
        loadTex("Jupiter", "jupiter.jpg");
        loadTex("Saturn", "saturn.jpg");
        loadTex("Mercury", "mercury.jpg");
        loadTex("Venus", "venus.jpg");
        loadTex("Uranus", "uranus.jpg");
        loadTex("Neptune", "neptune.jpg");
        loadTex("Moon", "moon.jpg");
        loadTex("Pluto", "pluto.jpg");
        
        // Create trail VAO/VBO
        glGenVertexArrays(1, &trailVAO);
        glGenBuffers(1, &trailVBO);
        
        // Enable OpenGL features
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        initialized = true;
        return true;
    }

    Camera3D& getCamera() { return camera; }
    
    // Expose controls for GUI compatibility
    void exposeControls(float*& scalePtr, float*& rotXPtr, float*& rotZPtr) {
        scalePtr = camera.getDistancePtr();
        rotXPtr = camera.getPitchPtr();
        rotZPtr = camera.getYawPtr();
    }
    
    float* getAmbientPtr() { return &ambientStrength; }
    float* getSpecularPtr() { return &specularStrength; }
    float* getShininessPtr() { return &shininess; }

    void handleEvent(const sf::Event& event) {
        camera.handleEvent(event);
    }

    void render(const std::vector<Body>& bodies, bool showTrails = true, bool showAxes = true) {
        if (!initialized) {
            if (!init()) return;
        }
        
        // Clear buffers
        glClearColor(0.02f, 0.02f, 0.06f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Get matrices
        sf::Vector2u winSize = window.getSize();
        float aspect = (float)winSize.x / (float)winSize.y;
        
        camera.update();
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = camera.getProjectionMatrix(aspect);
        glm::vec3 camPos = camera.getPosition();
        
        // Sun position (always at origin for lighting)
        glm::vec3 lightPos(0.0f, 0.0f, 0.0f);
        
        // Draw coordinate axes if enabled
        if (showAxes) {
            drawAxes(view, projection);
        }
        
        // Draw trails first (behind planets)
        if (showTrails) {
            drawTrails(bodies, view, projection);
        }
        
        // Draw all bodies
        for (const auto& body : bodies) {
            drawBody(body, view, projection, camPos, lightPos);
        }
        
        // Reset OpenGL state for ImGui
        glBindVertexArray(0);
        glUseProgram(0);
    }

private:
    void drawBody(const Body& body, const glm::mat4& view, const glm::mat4& projection,
                  const glm::vec3& camPos, const glm::vec3& lightPos) {
        
        // Get color
        sf::Color sfColor = bodyColors.count(body.name) ? bodyColors.at(body.name) : sf::Color::White;
        glm::vec3 color(sfColor.r / 255.0f, sfColor.g / 255.0f, sfColor.b / 255.0f);
        
        // Calculate visual radius (scaled for visibility)
        float visualRadius = (float)body.radius * 50.0f;  // Scale up for visibility
        if (body.name == "Sun") visualRadius = 0.8f;
        else if (body.name == "Asteroid") visualRadius = 0.02f;
        else visualRadius = std::max(0.05f, std::min(visualRadius, 0.5f));
        
        // Model matrix
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3((float)body.position.x, 
                                                  (float)body.position.z,  // Y-up convention
                                                  (float)body.position.y));
        
        // Apply axial tilt
        if (body.axialTilt != 0) {
            model = glm::rotate(model, glm::radians((float)body.axialTilt), glm::vec3(0.0f, 0.0f, 1.0f));
        }
        
        // Apply rotation
        model = glm::rotate(model, glm::radians((float)body.rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        
        // Scale
        model = glm::scale(model, glm::vec3(visualRadius));
        
        // Normal matrix for lighting
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
        
        // Choose shader
        bool isSun = (body.name == "Sun");
        ShaderProgram& shader = isSun ? sunShader : planetShader;
        shader.use();
        
        // Set uniforms
        shader.setMat4("model", model);
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        shader.setVec3("objectColor", color);
        
        // Check for texture
        bool hasTexture = glTextures.count(body.name) > 0;
        shader.setBool("useTexture", hasTexture);
        if (hasTexture) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, glTextures.at(body.name));
            shader.setInt("planetTexture", 0);
        }
        
        if (isSun) {
            shader.setFloat("glowIntensity", 1.5f);
        } else {
            shader.setMat3("normalMatrix", normalMatrix);
            shader.setVec3("lightPos", lightPos);
            shader.setVec3("viewPos", camPos);
            shader.setFloat("ambientStrength", ambientStrength);
            shader.setFloat("specularStrength", specularStrength);
            shader.setFloat("shininess", shininess);
        }
        
        sphereRenderer.draw();
    }
    
    void drawTrails(const std::vector<Body>& bodies, const glm::mat4& view, const glm::mat4& projection) {
        trailShader.use();
        trailShader.setMat4("view", view);
        trailShader.setMat4("projection", projection);
        
        for (const auto& body : bodies) {
            if (body.trail.size() < 2 || body.name == "Asteroid") continue;
            
            sf::Color sfColor = bodyColors.count(body.name) ? bodyColors.at(body.name) : sf::Color::White;
            
            // Build trail vertices with fading alpha
            std::vector<float> vertices;
            for (size_t i = 0; i < body.trail.size(); ++i) {
                float alpha = 0.4f * (float)i / (float)body.trail.size();
                vertices.push_back((float)body.trail[i].x);
                vertices.push_back((float)body.trail[i].z);  // Y-up
                vertices.push_back((float)body.trail[i].y);
                vertices.push_back(sfColor.r / 255.0f);
                vertices.push_back(sfColor.g / 255.0f);
                vertices.push_back(sfColor.b / 255.0f);
                vertices.push_back(alpha);
            }
            
            glBindVertexArray(trailVAO);
            glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
            
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
            
            glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)(body.trail.size()));
        }
    }
    
    void drawAxes(const glm::mat4& view, const glm::mat4& projection) {
        trailShader.use();
        trailShader.setMat4("view", view);
        trailShader.setMat4("projection", projection);
        
        // Axis lines: X (red), Y (green), Z (blue) - 10 AU each direction
        float axisVertices[] = {
            // X axis (red)
            -10.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.5f,
             10.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.5f,
            // Y axis (green) 
            0.0f, -10.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.5f,
            0.0f,  10.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.5f,
            // Z axis (blue)
            0.0f, 0.0f, -10.0f, 0.0f, 0.0f, 0.4f, 0.5f,
            0.0f, 0.0f,  10.0f, 0.0f, 0.0f, 0.4f, 0.5f,
        };
        
        glBindVertexArray(trailVAO);
        glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(axisVertices), axisVertices, GL_DYNAMIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glDrawArrays(GL_LINES, 0, 6);
    }

public:
    ~GraphicsEngine() {
        if (trailVAO) glDeleteVertexArrays(1, &trailVAO);
        if (trailVBO) glDeleteBuffers(1, &trailVBO);
        for (auto& [name, tex] : glTextures) {
            glDeleteTextures(1, &tex);
        }
    }
};

} // namespace SolarSim
