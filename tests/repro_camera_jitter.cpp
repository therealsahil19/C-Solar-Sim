#include <iostream>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Mock Camera3D minimal implementation or include the actual header
// Since we can't easily link the whole app, I'll copy the relevant logic or include the header if possible.
// Including headers might require dependencies (SFML, GLM).
// The user has GLM in include/glm.
// The user has SFML in external/SFML/include (maybe).
// Let's assume we can compile this with the right include paths.

#define GLM_ENABLE_EXPERIMENTAL
#include "Camera3D.hpp"

// Mock SFML types to allow compilation if headers are missing symbols? 
// Actually we included Camera3D.hpp which includes SFML/Window/Event.hpp.
// We need to link against SFML-window/system or mock them.
// For this test, we just want to test the updateCameraVectors logic.
// We can instantiate Camera3D and manipulate fields directly through getters if public, 
// or subclass it / modify it.
// Camera3D has private fields but public getters for pointers: getYawPtr().

using namespace SolarSim;

void printVec(const std::string& label, const glm::vec3& v) {
    std::cout << label << ": " << v.x << ", " << v.y << ", " << v.z << std::endl;
}

int main() {
    Camera3D camera;
    
    float* yawPtr = camera.getYawPtr();
    float* pitchPtr = camera.getPitchPtr();
    
    std::cout << "Initial Yaw: " << *yawPtr << std::endl;
    
    // Simulate massive rotation
    *yawPtr = 1000000.0f; // 1 million degrees
    camera.update(); // Update vectors
    
    glm::mat4 view1 = camera.getViewMatrix();
    glm::vec3 pos1 = camera.getPosition();
    printVec("Pos at 1M deg", pos1);
    
    // Rotate by 360 degrees (should be same/similar)
    *yawPtr += 360.0f;
    camera.update();
    
    glm::mat4 view2 = camera.getViewMatrix();
    glm::vec3 pos2 = camera.getPosition();
    printVec("Pos at 1M+360 deg", pos2);
    
    // Check difference
    glm::vec3 diff = pos2 - pos1;
    std::cout << "Diff length: " << glm::length(diff) << std::endl;
    
    if (glm::length(diff) > 1.0f) {
        std::cout << "FAIL: Large drift detected!" << std::endl;
    } else {
        std::cout << "Stability check passed (mostly)." << std::endl;
    }
    
    // Check small increment variance (jitter)
    // At high magnitude, small changes might be lost.
    float initialYaw = 1000000.0f;
    *yawPtr = initialYaw;
    camera.update();
    glm::vec3 pA = camera.getPosition();
    
    *yawPtr = initialYaw + 0.1f;
    camera.update();
    glm::vec3 pB = camera.getPosition();
    
    glm::vec3 drift = pB - pA;
    std::cout << "Movement for 0.1 deg at 1M deg: " << glm::length(drift) << std::endl;
    
    // Compare with low magnitude
    *yawPtr = 0.0f;
    camera.update();
    glm::vec3 pC = camera.getPosition();
    
    *yawPtr = 0.1f;
    camera.update();
    glm::vec3 pD = camera.getPosition();
    
    glm::vec3 expectedDrift = pD - pC;
    std::cout << "Movement for 0.1 deg at 0 deg: " << glm::length(expectedDrift) << std::endl;
    
    float error = std::abs(glm::length(drift) - glm::length(expectedDrift));
    std::cout << "Error: " << error << std::endl;
    
    if (error > 0.001f) {
        std::cout << "FAIL: Precision loss detected!" << std::endl;
    } else {
        std::cout << "PASS: Precision OK." << std::endl;
    }
    
    return 0;
}
