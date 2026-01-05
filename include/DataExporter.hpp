#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include "Body.hpp"

namespace SolarSim {

/**
 * @brief Data export utilities for simulation analysis.
 */
class DataExporter {
public:
    /**
     * @brief Exports full trajectory data to CSV file.
     * @param bodies Current bodies
     * @param filename Output filename
     * @param append Append to existing file or overwrite
     * @param timestamp Simulation time to record
     * @return True if export succeeded
     */
    static bool exportToCSV(const std::vector<Body>& bodies, 
                           const std::string& filename,
                           bool append = false,
                           double timestamp = 0.0) {
        std::ios_base::openmode mode = append ? std::ios::app : std::ios::out;
        std::ofstream file(filename, mode);
        
        if (!file.is_open()) {
            std::cerr << "Failed to open file for export: " << filename << std::endl;
            return false;
        }
        
        // Write header only if not appending
        if (!append) {
            file << "timestamp,name,mass,radius,"
                 << "pos_x,pos_y,pos_z,"
                 << "vel_x,vel_y,vel_z,"
                 << "speed,distance_from_origin\n";
        }
        
        // Write each body's data
        for (const auto& body : bodies) {
            double speed = body.velocity.length();
            double distance = body.position.length();
            
            file << std::fixed << std::setprecision(8)
                 << timestamp << ","
                 << body.name << ","
                 << body.mass << ","
                 << body.radius << ","
                 << body.position.x << "," << body.position.y << "," << body.position.z << ","
                 << body.velocity.x << "," << body.velocity.y << "," << body.velocity.z << ","
                 << speed << "," << distance << "\n";
        }
        
        file.close();
        return true;
    }

    /**
     * @brief Exports summary statistics to file.
     * @param bodies Current bodies
     * @param filename Output filename
     * @param elapsedYears Simulation elapsed time
     * @return True if export succeeded
     */
    static bool exportSummary(const std::vector<Body>& bodies,
                             const std::string& filename,
                             double elapsedYears) {
        std::ofstream file(filename);
        if (!file.is_open()) return false;
        
        file << "=== Solar System Simulation Summary ===" << std::endl;
        file << "Elapsed Time: " << elapsedYears << " years" << std::endl;
        file << "Number of Bodies: " << bodies.size() << std::endl;
        file << std::endl;
        
        file << "Body Details:" << std::endl;
        file << std::string(80, '-') << std::endl;
        
        for (const auto& body : bodies) {
            if (body.name == "Asteroid") continue;  // Skip asteroids
            
            file << "Name: " << body.name << std::endl;
            file << "  Mass: " << body.mass << " Solar Masses" << std::endl;
            file << "  Position (AU): [" 
                 << body.position.x << ", " 
                 << body.position.y << ", " 
                 << body.position.z << "]" << std::endl;
            file << "  Velocity (AU/yr): [" 
                 << body.velocity.x << ", " 
                 << body.velocity.y << ", " 
                 << body.velocity.z << "]" << std::endl;
            file << "  Distance from Origin: " << body.position.length() << " AU" << std::endl;
            file << "  Speed: " << body.velocity.length() << " AU/yr" << std::endl;
            
            // Approximate orbital period (Kepler's 3rd law)
            double dist = body.position.length();
            if (dist > 0.01) {
                double T = std::sqrt(dist * dist * dist);
                file << "  Est. Orbital Period: " << T << " years" << std::endl;
            }
            file << std::endl;
        }
        
        file.close();
        std::cout << "Exported summary to: " << filename << std::endl;
        return true;
    }

    /**
     * @brief Starts a trajectory recording session.
     * Creates/overwrites the CSV file with header.
     */
    static bool startRecording(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) return false;
        
        file << "timestamp,name,pos_x,pos_y,pos_z,vel_x,vel_y,vel_z\n";
        file.close();
        std::cout << "Started recording to: " << filename << std::endl;
        return true;
    }

    /**
     * @brief Records current frame to trajectory file.
     */
    static bool recordFrame(const std::vector<Body>& bodies,
                           const std::string& filename,
                           double timestamp) {
        std::ofstream file(filename, std::ios::app);
        if (!file.is_open()) return false;
        
        for (const auto& body : bodies) {
            if (body.name == "Asteroid") continue;
            file << std::fixed << std::setprecision(6)
                 << timestamp << "," << body.name << ","
                 << body.position.x << "," << body.position.y << "," << body.position.z << ","
                 << body.velocity.x << "," << body.velocity.y << "," << body.velocity.z << "\n";
        }
        
        file.close();
        return true;
    }
};

} // namespace SolarSim
