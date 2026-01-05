#include <iostream>
#include <vector>

#include "glad.h"

#include <SFML/Graphics.hpp>
#include "GraphicsEngine.hpp"
#include "PhysicsEngine.hpp"
#include "EphemerisLoader.hpp"
#include "SystemData.hpp"
#include "GuiEngine.hpp"
#include "StateManager.hpp"

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

    // Add asteroid belt (200 asteroids)
    for (int i = 0; i < 200; ++i) {
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
    settings.attributeFlags = sf::ContextSettings::Core;
    
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
    
    std::cout << "Controls: WASD (orbit camera), Scroll (zoom), or use GUI panels" << std::endl;
    
    while (window.isOpen()) {
        // Event handling
        sf::Event event;
        while (window.pollEvent(event)) {
            SolarSim::GuiEngine::processEvent(event);
            
            if (event.type == sf::Event::Closed) {
                window.close();
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

        // Handle preset requests (NEW)
        if (guiState.presetRequest >= 0) {
            SolarSim::PresetType preset = static_cast<SolarSim::PresetType>(guiState.presetRequest);
            system = SolarSim::StateManager::loadPreset(preset);
            
            // Add asteroids for Full Solar System preset
            if (guiState.presetRequest == 0) {
                for (int i = 0; i < 200; ++i) {
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
            std::cout << "Loaded preset: " << SolarSim::StateManager::getPresetName(preset) << std::endl;
        }
        
        // Handle save state request (NEW)
        if (guiState.requestSave) {
            SolarSim::StateManager::saveState(system, guiState.saveFilename);
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
            }
            guiState.requestLoad = false;
        }

        // Physics simulation (unless paused)
        if (!guiState.paused) {
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
        }

        // Render
        graphics.render(system, guiState.showTrails, guiState.showAxes);
        SolarSim::GuiEngine::render(system, scalePtr, rotXPtr, rotZPtr);
        SolarSim::GuiEngine::display(window);
        window.display();
    }

    SolarSim::GuiEngine::shutdown();
    return 0;
}
