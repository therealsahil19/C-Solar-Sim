#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include "Body.hpp"

namespace SolarSim {

/**
 * @brief Simple CSV-based config loader.
 * While the plan mentioned JSON, CSV is more robust without external libraries.
 * format: name,mass,radius,posX,posY,posZ,velX,velY,velZ
 */
class ConfigLoader {
public:
    static std::vector<Body> loadFromCSV(const std::string& filename) {
        std::vector<Body> bodies;
        std::ifstream file(filename);
        
        if (!file.is_open()) {
            std::cerr << "Error: Could not open config file " << filename << std::endl;
            return bodies;
        }

        std::string line;
        // Skip header
        std::getline(file, line);

        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;

            std::stringstream ss(line);
            std::string name, val;
            double mass, radius, px, py, pz, vx, vy, vz;

            std::getline(ss, name, ',');
            
            auto getDbl = [&](double& out) {
                std::getline(ss, val, ',');
                out = std::stod(val);
            };

            try {
                getDbl(mass);
                getDbl(radius);
                getDbl(px); getDbl(py); getDbl(pz);
                getDbl(vx); getDbl(vy); getDbl(vz);
                
                Body body(name, mass, radius, Vector3(px, py, pz), Vector3(vx, vy, vz));
                
                if (std::getline(ss, val, ',')) body.rotationSpeed = std::stod(val);
                if (std::getline(ss, val, ',')) body.axialTilt = std::stod(val);

                bodies.push_back(body);
            } catch (const std::exception& e) {
                std::cerr << "Error parsing line: " << line << " (" << e.what() << ")" << std::endl;
            }
        }

        return bodies;
    }
};

} // namespace SolarSim
