#pragma once

#include <vector>
#include <iostream>
#include <sstream>
#include <cmath>
#include "Body.hpp"
#include "PhysicsEngine.hpp"
#include "KeplerianSolver.hpp"

namespace SolarSim {

/**
 * @brief Validation tools for long-term simulation accuracy.
 */
class Validator {
public:
    /**
     * @brief Results from validation run.
     */
    struct ValidationResult {
        bool passed;
        double maxEnergyDrift;
        double maxMomentumDrift;
        double earthPeriodError;
        std::string summary;
    };

    /**
     * @brief Validates orbital periods against Kepler's 3rd law.
     * Runs simulation and checks that Earth completes ~1 orbit per year.
     * @param bodies Initial bodies
     * @param dt Base timestep
     * @param years Number of years to simulate
     * @return Validation result
     */
    static ValidationResult validateOrbitalPeriods(
        std::vector<Body> bodies, double dt, double years = 1.0) 
    {
        ValidationResult result;
        result.passed = true;
        
        // Find Earth's initial position
        Vector3 earthInitialPos;
        int earthIdx = -1;
        for (size_t i = 0; i < bodies.size(); ++i) {
            if (bodies[i].name == "Earth") {
                earthInitialPos = bodies[i].position;
                earthIdx = (int)i;
                break;
            }
        }
        
        if (earthIdx < 0) {
            result.passed = false;
            result.summary = "Earth not found in simulation";
            return result;
        }
        
        // Initial energy
        double initialEnergy = PhysicsEngine::calculateTotalEnergy(bodies);
        double maxEnergyDrift = 0.0;
        
        // Initial momentum
        Vector3 initialMomentum(0, 0, 0);
        for (const auto& b : bodies) {
            initialMomentum += b.velocity * b.mass;
        }
        double maxMomentumDrift = 0.0;
        
        // Run simulation for specified years
        int stepsPerYear = (int)(1.0 / dt);
        int totalSteps = (int)(stepsPerYear * years);
        
        for (int step = 0; step < totalSteps; ++step) {
            PhysicsEngine::stepVerlet(bodies, dt);
            
            // Check energy conservation periodically
            if (step % 100 == 0) {
                double currentEnergy = PhysicsEngine::calculateTotalEnergy(bodies);
                double energyDrift = std::abs((currentEnergy - initialEnergy) / initialEnergy);
                maxEnergyDrift = std::max(maxEnergyDrift, energyDrift);
                
                // Check momentum conservation
                Vector3 momentum(0, 0, 0);
                for (const auto& b : bodies) {
                    momentum += b.velocity * b.mass;
                }
                double momentumDrift = (momentum - initialMomentum).length();
                maxMomentumDrift = std::max(maxMomentumDrift, momentumDrift);
            }
        }
        
        // Calculate Earth's final position vs initial
        Vector3 earthFinalPos = bodies[earthIdx].position;
        double distanceFromStart = (earthFinalPos - earthInitialPos).length();
        
        // After exactly 1 year, Earth should be back to ~same position
        // Allow for some error due to other bodies' influence
        result.earthPeriodError = distanceFromStart;
        result.maxEnergyDrift = maxEnergyDrift;
        result.maxMomentumDrift = maxMomentumDrift;
        
        // Thresholds for passing
        bool energyOk = maxEnergyDrift < 5e-4;       // 0.05% max energy drift
        bool momentumOk = maxMomentumDrift < 1e-8;   // Very small momentum drift
        bool periodOk = distanceFromStart < 0.1;     // Within 0.1 AU of start
        
        result.passed = energyOk && momentumOk && periodOk;
        
        std::stringstream ss;
        ss << "Validation Results after " << years << " year(s):\n";
        ss << "  Energy conservation: " << (energyOk ? "PASS" : "FAIL") 
           << " (max drift: " << (maxEnergyDrift * 100) << "%)\n";
        ss << "  Momentum conservation: " << (momentumOk ? "PASS" : "FAIL")
           << " (max drift: " << maxMomentumDrift << ")\n";
        ss << "  Earth orbital period: " << (periodOk ? "PASS" : "FAIL")
           << " (distance from start: " << distanceFromStart << " AU)\n";
        result.summary = ss.str();
        
        return result;
    }

    /**
     * @brief Quick validation check for energy conservation.
     * 
     * @details
     * Checks if the relative energy drift exceeds the **Scribe Standard** of 0.05% 
     * ($\Delta E / E_0 < 5 \times 10^{-4}$).
     * 
     * @param bodies Bodies to check
     * @param steps Number of steps to run
     * @param dt Timestep
     * @return Max relative energy drift
     */
    static double quickEnergyCheck(std::vector<Body> bodies, int steps, double dt) {
        double initialEnergy = PhysicsEngine::calculateTotalEnergy(bodies);
        double maxDrift = 0.0;
        
        for (int i = 0; i < steps; ++i) {
            PhysicsEngine::stepVerlet(bodies, dt);
            if (i % 10 == 0) {
                double energy = PhysicsEngine::calculateTotalEnergy(bodies);
                double drift = std::abs((energy - initialEnergy) / initialEnergy);
                maxDrift = std::max(maxDrift, drift);
            }
        }
        
        return maxDrift;
    }

    /**
     * @brief Prints validation report to console.
     */
    static void printReport(const ValidationResult& result) {
        std::cout << "\n=== VALIDATION REPORT ===" << std::endl;
        std::cout << result.summary;
        std::cout << "Overall: " << (result.passed ? "PASSED" : "FAILED") << std::endl;
        std::cout << "=========================" << std::endl;
    }
};

} // namespace SolarSim
