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
#include "Camera3D.hpp"
#include "ShaderProgram.hpp"
#include "SphereRenderer.hpp"
#include "Theme.hpp"
#include "OrbitCalculator.hpp"

namespace SolarSim {

/**
 * @brief OpenGL 3D rendering engine with Phong lighting.
 */
class GraphicsEngine {
private:
    sf::RenderWindow& window;
    Camera3D camera;
    SphereRenderer sphereRenderer{64, 64};  // Higher resolution for smooth textures
    
    ShaderProgram planetShader;
    ShaderProgram sunShader;
    ShaderProgram trailShader;
    
    std::map<std::string, sf::Color> bodyColors;
    std::map<std::string, unsigned int> glTextures;
    
    // Trail rendering
    unsigned int trailVAO = 0, trailVBO = 0;
    
    // Instanced rendering for asteroids
    unsigned int asteroidInstanceVBO = 0;
    std::vector<glm::mat4> asteroidMatrices;
    
    // Orbit ellipse rendering
    unsigned int orbitVAO = 0, orbitVBO = 0;
    
    // Lighting parameters
    float ambientStrength = 0.15f;
    float specularStrength = 0.3f;
    float shininess = 32.0f;
    
    bool initialized = false;
    std::string shaderPath;

    // Visual scale multiplier - maps real AU to visual units
    // Real distances: Mercury=0.39, Venus=0.72, Earth=1.0, Mars=1.52, Jupiter=5.2, Saturn=9.5, Uranus=19.2, Neptune=30
    // Desired visual: Mercury=15, Venus=30, Earth=50, Mars=75, Jupiter=200, Saturn=350, Uranus=600, Neptune=900
    static float getVisualScale(const std::string& name) {
        // Scale factors derived from: visual_distance / real_distance
        if (name == "Sun") return 1.0f;  // Origin
        if (name == "Mercury") return 15.0f / 0.39f;   // ~38.5
        if (name == "Venus") return 30.0f / 0.72f;     // ~41.7
        if (name == "Earth") return 50.0f / 1.0f;      // 50
        if (name == "Moon") return 1.0f;               // Special handling relative to Earth
        if (name == "Mars") return 75.0f / 1.52f;      // ~49.3
        if (name == "Asteroid") return 125.0f / 2.7f;  // ~46.3 (center of belt)
        if (name == "Jupiter") return 200.0f / 5.2f;   // ~38.5
        if (name == "Saturn") return 350.0f / 9.54f;   // ~36.7
        if (name == "Uranus") return 600.0f / 19.2f;   // ~31.3
        if (name == "Neptune") return 900.0f / 30.0f;  // 30
        if (name == "Pluto") return 1100.0f / 39.5f;   // ~27.8
        return 40.0f;  // Default scale
    }

    static glm::vec3 getVisualPosition(const Body& body) {
        float scale = getVisualScale(body.name);
        return glm::vec3(
            (float)body.position.x * scale,
            (float)body.position.z * scale,  // Y-up convention
            (float)body.position.y * scale
        );
    }

    static float getVisualRadius(const std::string& name) {
        if (name == "Sun") return 5.0f;
        if (name == "Mercury") return 0.8f;
        if (name == "Venus") return 1.5f;
        if (name == "Earth") return 1.6f;
        if (name == "Moon") return 0.4f;
        if (name == "Mars") return 1.0f;
        if (name == "Jupiter") return 4.0f;
        if (name == "Saturn") return 3.5f;
        if (name == "Uranus") return 2.5f;
        if (name == "Neptune") return 2.4f;
        if (name == "Pluto") return 0.5f;
        if (name == "Asteroid") return 0.3f;
        return 1.0f;  // Default
    }

public:
    // Helper for shared Moon positioning logic
    static glm::vec3 calculateMoonVisualPosition(const Body& moon, const Body& earth) {
        glm::vec3 earthVisualPos = getVisualPosition(earth);
        glm::vec3 earthRealPos(earth.position.x, earth.position.z, earth.position.y);
        glm::vec3 moonRealPos(moon.position.x, moon.position.z, moon.position.y);

        glm::vec3 relativePos = moonRealPos - earthRealPos;
        // Scale the distance significantly so it's visible outside Earth
        // Real distance ~0.00257 AU. Earth Visual Radius = 1.6.
        // We need distance > 2.0. Scale factor ~1500x relative.
        float relativeScale = 2000.0f;

        return earthVisualPos + (relativePos * relativeScale);
    }

private:
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
        // Use unified theme colors
        bodyColors = Theme::getBodyColors();
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

        // Create asteroid instance VBO
        glGenBuffers(1, &asteroidInstanceVBO);
        
        // Create orbit VAO/VBO
        glGenVertexArrays(1, &orbitVAO);
        glGenBuffers(1, &orbitVBO);
        
        // Enable OpenGL features
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        initialized = true;
        return true;
    }

    Camera3D& getCamera() { return camera; }

    glm::mat4 getViewProjectionMatrix() {
        sf::Vector2u winSize = window.getSize();
        float aspect = (float)winSize.x / (float)winSize.y;
        return camera.getProjectionMatrix(aspect) * camera.getViewMatrix();
    }
    
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

    void render(const std::vector<Body>& bodies, bool showTrails = true, bool showPlanetOrbits = true, bool showOtherOrbits = false) {
        if (!initialized) {
            if (!init()) return;
        }
        
        // Clear buffers using theme background
        glClearColor(Theme::Background.x, Theme::Background.y, Theme::Background.z, Theme::Background.w);
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
        
        // Enable blending for trails and orbits (they need transparency)
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Draw trails first (behind planets)
        if (showTrails) {
            drawTrails(bodies, view, projection);
        }
        
        // Draw orbit ellipses
        if (showPlanetOrbits || showOtherOrbits) {
            drawOrbits(bodies, view, projection, showPlanetOrbits, showOtherOrbits);
        }
        
        // Disable blending for opaque bodies (fixes depth buffer issue)
        glDisable(GL_BLEND);
        
        // Find Earth index for Moon positioning
        int earthIndex = -1;
        for (size_t i = 0; i < bodies.size(); ++i) {
            if (bodies[i].name == "Earth") {
                earthIndex = (int)i;
                break;
            }
        }

        // Draw all bodies (except asteroids, which are instanced)
        asteroidMatrices.clear();
        for (const auto& body : bodies) {
            if (body.name == "Asteroid") {
                float visualRadius = getVisualRadius("Asteroid");
                glm::mat4 model = glm::mat4(1.0f);
                glm::vec3 visualPos = getVisualPosition(body);
                model = glm::translate(model, visualPos);
                model = glm::scale(model, glm::vec3(visualRadius));
                asteroidMatrices.push_back(model);
                continue;
            }

            // Special handling for Moon
            if (body.name == "Moon" && earthIndex != -1) {
                glm::vec3 visualPos = calculateMoonVisualPosition(body, bodies[earthIndex]);
                drawBodyInternal(body, visualPos, view, projection, camPos, lightPos);
                continue;
            }

            drawBody(body, view, projection, camPos, lightPos);
        }

        // Render asteroids instanced
        if (!asteroidMatrices.empty()) {
            drawAsteroidsInstanced(view, projection, camPos, lightPos);
        }
        
        // Reset OpenGL state for ImGui
        glBindVertexArray(0);
        glUseProgram(0);
    }

private:
    void drawBody(const Body& body, const glm::mat4& view, const glm::mat4& projection,
                  const glm::vec3& camPos, const glm::vec3& lightPos) {
        glm::vec3 visualPos = getVisualPosition(body);
        drawBodyInternal(body, visualPos, view, projection, camPos, lightPos);
    }

    void drawBodyInternal(const Body& body, const glm::vec3& visualPos,
                          const glm::mat4& view, const glm::mat4& projection,
                          const glm::vec3& camPos, const glm::vec3& lightPos) {
        
        // Get color
        sf::Color sfColor = bodyColors.count(body.name) ? bodyColors.at(body.name) : sf::Color::White;
        glm::vec3 color(sfColor.r / 255.0f, sfColor.g / 255.0f, sfColor.b / 255.0f);
        
        // Use visual radius from lookup
        float visualRadius = getVisualRadius(body.name);
        
        // Model matrix with visual position scaling
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, visualPos);
        
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
            
            // Build trail vertices with fading alpha and visual scaling
            float scale = getVisualScale(body.name);
            std::vector<float> vertices;
            for (size_t i = 0; i < body.trail.size(); ++i) {
                float alpha = 0.4f * (float)i / (float)body.trail.size();
                vertices.push_back((float)body.trail[i].x * scale);
                vertices.push_back((float)body.trail[i].z * scale);  // Y-up
                vertices.push_back((float)body.trail[i].y * scale);
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
    
    void drawAsteroidsInstanced(const glm::mat4& view, const glm::mat4& projection,
                                const glm::vec3& camPos, const glm::vec3& lightPos) {
        planetShader.use();
        planetShader.setMat4("view", view);
        planetShader.setMat4("projection", projection);
        planetShader.setVec3("objectColor", glm::vec3(0.6f, 0.6f, 0.61f));
        planetShader.setBool("useTexture", false);
        planetShader.setBool("isInstanced", true);
        
        planetShader.setVec3("lightPos", lightPos);
        planetShader.setVec3("viewPos", camPos);
        planetShader.setFloat("ambientStrength", ambientStrength);
        planetShader.setFloat("specularStrength", specularStrength);
        planetShader.setFloat("shininess", shininess);

        // Update instance data
        glBindBuffer(GL_ARRAY_BUFFER, asteroidInstanceVBO);
        glBufferData(GL_ARRAY_BUFFER, asteroidMatrices.size() * sizeof(glm::mat4), asteroidMatrices.data(), GL_DYNAMIC_DRAW);

        sphereRenderer.bindVAO();
        
        // Set up instance matrix attributes (location 3)
        // A mat4 takes 4 attribute slots
        for (int i = 0; i < 4; i++) {
            glEnableVertexAttribArray(3 + i);
            glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4) * i));
            glVertexAttribDivisor(3 + i, 1);
        }

        sphereRenderer.drawInstanced((unsigned int)asteroidMatrices.size());

        for (int i = 0; i < 4; i++) {
            glDisableVertexAttribArray(3 + i);
        }
        planetShader.setBool("isInstanced", false);
        glBindVertexArray(0);
    }

    void drawOrbits(const std::vector<Body>& bodies, const glm::mat4& view, const glm::mat4& projection, bool showPlanets, bool showOthers) {
        trailShader.use();
        trailShader.setMat4("view", view);
        trailShader.setMat4("projection", projection);
        
        // Find the Sun (central body) for mu calculation
        double sunMass = 1.0; // Default to 1 solar mass
        for (const auto& body : bodies) {
            if (body.name == "Sun") {
                sunMass = body.mass;
                break;
            }
        }
        double mu = Constants::G * sunMass;
        
        for (const auto& body : bodies) {
            // Skip the Sun and asteroids
            if (body.name == "Sun" || body.name == "Asteroid") continue;

            // Determine if it's a planet
            bool isPlanet = (body.name == "Mercury" || body.name == "Venus" || body.name == "Earth" ||
                            body.name == "Mars" || body.name == "Jupiter" || body.name == "Saturn" ||
                            body.name == "Uranus" || body.name == "Neptune");

            // Filter based on toggles
            if (isPlanet && !showPlanets) continue;
            if (!isPlanet && !showOthers) continue;
            
            // Special handling for Moon (orbits Earth, not Sun)
            if (body.name == "Moon") {
                // For now, Moon orbit is included in "others" if toggle is on
                // but we might need Earth's mass for mu if we want it correct.
                // Re-calculating elements for Moon around Sun result in chaotic lines
                // or very large ellipses. Let's skip Moon for now or just treat as other.
                continue; 
            }
            
            // Calculate orbital elements
            OrbitalElements orbit = OrbitCalculator::calculateElements(body.position, body.velocity, mu);
            
            if (!orbit.isValid) continue;
            
            // Generate orbit path (64 points for smooth ellipse)
            std::vector<Vector3> orbitPoints = OrbitCalculator::generateOrbitPath(orbit, 64);
            
            if (orbitPoints.size() < 2) continue;
            
            // Get body color
            sf::Color sfColor = bodyColors.count(body.name) ? bodyColors.at(body.name) : sf::Color::White;
            
            // Build orbit vertices with semi-transparent color and visual scaling
            float scale = getVisualScale(body.name);
            std::vector<float> vertices;
            for (const auto& pt : orbitPoints) {
                vertices.push_back((float)pt.x * scale);
                vertices.push_back((float)pt.z * scale);  // Y-up convention
                vertices.push_back((float)pt.y * scale);
                vertices.push_back(sfColor.r / 255.0f);
                vertices.push_back(sfColor.g / 255.0f);
                vertices.push_back(sfColor.b / 255.0f);
                vertices.push_back(0.3f);  // Semi-transparent
            }
            
            glBindVertexArray(orbitVAO);
            glBindBuffer(GL_ARRAY_BUFFER, orbitVBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
            
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
            
            glDrawArrays(GL_LINE_LOOP, 0, (GLsizei)orbitPoints.size());
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
        if (orbitVAO) glDeleteVertexArrays(1, &orbitVAO);
        if (orbitVBO) glDeleteBuffers(1, &orbitVBO);
        for (auto& [name, tex] : glTextures) {
            glDeleteTextures(1, &tex);
        }
    }
};

} // namespace SolarSim

