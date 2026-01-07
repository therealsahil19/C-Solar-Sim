#include <iostream>
#include <vector>
#include <chrono>
#include "Body.hpp"
#include "PhysicsEngine.hpp"
#include "EphemerisLoader.hpp"
#include "SystemData.hpp"

using namespace SolarSim;

int main() {
    std::cout << "=== Moon Orbital Stability Check ===" << std::endl;
    std::cout << "Loading solar system..." << std::endl;
    
    auto bodies = EphemerisLoader::loadSolarSystemJ2000();
    convertToBarycentric(bodies);
    
    // Find moons of interest
    std::vector<std::string> moonsToCheck = {"Moon", "Io", "Europa", "Ganymede", "Callisto", "Titan", "Triton"};
    
    // Print initial distances
    std::cout << "\n--- INITIAL STATE ---" << std::endl;
    for (const auto& moonName : moonsToCheck) {
        for (const auto& moon : bodies) {
            if (moon.name == moonName && !moon.parentName.empty()) {
                for (const auto& parent : bodies) {
                    if (parent.name == moon.parentName) {
                        double dist = (moon.position - parent.position).length();
                        std::cout << moonName << " -> " << moon.parentName 
                                  << ": " << dist << " AU" << std::endl;
                        break;
                    }
                }
                break;
            }
        }
    }
    
    // Simulate for ~10 seconds of simulation time at 1.0x rate
    // (1 day = 1/365.25 years, 10 sec real time with 60fps = 600 frames)
    // At timeRate 1.0, baseDt = 1/365.25, so 600 frames = ~1.6 days
    // Let's simulate ~30 days to see real orbital behavior
    std::cout << "\nSimulating ~30 days..." << std::endl;
    
    double baseDt = 1.0 / 365.25;  // 1 day
    for (int day = 0; day < 30; ++day) {
        for (int i = 0; i < 10; ++i) {  // Sub-steps per day
            PhysicsEngine::stepBarnesHut(bodies, baseDt / 10.0, 0.5);
        }
    }
    
    // Print final distances
    std::cout << "\n--- AFTER 30 DAYS ---" << std::endl;
    for (const auto& moonName : moonsToCheck) {
        for (const auto& moon : bodies) {
            if (moon.name == moonName && !moon.parentName.empty()) {
                for (const auto& parent : bodies) {
                    if (parent.name == moon.parentName) {
                        double dist = (moon.position - parent.position).length();
                        std::cout << moonName << " -> " << moon.parentName 
                                  << ": " << dist << " AU" << std::endl;
                        break;
                    }
                }
                break;
            }
        }
    }
    
    std::cout << "\n=== Check Complete ===" << std::endl;
    std::cout << "If distances are similar to initial values, the fix is working!" << std::endl;
    
    return 0;
}
