#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SFML/Window/Event.hpp>
#include <cmath>

namespace SolarSim {

enum class CameraMode {
    Orbit,      // Rotate around a focus point
    FreeFly,    // WASD + mouse look
    Follow      // Track a selected body
};

/**
 * @brief 3D camera with multiple viewing modes.
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
    float yaw;
    float pitch;

    // Camera settings
    float fov;
    float nearPlane;
    float farPlane;
    float moveSpeed;
    float zoomSpeed;
    float rotateSpeed;

    CameraMode mode;

    void updateCameraVectors() {
        glm::vec3 newFront;
        newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        newFront.y = sin(glm::radians(pitch));
        newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(newFront);
        right = glm::normalize(glm::cross(front, worldUp));
        up = glm::normalize(glm::cross(right, front));

        if (mode == CameraMode::Orbit) {
            position = focusPoint - front * orbitDistance;
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
        , yaw(-90.0f)
        , pitch(30.0f)
        , fov(45.0f)
        , nearPlane(0.01f)
        , farPlane(1000.0f)
        , moveSpeed(5.0f)
        , zoomSpeed(5.0f)
        , rotateSpeed(0.3f)
        , mode(CameraMode::Orbit)
    {
        updateCameraVectors();
    }

    glm::mat4 getViewMatrix() const {
        return glm::lookAt(position, position + front, up);
    }

    glm::mat4 getProjectionMatrix(float aspectRatio) const {
        return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
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

    // Expose controls for GUI
    float* getDistancePtr() { return &orbitDistance; }
    float* getYawPtr() { return &yaw; }
    float* getPitchPtr() { return &pitch; }
    float* getFovPtr() { return &fov; }

    void handleEvent(const sf::Event& event) {
        if (event.type == sf::Event::MouseWheelScrolled) {
            orbitDistance -= event.mouseWheelScroll.delta * zoomSpeed;
            orbitDistance = std::max(1.0f, std::min(orbitDistance, 500.0f));
            updateCameraVectors();
        }

        if (event.type == sf::Event::KeyPressed) {
            switch (mode) {
                case CameraMode::Orbit:
                    if (event.key.code == sf::Keyboard::W) pitch += 5.0f;
                    if (event.key.code == sf::Keyboard::S) pitch -= 5.0f;
                    if (event.key.code == sf::Keyboard::A) yaw -= 5.0f;
                    if (event.key.code == sf::Keyboard::D) yaw += 5.0f;
                    pitch = std::max(-89.0f, std::min(pitch, 89.0f));
                    break;

                case CameraMode::FreeFly:
                    if (event.key.code == sf::Keyboard::W) position += front * moveSpeed;
                    if (event.key.code == sf::Keyboard::S) position -= front * moveSpeed;
                    if (event.key.code == sf::Keyboard::A) position -= right * moveSpeed;
                    if (event.key.code == sf::Keyboard::D) position += right * moveSpeed;
                    if (event.key.code == sf::Keyboard::Q) position -= up * moveSpeed;
                    if (event.key.code == sf::Keyboard::E) position += up * moveSpeed;
                    break;

                case CameraMode::Follow:
                    // In follow mode, WASD adjusts orbit around followed body
                    if (event.key.code == sf::Keyboard::W) pitch += 5.0f;
                    if (event.key.code == sf::Keyboard::S) pitch -= 5.0f;
                    if (event.key.code == sf::Keyboard::A) yaw -= 5.0f;
                    if (event.key.code == sf::Keyboard::D) yaw += 5.0f;
                    pitch = std::max(-89.0f, std::min(pitch, 89.0f));
                    break;
            }
            updateCameraVectors();
        }
    }

    void update() {
        updateCameraVectors();
    }
};

} // namespace SolarSim
