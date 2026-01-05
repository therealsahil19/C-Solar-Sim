#pragma once

#include <vector>
#include <iostream>
#include <string>
#include <algorithm>
#include "Body.hpp"

namespace SolarSim {

/**
 * @brief Simple ASCII-based console renderer for the simulation.
 */
class ConsoleRenderer {
public:
    static void render(const std::vector<Body>& bodies, double scale = 10.0, int width = 80, int height = 40) {
        // Clear screen using ANSI escape codes
        std::cout << "\033[2J\033[1;1H";

        // Create character buffer
        std::vector<std::string> buffer(height, std::string(width, ' '));

        // Draw center (Sun is usually at 0,0 but we'll center it on screen)
        int centerX = width / 2;
        int centerY = height / 2;

        for (const auto& body : bodies) {
            // Project 3D to 2D (simple X-Y projection)
            int x = static_cast<int>(body.position.x * scale) + centerX;
            int y = static_cast<int>(body.position.y * (scale * 0.5)) + centerY; // Adjust aspect ratio for console

            if (x >= 0 && x < width && y >= 0 && y < height) {
                char symbol = body.name[0];
                buffer[y][x] = symbol;
            }
        }

        // Print buffer
        for (const auto& row : buffer) {
            std::cout << row << "\n";
        }
    }
};

} // namespace SolarSim
