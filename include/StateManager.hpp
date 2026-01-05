#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include "Body.hpp"
#include "EphemerisLoader.hpp"

namespace SolarSim {

/**
 * @brief Preset simulation scenarios.
 */
enum class PresetType {
    FullSolarSystem,
    InnerPlanets,
    OuterGiants,
    EarthMoonSystem,
    BinaryStarTest
};

/**
 * @brief Manages simulation state: save, load, and preset scenarios.
 */
class StateManager {
public:
    /**
     * @brief Saves current simulation state to a CSV file.
     * @param bodies Current body vector
     * @param filename Output filename
     * @return True if save succeeded
     */
    static bool saveState(const std::vector<Body>& bodies, const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for saving: " << filename << std::endl;
            return false;
        }

        // Header
        file << "name,mass,radius,px,py,pz,vx,vy,vz,rotAngle,rotSpeed,axialTilt\n";

        // Write each body
        for (const auto& body : bodies) {
            file << body.name << ","
                 << body.mass << ","
                 << body.radius << ","
                 << body.position.x << "," << body.position.y << "," << body.position.z << ","
                 << body.velocity.x << "," << body.velocity.y << "," << body.velocity.z << ","
                 << body.rotationAngle << "," << body.rotationSpeed << "," << body.axialTilt << "\n";
        }

        file.close();
        std::cout << "Saved simulation state to: " << filename << std::endl;
        return true;
    }

    /**
     * @brief Loads simulation state from a CSV file.
     * @param filename Input filename
     * @return Vector of bodies (empty if load failed)
     */
    static std::vector<Body> loadState(const std::string& filename) {
        std::vector<Body> bodies;
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for loading: " << filename << std::endl;
            return bodies;
        }

        std::string line;
        std::getline(file, line); // Skip header

        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string token;
            std::vector<std::string> tokens;
            
            while (std::getline(ss, token, ',')) {
                tokens.push_back(token);
            }

            if (tokens.size() >= 12) {
                Body body(
                    tokens[0],                           // name
                    std::stod(tokens[1]),                // mass
                    std::stod(tokens[2]),                // radius
                    Vector3(std::stod(tokens[3]), std::stod(tokens[4]), std::stod(tokens[5])),  // position
                    Vector3(std::stod(tokens[6]), std::stod(tokens[7]), std::stod(tokens[8]))   // velocity
                );
                body.rotationAngle = std::stod(tokens[9]);
                body.rotationSpeed = std::stod(tokens[10]);
                body.axialTilt = std::stod(tokens[11]);
                bodies.push_back(body);
            }
        }

        file.close();
        std::cout << "Loaded " << bodies.size() << " bodies from: " << filename << std::endl;
        return bodies;
    }

    /**
     * @brief Loads a preset simulation scenario.
     * @param preset The scenario to load
     * @return Vector of bodies configured for the preset
     */
    static std::vector<Body> loadPreset(PresetType preset) {
        std::vector<Body> system;

        switch (preset) {
            case PresetType::FullSolarSystem:
                return EphemerisLoader::loadSolarSystemJ2000();

            case PresetType::InnerPlanets: {
                // Sun + Mercury, Venus, Earth, Mars
                Body sun("Sun", 1.0, 0.00465, Vector3(0, 0, 0), Vector3(0, 0, 0));
                sun.rotationSpeed = 13.0;
                system.push_back(sun);

                auto full = EphemerisLoader::loadSolarSystemJ2000();
                for (const auto& b : full) {
                    if (b.name == "Mercury" || b.name == "Venus" || 
                        b.name == "Earth" || b.name == "Mars" || b.name == "Moon") {
                        system.push_back(b);
                    }
                }
                break;
            }

            case PresetType::OuterGiants: {
                // Sun + Jupiter, Saturn, Uranus, Neptune
                Body sun("Sun", 1.0, 0.00465, Vector3(0, 0, 0), Vector3(0, 0, 0));
                system.push_back(sun);

                auto full = EphemerisLoader::loadSolarSystemJ2000();
                for (const auto& b : full) {
                    if (b.name == "Jupiter" || b.name == "Saturn" || 
                        b.name == "Uranus" || b.name == "Neptune") {
                        system.push_back(b);
                    }
                }
                break;
            }

            case PresetType::EarthMoonSystem: {
                // Sun + Earth + Moon only
                Body sun("Sun", 1.0, 0.00465, Vector3(0, 0, 0), Vector3(0, 0, 0));
                system.push_back(sun);

                auto full = EphemerisLoader::loadSolarSystemJ2000();
                for (const auto& b : full) {
                    if (b.name == "Earth" || b.name == "Moon") {
                        system.push_back(b);
                    }
                }
                break;
            }

            case PresetType::BinaryStarTest: {
                // Two equal-mass stars orbiting each other
                double starMass = 0.5;  // Each half solar mass
                double separation = 1.0;  // 1 AU apart
                double orbitalVelocity = std::sqrt(39.478 * starMass / (2.0 * separation));

                Body star1("Star A", starMass, 0.004, 
                          Vector3(-separation/2, 0, 0), Vector3(0, -orbitalVelocity/2, 0));
                star1.rotationSpeed = 15.0;
                
                Body star2("Star B", starMass, 0.004, 
                          Vector3(separation/2, 0, 0), Vector3(0, orbitalVelocity/2, 0));
                star2.rotationSpeed = 15.0;

                system.push_back(star1);
                system.push_back(star2);
                break;
            }
        }

        return system;
    }

    /**
     * @brief Returns human-readable name for a preset.
     */
    static const char* getPresetName(PresetType preset) {
        switch (preset) {
            case PresetType::FullSolarSystem: return "Full Solar System";
            case PresetType::InnerPlanets: return "Inner Planets";
            case PresetType::OuterGiants: return "Outer Giants";
            case PresetType::EarthMoonSystem: return "Earth-Moon System";
            case PresetType::BinaryStarTest: return "Binary Star Test";
            default: return "Unknown";
        }
    }
};

} // namespace SolarSim
