#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>
#include <cmath>

namespace SolarSim {

enum class CameraMode {
    Orbit,      // Rotate around a focus point (mouse drag)
    FreeFly,    // WASD + mouse look
    Follow      // Track a selected body (mouse drag to adjust view)
};

/**
 * @brief 3D camera with mouse-based controls.
 * 
 * Controls:
 * - Left mouse drag: Orbit rotation (yaw/pitch)
 * - Right mouse drag: Pan the focus point
 * - Scroll wheel: Zoom in/out
 */
class Camera3D {
private:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    // Orbit mode parameters
    glm::vec3 focusPoint;
    float orbitDistance;
    float minOrbitDistance;  // Minimum zoom distance (prevents clipping into planets)
    float yaw;
    float pitch;
    float roll;              // Camera roll (tilt) angle in degrees

    // Camera settings
    float fov;
    float nearPlane;
    float farPlane;
    float moveSpeed;
    float zoomSpeed;
    float rotateSpeed;
    float panSpeed;

    CameraMode mode;

    // Mouse state tracking
    bool leftMouseDown = false;
    bool rightMouseDown = false;
    bool middleMouseDown = false;  // For roll control
    int lastMouseX = 0;
    int lastMouseY = 0;

    void updateCameraVectors() {
        // Normalize yaw to prevent floating point precision loss at large values
        yaw = std::fmod(yaw, 360.0f);

        glm::vec3 newFront;
        newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        newFront.y = sin(glm::radians(pitch));
        newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(newFront);
        
        // Calculate camera position first
        if (mode == CameraMode::Orbit || mode == CameraMode::Follow) {
            position = focusPoint - front * orbitDistance;
        }
        
        // ARCBALL CAMERA: Use fixed world-up for stable planet orientation
        // This prevents the viewport from rotating as the user orbits, making
        // planet textures appear stable from the user's perspective.
        
        // Since pitch is constrained to +/- 89 degrees, front is never parallel to worldUp.
        // We can safely use worldUp without fallback logic that causes discontinuities.
        glm::vec3 baseUp = worldUp;
        
        // Compute right and up from baseUp (Gram-Schmidt orthogonalization)
        glm::vec3 initialRight = glm::normalize(glm::cross(front, baseUp));
        glm::vec3 initialUp = glm::normalize(glm::cross(initialRight, front));
        
        // Apply manual roll rotation around the front axis
        if (roll != 0.0f) {
            float rollRad = glm::radians(roll);
            float cosRoll = cos(rollRad);
            float sinRoll = sin(rollRad);
            right = initialRight * cosRoll + initialUp * sinRoll;
            up = -initialRight * sinRoll + initialUp * cosRoll;
        } else {
            right = initialRight;
            up = initialUp;
        }
    }

public:
    Camera3D()
        : position(0.0f, 50.0f, 50.0f)
        , front(0.0f, 0.0f, -1.0f)
        , up(0.0f, 1.0f, 0.0f)
        , worldUp(0.0f, 1.0f, 0.0f)
        , focusPoint(0.0f, 0.0f, 0.0f)
        , orbitDistance(80.0f)
        , minOrbitDistance(1.0f)  // Default minimum
        , yaw(-90.0f)
        , pitch(30.0f)
        , roll(0.0f)  // Start with no roll
        , fov(45.0f)
        , nearPlane(0.5f)
        ,farPlane(25000.0f)
        , moveSpeed(5.0f)
        , zoomSpeed(5.0f)
        , rotateSpeed(0.2f)
        , panSpeed(0.05f)
        , mode(CameraMode::Orbit)
    {
        updateCameraVectors();
    }

    glm::mat4 getViewMatrix() const {
        // Pole-safe view matrix: avoid gimbal lock when looking straight up/down
        //
        // At high pitch (|pitch| > ~85°), 'front' becomes nearly parallel to worldUp
        // causing glm::lookAt to produce unstable or degenerate view matrices.
        // Solution: use a forward-facing vector as the up hint near the poles.
        
        glm::vec3 viewUp;
        const float polePitchThreshold = 85.0f;
        
        if (pitch > polePitchThreshold) {
            // Looking up (camera below, looking at sky) - use -Z as up
            viewUp = glm::vec3(0.0f, 0.0f, -1.0f);
        } else if (pitch < -polePitchThreshold) {
            // Looking down (camera above, looking at ground) - use +Z as up
            viewUp = glm::vec3(0.0f, 0.0f, 1.0f);
        } else {
            // Normal case: use world up
            viewUp = worldUp;
        }
        
        // Apply explicit roll if requested (middle mouse drag)
        if (roll != 0.0f) {
            glm::vec3 tempRight = glm::normalize(glm::cross(front, viewUp));
            float cosRoll = cos(glm::radians(roll));
            float sinRoll = sin(glm::radians(roll));
            viewUp = viewUp * cosRoll + tempRight * sinRoll;
        }
        
        return glm::lookAt(position, position + front, viewUp);
    }

    glm::mat4 getProjectionMatrix(float aspectRatio) const {
        // Dynamic near plane: scales with distance to improve depth precision at close range
        // Clamp to minimum of 0.1f to avoid extreme precision loss
        float dynamicNear = std::max(0.1f, orbitDistance * 0.005f);
        return glm::perspective(glm::radians(fov), aspectRatio, dynamicNear, farPlane);
    }

    glm::vec3 getPosition() const { return position; }
    CameraMode getMode() const { return mode; }
    void setMode(CameraMode m) { mode = m; }

    void setFocusPoint(const glm::vec3& focus) {
        focusPoint = focus;
        if (mode == CameraMode::Orbit || mode == CameraMode::Follow) {
            updateCameraVectors();
        }
    }

    void setMinDistance(float minDist) {
        minOrbitDistance = minDist;
        // Clamp current distance if needed
        if (orbitDistance < minOrbitDistance) {
            orbitDistance = minOrbitDistance;
            updateCameraVectors();
        }
    }

    float getMinDistance() const { return minOrbitDistance; }

    bool isFocused() const { return mode == CameraMode::Follow; }

    // Reset camera to default position/orientation
    void resetToDefault() {
        focusPoint = glm::vec3(0.0f, 0.0f, 0.0f);
        orbitDistance = 80.0f;
        minOrbitDistance = 1.0f;
        yaw = -90.0f;
        pitch = 30.0f;
        roll = 0.0f;  // Reset roll
        mode = CameraMode::Orbit;
        updateCameraVectors();
    }

    // Expose controls for GUI
    float* getDistancePtr() { return &orbitDistance; }
    float* getYawPtr() { return &yaw; }
    float* getPitchPtr() { return &pitch; }
    float* getRollPtr() { return &roll; }
    float* getFovPtr() { return &fov; }

    void handleEvent(const sf::Event& event) {
        // Mouse wheel zoom (all modes) - distance-proportional for smooth control
        if (event.type == sf::Event::MouseWheelScrolled) {
            // Zoom speed scales with distance: slow when close, faster when far
            float adaptiveZoom = orbitDistance * 0.1f;  // 10% of current distance per scroll
            adaptiveZoom = std::max(0.5f, std::min(adaptiveZoom, 1000.0f));  // Clamp to reasonable range
            orbitDistance -= event.mouseWheelScroll.delta * adaptiveZoom;
            orbitDistance = std::max(minOrbitDistance, std::min(orbitDistance, 20000.0f));
            updateCameraVectors();
        }

        // Mouse button press
        if (event.type == sf::Event::MouseButtonPressed) {
            if (event.mouseButton.button == sf::Mouse::Left) {
                leftMouseDown = true;
                lastMouseX = event.mouseButton.x;
                lastMouseY = event.mouseButton.y;
            }
            if (event.mouseButton.button == sf::Mouse::Right) {
                rightMouseDown = true;
                lastMouseX = event.mouseButton.x;
                lastMouseY = event.mouseButton.y;
            }
            if (event.mouseButton.button == sf::Mouse::Middle) {
                middleMouseDown = true;
                lastMouseX = event.mouseButton.x;
                lastMouseY = event.mouseButton.y;
            }
        }

        // Mouse button release
        if (event.type == sf::Event::MouseButtonReleased) {
            if (event.mouseButton.button == sf::Mouse::Left) {
                leftMouseDown = false;
            }
            if (event.mouseButton.button == sf::Mouse::Right) {
                rightMouseDown = false;
            }
            if (event.mouseButton.button == sf::Mouse::Middle) {
                middleMouseDown = false;
            }
        }

        // Mouse movement
        if (event.type == sf::Event::MouseMoved) {
            int deltaX = event.mouseMove.x - lastMouseX;
            int deltaY = event.mouseMove.y - lastMouseY;
            lastMouseX = event.mouseMove.x;
            lastMouseY = event.mouseMove.y;

            if (leftMouseDown) {
                // Left drag: Orbit rotation (yaw/pitch)
                if (mode == CameraMode::Orbit || mode == CameraMode::Follow) {
                    yaw += static_cast<float>(deltaX) * rotateSpeed;
                    pitch -= static_cast<float>(deltaY) * rotateSpeed;
                    pitch = std::max(-89.0f, std::min(pitch, 89.0f));
                    updateCameraVectors();
                }
            }

            if (rightMouseDown) {
                // Right drag: Pan the focus point
                if (mode == CameraMode::Orbit || mode == CameraMode::Follow) {
                    float panScale = orbitDistance * panSpeed * 0.01f;
                    focusPoint -= right * static_cast<float>(deltaX) * panScale;
                    focusPoint += up * static_cast<float>(deltaY) * panScale;
                    updateCameraVectors();
                }
            }
            
            if (middleMouseDown) {
                // Middle drag: Roll the camera
                if (mode == CameraMode::Orbit || mode == CameraMode::Follow) {
                    roll += static_cast<float>(deltaX) * rotateSpeed;
                    // Clamp roll to reasonable range
                    roll = std::fmod(roll + 180.0f, 360.0f) - 180.0f;
                    updateCameraVectors();
                }
            }
        }

        // Keep WASD for FreeFly mode only
        if (event.type == sf::Event::KeyPressed && mode == CameraMode::FreeFly) {
            if (event.key.code == sf::Keyboard::W) position += front * moveSpeed;
            if (event.key.code == sf::Keyboard::S) position -= front * moveSpeed;
            if (event.key.code == sf::Keyboard::A) position -= right * moveSpeed;
            if (event.key.code == sf::Keyboard::D) position += right * moveSpeed;
            if (event.key.code == sf::Keyboard::Q) position -= up * moveSpeed;
            if (event.key.code == sf::Keyboard::E) position += up * moveSpeed;
        }
    }

    void update() {
        updateCameraVectors();
    }
};

} // namespace SolarSim

