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

int main() {
    std::cout << "Solar System Simulation: Professional Edition" << std::endl;
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
    // settings.attributeFlags = sf::ContextSettings::Core;
    
    sf::RenderWindow window(sf::VideoMode(1280, 720), "Solar System Simulation 3D", 
                            sf::Style::Default, settings);
    window.setFramerateLimit(60);
    window.setActive(true);
    
    // Initialize GLAD for modern OpenGL functions
    if (!gladLoadGL()) {
        std::cerr << "GLAD initialization failed" << std::endl;
        return 1;
    }
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    
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
    
    std::cout << "Controls: Mouse drag (orbit), Right-drag (pan), Scroll (zoom), or use GUI panels" << std::endl;
    
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
            
            // Handle window resize for fullscreen support
            if (event.type == sf::Event::Resized) {
                glViewport(0, 0, event.size.width, event.size.height);
            }

            // Global Hotkeys (only if not typing in ImGui)
            if (!ImGui::GetIO().WantCaptureKeyboard) {
                if (event.type == sf::Event::KeyPressed) {
                    auto& state = SolarSim::GuiEngine::getState();
                    
                    if (event.key.code == sf::Keyboard::Space) {
                        state.paused = !state.paused;
                        SolarSim::GuiEngine::addToast(state.paused ? "Simulation Paused" : "Simulation Resumed", 
                            state.paused ? SolarSim::GuiEngine::ToastType::Warning : SolarSim::GuiEngine::ToastType::Success);
                    }
                    else if (event.key.code == sf::Keyboard::T) {
                        state.showTrails = !state.showTrails;
                        SolarSim::GuiEngine::addToast(state.showTrails ? "Trails: ON" : "Trails: OFF", SolarSim::GuiEngine::ToastType::Info);
                    }
                    else if (event.key.code == sf::Keyboard::S) {
                        state.requestSave = true;
                        SolarSim::GuiEngine::addToast("Saving state...", SolarSim::GuiEngine::ToastType::Info);
                    }
                    else if (event.key.code == sf::Keyboard::L) {
                        state.requestLoad = true;
                        SolarSim::GuiEngine::addToast("Loading state...", SolarSim::GuiEngine::ToastType::Info);
                    }
                    else if (event.key.code == sf::Keyboard::H) {
                        state.showHelp = !state.showHelp;
                    }
                }
            }

            // Only pass to graphics if ImGui doesn't want the event
            if (!ImGui::GetIO().WantCaptureMouse && !ImGui::GetIO().WantCaptureKeyboard) {
                graphics.handleEvent(event);
            }
        }

        // Update ImGui
        sf::Time deltaTime = deltaClock.restart();
        SolarSim::GuiEngine::update(window, deltaTime);

        // Calculate FPS
        frameCount++;
        if (fpsClock.getElapsedTime().asSeconds() >= 1.0f) {
            SolarSim::GuiEngine::getState().fps = frameCount;
            frameCount = 0;
            fpsClock.restart();
        }

        auto& guiState = SolarSim::GuiEngine::getState();

        // Handle camera focus based on planet selection
        if (guiState.selectedBody != guiState.lastSelectedBody) {
            guiState.lastSelectedBody = guiState.selectedBody;
            
            if (guiState.selectedBody >= 0 && guiState.selectedBody < (int)system.size()) {
                const auto& body = system[guiState.selectedBody];
                
                if (body.name == "Sun") {
                    // Sun selected: camera behaves as free (Orbit mode focused on origin)
                    graphics.getCamera().setMode(SolarSim::CameraMode::Orbit);
                    graphics.getCamera().setFocusPoint(glm::vec3(0.0f, 0.0f, 0.0f));
                    graphics.getCamera().setMinDistance(1.0f);  // Reset to default zoom limit
                    guiState.cameraFocused = false;
                } else {
                    // Planet selected: camera follows the planet
                    glm::vec3 pos((float)body.position.x, (float)body.position.z, (float)body.position.y);
                    graphics.getCamera().setFocusPoint(pos);
                    graphics.getCamera().setMode(SolarSim::CameraMode::Follow);
                    guiState.cameraFocused = true;
                    
                    // Calculate min zoom distance based on body radius (visual radius * safety factor)
                    float visualRadius = (float)body.radius * 50.0f;
                    visualRadius = std::max(0.05f, std::min(visualRadius, 0.5f));
                    float minZoom = visualRadius * 2.5f;  // 2.5x the visual radius as safe distance
                    graphics.getCamera().setMinDistance(minZoom);
                }
            }
        }
        
        // Handle unfocus request
        if (guiState.requestCameraUnfocus) {
            graphics.getCamera().setMode(SolarSim::CameraMode::Orbit);
            guiState.cameraFocused = false;
            guiState.requestCameraUnfocus = false;
            SolarSim::GuiEngine::addToast("Camera unfocused", SolarSim::GuiEngine::ToastType::Info);
        }
        
        // Update camera focus point each frame if focused on a planet (not Sun)
        if (guiState.cameraFocused && guiState.selectedBody >= 0 && guiState.selectedBody < (int)system.size()) {
            const auto& body = system[guiState.selectedBody];
            if (body.name != "Sun") {
                glm::vec3 pos((float)body.position.x, (float)body.position.z, (float)body.position.y);
                graphics.getCamera().setFocusPoint(pos);
            }
        }

        // Handle preset requests (NEW)
        if (guiState.presetRequest >= 0) {
            SolarSim::PresetType preset = static_cast<SolarSim::PresetType>(guiState.presetRequest);
            system = SolarSim::StateManager::loadPreset(preset);
            
            // Add asteroids for Full Solar System preset
            if (guiState.presetRequest == 0) {
                for (int i = 0; i < 100; ++i) {
                    double d = 2.2 + (double)rand()/RAND_MAX * 1.0;
                    double a = (double)rand()/RAND_MAX * 2.0 * M_PI;
                    double v = std::sqrt(39.478 / d);
                    system.emplace_back("Asteroid", 1e-10, 0.0001,
                        SolarSim::Vector3(d*std::cos(a), d*std::sin(a), ((double)rand()/RAND_MAX-0.5)*0.2),
                        SolarSim::Vector3(-v*std::sin(a), v*std::cos(a), 0));
                }
            }
            
            SolarSim::convertToBarycentric(system);
            SolarSim::PhysicsEngine::calculateAccelerations(system);
            guiState.presetRequest = -1;  // Reset request
            guiState.elapsedYears = 0.0f;
            guiState.selectedBody = -1;
            guiState.isLoading = false;   // Reset loading state
            guiState.loadingProgress = 1.0f;
            history.clear();
            SolarSim::GuiEngine::addToast(std::string("Loaded: ") + SolarSim::StateManager::getPresetName(preset), SolarSim::GuiEngine::ToastType::Success);
            std::cout << "Loaded preset: " << SolarSim::StateManager::getPresetName(preset) << std::endl;
        }
        
        // Handle save state request (NEW)
        if (guiState.requestSave) {
            if (SolarSim::StateManager::saveState(system, guiState.saveFilename)) {
                SolarSim::GuiEngine::addToast("State saved successfully", SolarSim::GuiEngine::ToastType::Success);
            } else {
                SolarSim::GuiEngine::addToast("Failed to save state", SolarSim::GuiEngine::ToastType::Error);
            }
            guiState.requestSave = false;
        }
        
        // Handle load state request (NEW)
        if (guiState.requestLoad) {
            auto loaded = SolarSim::StateManager::loadState(guiState.saveFilename);
            if (!loaded.empty()) {
                system = loaded;
                SolarSim::convertToBarycentric(system);
                SolarSim::PhysicsEngine::calculateAccelerations(system);
                guiState.elapsedYears = 0.0f;
                guiState.selectedBody = -1;
                history.clear();
                SolarSim::GuiEngine::addToast("State loaded successfully", SolarSim::GuiEngine::ToastType::Success);
            } else {
                SolarSim::GuiEngine::addToast("Failed to load state", SolarSim::GuiEngine::ToastType::Error);
            }
            guiState.requestLoad = false;
        }

        // Handle Time-Travel Transitions (NEW)
        if (guiState.timeTravelActive && !wasTimeTravelActive) {
            guiState.scrubTime = guiState.elapsedYears;
        }
        wasTimeTravelActive = guiState.timeTravelActive;

        // Handle Epoch Marking (NEW)
        if (guiState.requestMarkEpoch) {
            history.markEpoch(guiState.epochName, guiState.elapsedYears, system);
            guiState.requestMarkEpoch = false;
            std::cout << "Marked Epoch: " << guiState.epochName << " at " << guiState.elapsedYears << " years" << std::endl;
        }

        // Time travel vs Physics
        if (guiState.timeTravelActive) {
            history.getStateAt(guiState.scrubTime, system);
        } else if (!guiState.paused) {
            double frameTime = baseDt * guiState.timeRate;
            double currentT = 0;
            while (currentT < frameTime) {
                double adt = SolarSim::PhysicsEngine::getAdaptiveTimestep(system, baseDt);
                adt = std::min(adt, frameTime - currentT);
                
                // Use selected integrator
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

        // Render
        graphics.render(system, guiState.showTrails, guiState.showOrbits);
        SolarSim::GuiEngine::render(system, history, scalePtr, rotXPtr, rotZPtr);
        SolarSim::GuiEngine::display(window);
        window.display();
    }

    SolarSim::GuiEngine::shutdown();
    return 0;
}
