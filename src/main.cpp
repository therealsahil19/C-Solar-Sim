#include <iostream>
#include <vector>

#include "glad.h"

#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>
#include "GraphicsEngine.hpp"
#include "PhysicsEngine.hpp"
#include "EphemerisLoader.hpp"
#include "SystemData.hpp"
#include "GuiEngine.hpp"
#include "StateManager.hpp"
#include "HistoryManager.hpp"

#include <filesystem>

/**
 * @brief Captures the current window content and saves it to a file.
 */
void captureScreen(sf::RenderWindow& window, const std::string& filename) {
    sf::Texture texture;
    texture.create(window.getSize().x, window.getSize().y);
    texture.update(window);
    sf::Image screenshot = texture.copyToImage();
    
    // Ensure directory exists
    std::filesystem::path path(filename);
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }
    
    if (screenshot.saveToFile(filename)) {
        std::cout << "Captured: " << filename << std::endl;
    } else {
        std::cerr << "Failed to capture: " << filename << std::endl;
    }
}

int main(int argc, char* argv[]) {
    // Check for mission mode
    bool isMission = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--mission") {
            isMission = true;
            break;
        }
    }

    std::cout << "Solar System Simulation: Professional Edition" << std::endl;
    if (isMission) std::cout << "--- MISSION MODE ENABLED ---" << std::endl;
    std::cout << "Using J2000 Ephemeris Data for accurate orbital positions" << std::endl;
    
    // Load celestial bodies using real J2000 ephemeris data
    std::vector<SolarSim::Body> system = SolarSim::EphemerisLoader::loadSolarSystemJ2000();
    if (system.empty()) {
        std::cerr << "Error: Failed to load ephemeris data" << std::endl;
        return 1;
    }
    std::cout << "Loaded " << system.size() << " celestial bodies" << std::endl;

    // Add asteroid belt (100 asteroids for performance)
    for (int i = 0; i < 100; ++i) {
        double d = 2.2 + (double)rand()/RAND_MAX * 1.0;  // 2.2-3.2 AU
        double a = (double)rand()/RAND_MAX * 2.0 * M_PI;
        double v = std::sqrt(39.478 / d);  // Circular orbit velocity
        system.emplace_back("Asteroid", 1e-10, 0.0001, 
                           SolarSim::Vector3(d*std::cos(a), d*std::sin(a), ((double)rand()/RAND_MAX-0.5)*0.2), 
                           SolarSim::Vector3(-v*std::sin(a), v*std::cos(a), 0));
    }

    // Convert to barycentric coordinates (zero total momentum)
    SolarSim::convertToBarycentric(system);
    SolarSim::PhysicsEngine::calculateAccelerations(system);

    // Create SFML window with OpenGL 3.3 context
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.majorVersion = 3;
    settings.minorVersion = 3;
    
    sf::RenderWindow window(sf::VideoMode(1280, 720), "Solar System Simulation 3D", 
                            sf::Style::Default, settings);
    window.setFramerateLimit(60);
    window.setActive(true);
    
    // Initialize GLAD for modern OpenGL functions
    if (!gladLoadGL()) {
        std::cerr << "GLAD initialization failed" << std::endl;
        return 1;
    }
    
    // Initialize graphics engine (loads shaders and textures)
    SolarSim::GraphicsEngine graphics(window);
    if (!graphics.init()) {
        std::cerr << "Failed to initialize graphics engine" << std::endl;
        return 1;
    }
    
    // Initialize GUI
    SolarSim::GuiEngine::init(window);

    // Get pointers to camera controls for GUI
    float *scalePtr, *rotXPtr, *rotZPtr;
    graphics.exposeControls(scalePtr, rotXPtr, rotZPtr);

    double baseDt = 1.0/365.25;  // 1 day in years
    sf::Clock deltaClock;
    sf::Clock fpsClock;
    int frameCount = 0;
    
    // Mission Control Variables
    int missionStep = 0;
    float missionTimer = 0.0f;
    int videoFramesCaptured = 0;
    const int MAX_VIDEO_FRAMES = 60;

    // Set default body selection to Sun
    for (int i = 0; i < (int)system.size(); ++i) {
        if (system[i].name == "Sun") {
            SolarSim::GuiEngine::getState().selectedBody = i;
            break;
        }
    }
    
    SolarSim::HistoryManager history;
    bool wasTimeTravelActive = false;

    while (window.isOpen()) {
        // Event handling
        sf::Event event;
        while (window.pollEvent(event)) {
            SolarSim::GuiEngine::processEvent(event);
            
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            
            if (event.type == sf::Event::Resized) {
                glViewport(0, 0, event.size.width, event.size.height);
            }

            if (!ImGui::GetIO().WantCaptureKeyboard) {
                if (event.type == sf::Event::KeyPressed) {
                    auto& state = SolarSim::GuiEngine::getState();
                    if (event.key.code == sf::Keyboard::Space) state.paused = !state.paused;
                    else if (event.key.code == sf::Keyboard::T) state.showTrails = !state.showTrails;
                    else if (event.key.code == sf::Keyboard::H) state.showHelp = !state.showHelp;
                }
            }

            if (!ImGui::GetIO().WantCaptureMouse && !ImGui::GetIO().WantCaptureKeyboard) {
                graphics.handleEvent(event);
            }
        }

        sf::Time deltaTime = deltaClock.restart();
        float dtSec = deltaTime.asSeconds();
        SolarSim::GuiEngine::update(window, deltaTime);

        auto& guiState = SolarSim::GuiEngine::getState();


        // FPS Calculation
        frameCount++;
        if (fpsClock.getElapsedTime().asSeconds() >= 1.0f) {
            guiState.fps = frameCount;
            frameCount = 0;
            fpsClock.restart();
        }

        // Helper to get visual position matching graphics rendering
        auto getVisualPos = [&system](const SolarSim::Body& body) -> glm::vec3 {
            if (body.name == "Moon") {
                for (const auto& b : system) {
                    if (b.name == "Earth") {
                        return SolarSim::GraphicsEngine::calculateMoonVisualPosition(body, b);
                    }
                }
            }
            return SolarSim::GraphicsEngine::getVisualPosition(body.position, body.name);
        };

        auto getVisualRad = [](const std::string& name) -> float {
            return SolarSim::GraphicsEngine::getVisualRadius(name);
        };

        // MISSION LOGIC
        if (isMission) {
            missionTimer += dtSec;
            guiState.paused = false;
            guiState.timeRate = 1.0f;

            if (missionStep == 0) { // Initial wait for assets
                if (missionTimer > 2.0f) {
                    // Navigate to Jupiter
                    for (int i = 0; i < (int)system.size(); ++i) {
                        if (system[i].name == "Jupiter") {
                            guiState.selectedBody = i;
                            break;
                        }
                    }
                    missionStep = 1;
                    missionTimer = 0.0f;
                }
            } 
            else if (missionStep == 1) { // Setup Close-up
                if (missionTimer > 1.0f) {
                    float jupiterRad = getVisualRad("Jupiter");
                    graphics.getCamera().setMinDistance(jupiterRad * 1.2f);
                    *graphics.getCamera().getDistancePtr() = jupiterRad * 2.0f; // Start a bit back
                    *graphics.getCamera().getYawPtr() = -90.0f;
                    *graphics.getCamera().getPitchPtr() = 10.0f;
                    graphics.getCamera().update();
                    missionStep = 2;
                    missionTimer = 0.0f;
                }
            }
            else if (missionStep >= 2 && missionStep <= 5) { // Capture 4 Angles
                if (missionTimer > 0.5f) {
                    int angleIndex = missionStep - 2;
                    float yaw = -90.0f + angleIndex * 90.0f;
                    *graphics.getCamera().getYawPtr() = yaw;
                    *graphics.getCamera().getDistancePtr() = getVisualRad("Jupiter") * 1.5f; // Very close
                    graphics.getCamera().update();
                    
                    char filename[64];
                    sprintf(filename, "jupiter_%d.png", angleIndex + 1);
                    captureScreen(window, filename);
                    
                    missionStep++;
                    missionTimer = 0.0f;
                }
            }
            else if (missionStep == 6) { // Prepare 360 Recording
                if (missionTimer > 0.5f) {
                    missionStep = 7;
                    missionTimer = 0.0f;
                    videoFramesCaptured = 0;
                }
            }
            else if (missionStep == 7) { // Recording Jupiter 360 (Simulated User Flow)
                float startYaw = -90.0f;
                float currentYaw = startYaw + ((float)videoFramesCaptured / MAX_VIDEO_FRAMES) * 360.0f;
                
                *graphics.getCamera().getYawPtr() = currentYaw;
                *graphics.getCamera().getPitchPtr() = 5.0f; // Slightly lower angle for surface feel
                *graphics.getCamera().getDistancePtr() = getVisualRad("Jupiter") * 1.8f;
                graphics.getCamera().update();
                
                char filename[64];
                sprintf(filename, "frames/jupiter_%03d.png", videoFramesCaptured);
                captureScreen(window, filename);
                
                videoFramesCaptured++;
                if (videoFramesCaptured >= MAX_VIDEO_FRAMES) {
                    missionStep = 8;
                    std::cout << "Successfully completed Jupiter mission." << std::endl;
                    window.close();
                }
            }
        }

        // Camera Logic
        if (guiState.selectedBody != guiState.lastSelectedBody) {
            guiState.lastSelectedBody = guiState.selectedBody;
            if (guiState.selectedBody >= 0 && guiState.selectedBody < (int)system.size()) {
                const auto& body = system[guiState.selectedBody];
                if (body.name == "Sun") {
                    graphics.getCamera().setMode(SolarSim::CameraMode::Orbit);
                    graphics.getCamera().setFocusPoint(glm::vec3(0.0f, 0.0f, 0.0f));
                    graphics.getCamera().setMinDistance(getVisualRad("Sun") * 1.5f);
                    guiState.cameraFocused = false;
                } else {
                    glm::vec3 pos = getVisualPos(body);
                    graphics.getCamera().setFocusPoint(pos);
                    graphics.getCamera().setMode(SolarSim::CameraMode::Follow);
                    guiState.cameraFocused = true;
                    graphics.getCamera().setMinDistance(getVisualRad(body.name) * 2.0f);
                }
            }
        }
        
        if (guiState.requestCameraUnfocus) {
            graphics.getCamera().setMode(SolarSim::CameraMode::Orbit);
            graphics.getCamera().setFocusPoint(glm::vec3(0.0f, 0.0f, 0.0f));
            guiState.cameraFocused = false;
            guiState.requestCameraUnfocus = false;
            for (int i = 0; i < (int)system.size(); ++i) {
                if (system[i].name == "Sun") { guiState.selectedBody = i; guiState.lastSelectedBody = i; break; }
            }
        }
        
        if (guiState.cameraFocused && guiState.selectedBody >= 0 && guiState.selectedBody < (int)system.size()) {
            const auto& body = system[guiState.selectedBody];
            if (body.name != "Sun") {
                graphics.getCamera().setFocusPoint(getVisualPos(body));
            }
        }

        // Time-Travel Logic
        if (guiState.timeTravelActive) {
            // Scrubbing through history - restore state
            history.getStateAt(guiState.scrubTime, system);
            
            // Handle requests to resume from current position
            if (!guiState.timeTravelActive) {
                // User clicked "Resume" - truncate future history
                history.truncateAfter(guiState.scrubTime);
            }
        }
        
        // Handle epoch marking
        if (guiState.requestMarkEpoch) {
            history.markEpoch(guiState.epochName, guiState.elapsedYears, system);
            guiState.requestMarkEpoch = false;
        }
        
        // Handle epoch jumping
        if (!guiState.requestEpochJump.empty()) {
            const auto& epochs = history.getEpochs();
            auto it = epochs.find(guiState.requestEpochJump);
            if (it != epochs.end()) {
                history.getStateAt(it->second.time, system);
                guiState.scrubTime = (float)it->second.time;
            }
            guiState.requestEpochJump = "";
        }

        // Physics (skip if time-traveling)
        if (!guiState.paused && !guiState.timeTravelActive) {
            double frameTime = baseDt * guiState.timeRate;
            double currentT = 0;
            while (currentT < frameTime) {
                double adt = SolarSim::PhysicsEngine::getAdaptiveTimestep(system, baseDt);
                adt = std::min(adt, frameTime - currentT);
                switch (guiState.integrator) {
                    case 0: SolarSim::PhysicsEngine::stepVerlet(system, adt); break;
                    case 1: SolarSim::PhysicsEngine::stepRK4(system, adt); break;
                    case 2: SolarSim::PhysicsEngine::stepBarnesHut(system, adt, 0.5); break;
                }
                currentT += adt;
            }
            guiState.elapsedYears += (float)frameTime;
            history.record(guiState.elapsedYears, system);
        }

        // Rendering
        const SolarSim::Snapshot* ghostSnapshot = nullptr;
        static SolarSim::Snapshot scrubGhost;
        static bool hasScrubGhost = false;

        if (guiState.showGhost) {
            if (guiState.selectedGhostEpoch.empty()) {
                // Ghost follows the current scrubbed state if active
                if (guiState.timeTravelActive) {
                    scrubGhost.time = (double)guiState.scrubTime;
                    scrubGhost.bodyStates.clear();
                    for (const auto& b : system) {
                        scrubGhost.bodyStates.push_back({b.position, b.velocity, b.rotationAngle});
                    }
                    hasScrubGhost = true;
                }
                if (hasScrubGhost) ghostSnapshot = &scrubGhost;
            } else {
                const auto& epochs = history.getEpochs();
                auto it = epochs.find(guiState.selectedGhostEpoch);
                if (it != epochs.end()) ghostSnapshot = &it->second;
            }
        }

        graphics.render(system, guiState.showTrails, guiState.showPlanetOrbits, guiState.showOtherOrbits, ghostSnapshot, guiState.ghostOpacity);
        SolarSim::GuiEngine::renderLabels(system, graphics.getViewProjectionMatrix(), window.getSize());
        SolarSim::GuiEngine::render(system, history, scalePtr, rotXPtr, rotZPtr);
        SolarSim::GuiEngine::display(window);
        window.display();
    }

    SolarSim::GuiEngine::shutdown();
    return 0;
}
