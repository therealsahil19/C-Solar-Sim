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

public:
    /**
     * @brief Visual scale multiplier - maps real AU to visual units.
     * 
     * @philosophy
     * **The Scaling Bridge**: Space is mostly empty (99.999% vacuum). If we rendered 
     * the system to scale, the Sun would be a sub-pixel speck and planets would be 
     * invisible. To create a "Visual Literacy" of the system, we use a **Hybrid 
     * Log-Linear Scaling**:
     * 
     * 1. **Linear Scaling (Inner)**: Mercury to Mars use linear offsets to preserve 
     *    the feeling of the rocky inner system.
     * 2. **Logarithmic Compression (Outer)**: Distant giants (Jupiter to Eris) are 
     *    pulled inward using a log-based mapping, allowing the user to see the entire 
     *    family of planets in one view without Neptune being 30x further than Earth.
     * 3. **Size Magnification**: Planetary radii are magnified by ~1000x relative 
     *    to orbital distances so they appear as discs rather than points.
     * 
     * @param name Name of the celestial body
     * @returns The visual scale factor
     */
    static float getVisualScale(const std::string& name) {
        // Scale factors derived from: visual_distance / real_distance
        if (name == "Sun") return 1.0f;  // Origin
        if (name == "Mercury") return 30.0f / 0.39f;    // ~76.9
        if (name == "Venus") return 75.0f / 0.72f;      // ~104.2
        if (name == "Earth") return 130.0f / 1.0f;      // 130
        if (name == "Moon") return 1.0f;                // Special handling relative to Earth
        if (name == "Mars") return 200.0f / 1.52f;      // ~131.6
        if (name == "Asteroid") return 280.0f / 2.7f;   // ~103.7 (center of belt 240-320)
        if (name == "Ceres") return 280.0f / 2.77f;     // ~101.1 (in asteroid belt at 2.77 AU)
        if (name == "Jupiter") return 600.0f / 5.2f;    // ~115.4
        if (name == "Saturn") return 950.0f / 9.54f;    // ~99.6
        if (name == "Uranus") return 1350.0f / 19.2f;   // ~70.3
        if (name == "Neptune") return 1900.0f / 30.0f;  // ~63.3
        if (name == "Pluto") return 2500.0f / 39.5f;    // ~63.3 (just beyond Neptune)
        if (name == "Haumea") return 2700.0f / 43.1f;   // ~62.6 (at 43 AU)
        if (name == "Makemake") return 2900.0f / 45.8f; // ~63.3 (at 45.8 AU)
        if (name == "Eris") return 4300.0f / 67.7f;     // ~63.5 (most distant at 68 AU)
        return 63.0f;  // Default scale for other celestial bodies
    }

    static glm::vec3 getVisualPosition(const Vector3& pos, const std::string& name) {
        float scale = getVisualScale(name);
        return glm::vec3(
            (float)pos.x * scale,
            (float)pos.z * scale,  // Y-up convention
            (float)pos.y * scale
        );
    }

    static float getVisualRadius(const std::string& name) {
        if (name == "Sun") return 5.0f;
        if (name == "Mercury") return 0.8f;
        if (name == "Venus") return 1.5f;
        if (name == "Earth") return 1.6f;
        if (name == "Moon") return 0.4f;  // ~25% of Earth
        if (name == "Mars") return 1.0f;
        if (name == "Jupiter") return 4.0f;
        if (name == "Saturn") return 3.5f;
        if (name == "Uranus") return 2.5f;
        if (name == "Neptune") return 2.4f;
        if (name == "Pluto") return 0.5f;
        if (name == "Asteroid") return 0.3f;
        // Jupiter's moons (relative to Jupiter's visual radius of 4.0)
        if (name == "Io") return 0.5f;        // 2.6% of Jupiter -> 0.1, but make visible
        if (name == "Europa") return 0.45f;   // 2.2% of Jupiter
        if (name == "Ganymede") return 0.6f;  // 3.7% of Jupiter, largest moon
        if (name == "Callisto") return 0.55f; // 3.4% of Jupiter
        // Saturn's moon
        if (name == "Titan") return 0.6f;     // 4.4% of Saturn, larger than Mercury
        // Neptune's moon
        if (name == "Triton") return 0.4f;    // 5.5% of Neptune
        return 0.5f;  // Default for moons
    }

    /**
     * @brief Returns the parent planet name for a given moon.
     */
    static std::string getParentPlanet(const std::string& moonName) {
        if (moonName == "Moon") return "Earth";
        if (moonName == "Io" || moonName == "Europa" || 
            moonName == "Ganymede" || moonName == "Callisto") return "Jupiter";
        if (moonName == "Titan") return "Saturn";
        if (moonName == "Triton") return "Neptune";
        return "";  // Not a moon
    }

    /**
     * @brief Calculates visual position for any satellite relative to its parent planet.
     */
    static glm::vec3 calculateSatelliteVisualPosition(const Body& satellite, const Body& parent) {
        glm::vec3 parentVisualPos = getVisualPosition(parent.position, parent.name);
        
        // Get real positions in Y-up convention
        glm::vec3 parentRealPos(parent.position.x, parent.position.z, parent.position.y);
        glm::vec3 satRealPos(satellite.position.x, satellite.position.z, satellite.position.y);
        
        glm::vec3 relativePos = satRealPos - parentRealPos;
        
        // Scale factor to make moons visible around their parent
        // Different scales based on parent planet size and moon orbital distance
        float relativeScale = 1500.0f;  // Default for Earth's Moon
        
        if (satellite.name == "Io" || satellite.name == "Europa" || 
            satellite.name == "Ganymede" || satellite.name == "Callisto") {
            // Jupiter's moons are further out, use larger scale
            relativeScale = 800.0f;
        } else if (satellite.name == "Titan") {
            relativeScale = 600.0f;
        } else if (satellite.name == "Triton") {
            relativeScale = 1000.0f;
        }
        
        return parentVisualPos + (relativePos * relativeScale);
    }

    // Legacy helper for backward compatibility
    static glm::vec3 calculateMoonVisualPosition(const Body& moon, const Body& earth) {
        return calculateSatelliteVisualPosition(moon, earth);
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
        
        // ============= PASS 1: OPAQUE OBJECTS (Planets) =============
        /**
         * @details
         * Solid spheres are rendered first with full depth writing. This provides 
         * the "occlusion skeleton" for the scene.
         * - **Depth Testing**: Enabled (GL_LESS)
         * - **Depth Writing**: Enabled (GL_TRUE)
         * - **Blending**: Disabled
         */
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE); 
        glDisable(GL_BLEND); 
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        
        // Find parent planet indices for moon positioning
        std::map<std::string, int> parentIndices;
        for (size_t i = 0; i < bodies.size(); ++i) {
            const std::string& name = bodies[i].name;
            if (name == "Earth" || name == "Jupiter" || name == "Saturn" || name == "Neptune") {
                parentIndices[name] = (int)i;
            }
        }

        // Draw all bodies (except asteroids, which are instanced)
        asteroidMatrices.clear();
        for (const auto& body : bodies) {
            if (body.name == "Asteroid") {
                float visualRadius = getVisualRadius("Asteroid");
                glm::mat4 model = glm::mat4(1.0f);
                glm::vec3 visualPos = getVisualPosition(body.position, body.name);
                model = glm::translate(model, visualPos);
                model = glm::scale(model, glm::vec3(visualRadius));
                asteroidMatrices.push_back(model);
                continue;
            }

            // Check if this is a moon that needs satellite positioning
            std::string parentName = getParentPlanet(body.name);
            if (!parentName.empty() && parentIndices.count(parentName)) {
                int parentIdx = parentIndices[parentName];
                glm::vec3 visualPos = calculateSatelliteVisualPosition(body, bodies[parentIdx]);
                drawBodyInternal(body.name, visualPos, body.axialTilt, (float)body.rotationAngle, view, projection, camPos, lightPos);
                continue;
            }

            drawBody(body, view, projection, camPos, lightPos);
        }

        // Render asteroids instanced
        if (!asteroidMatrices.empty()) {
            drawAsteroidsInstanced(view, projection, camPos, lightPos);
        }
        
        // ============= PASS 2: TRANSPARENT OBJECTS (Trails/Orbits) =============
        /**
         * @details
         * Semi-transparent trails and orbit lines must be drawn AFTER opaque objects. 
         * They sample the depth buffer (to hide behind planets) but DO NOT write 
         * to it (to avoid occluding each other).
         * - **Depth Testing**: Enabled
         * - **Depth Writing**: Disabled (GL_FALSE)
         * - **Blending**: Enabled (Alpha Blending)
         */
        glDepthMask(GL_FALSE);  
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE); 
        
        // Draw trails
        if (showTrails) {
            drawTrails(bodies, view, projection);
        }
        
        // Draw orbit ellipses
        if (showPlanetOrbits || showOtherOrbits) {
            drawOrbits(bodies, view, projection, showPlanetOrbits, showOtherOrbits);
        }
        
        
        // Restore depth mask for next frame
        glDepthMask(GL_TRUE);
        
        // Reset OpenGL state for ImGui
        glBindVertexArray(0);
        glUseProgram(0);
    }

private:
    void drawBody(const Body& body, const glm::mat4& view, const glm::mat4& projection,
                  const glm::vec3& camPos, const glm::vec3& lightPos) {
        glm::vec3 visualPos = getVisualPosition(body.position, body.name);
        drawBodyInternal(body.name, visualPos, body.axialTilt, (float)body.rotationAngle, view, projection, camPos, lightPos);
    }

        /**
     * @brief Internal rendering logic for a single celestial body.
     * 
     * @details
     * Handles shader activation, uniform mapping (MVP matrices, lighting),
     * and texture binding.
     * 
     * **Shader Uniforms**:
     * - `model`, `view`, `projection`: Standard MVP matrices.
     * - `lightPos`: Position of the Sun in visual units.
     * - `viewPos`: Camera position for specular highlights.
     * - `opacity`: Used for ghost/delta visualization.
     * - `isSun`: Boolean toggle in `planet.frag` to disable shadowing on the Sun.
     */
    void drawBodyInternal(const std::string& name, const glm::vec3& visualPos,
                          double axialTilt, float rotationAngle,
                          const glm::mat4& view, const glm::mat4& projection,
                          const glm::vec3& camPos, const glm::vec3& lightPos) {
        
        // Get color
        sf::Color sfColor = bodyColors.count(name) ? bodyColors.at(name) : sf::Color::White;
        glm::vec3 color(sfColor.r / 255.0f, sfColor.g / 255.0f, sfColor.b / 255.0f);
        
        // Use visual radius from lookup
        float visualRadius = getVisualRadius(name);
        
        // Model matrix with visual position scaling
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, visualPos);
        
        // Apply axial tilt
        if (axialTilt != 0) {
            model = glm::rotate(model, glm::radians((float)axialTilt), glm::vec3(0.0f, 0.0f, 1.0f));
        }
        
        // Apply rotation
        model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        
        // Scale
        model = glm::scale(model, glm::vec3(visualRadius));
        
        // Normal matrix for lighting
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
        
        // Choose shader
        bool isSun = (name == "Sun");
        ShaderProgram& shader = isSun ? sunShader : planetShader;
        shader.use();
        
        // Set uniforms
        shader.setMat4("model", model);
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        shader.setVec3("objectColor", color);
        
        // Check for texture
        bool hasTexture = glTextures.count(name) > 0;
        shader.setBool("useTexture", hasTexture);
        if (hasTexture) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, glTextures.at(name));
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
            
            // Skip trails for moons - they're drawn at physics coordinates which don't match
            // the visual satellite positions (moons are scaled relative to their parent planets)
            if (!getParentPlanet(body.name).empty()) continue;

            
            sf::Color sfColor = bodyColors.count(body.name) ? bodyColors.at(body.name) : sf::Color::White;
            
            // Build trail vertices with fading alpha and visual scaling
            std::vector<float> vertices;
            for (size_t i = 0; i < body.trail.size(); ++i) {
                float alpha = 0.4f * (float)i / (float)body.trail.size();
                glm::vec3 visualPt = getVisualPosition(body.trail[i], body.name);
                vertices.push_back(visualPt.x);
                vertices.push_back(visualPt.y);  // Y-up convention handled by getVisualPosition
                vertices.push_back(visualPt.z);
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
            std::vector<float> vertices;
            for (const auto& pt : orbitPoints) {
                glm::vec3 visualPt = getVisualPosition(pt, body.name);
                vertices.push_back(visualPt.x);
                vertices.push_back(visualPt.y);  // Y-up convention handled by getVisualPosition
                vertices.push_back(visualPt.z);
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
public:
    ~GraphicsEngine() {
        if (trailVAO) glDeleteVertexArrays(1, &trailVAO);
        if (trailVBO) glDeleteBuffers(1, &trailVBO);
        if (asteroidInstanceVBO) glDeleteBuffers(1, &asteroidInstanceVBO);
        if (orbitVAO) glDeleteVertexArrays(1, &orbitVAO);
        if (orbitVBO) glDeleteBuffers(1, &orbitVBO);
        for (auto& [name, tex] : glTextures) {
            glDeleteTextures(1, &tex);
        }
    }
};

} // namespace SolarSim

