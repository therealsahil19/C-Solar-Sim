#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <vector>
#include <cassert>
#include "Body.hpp"
#include "PhysicsEngine.hpp"
#include "Validator.hpp"
#include "StateManager.hpp"
#include "SystemData.hpp"

using namespace SolarSim;

void test_physics_stability() {
    std::cout << "[TEST] Physics Stability (RK4 & Energy Conservation)..." << std::endl;
    
    auto bodies = StateManager::loadPreset(PresetType::InnerPlanets);
    assert(!bodies.empty());
    
    // Ensure system is barycentric for better conservation
    convertToBarycentric(bodies);
    
    // Test RK4 stability over short term
    double dt = 0.001;
    int steps = 1000;
    double initialEnergy = PhysicsEngine::calculateTotalEnergy(bodies);
    
    for (int i = 0; i < steps; ++i) {
        PhysicsEngine::stepRK4(bodies, dt);
    }
    
    double finalEnergy = PhysicsEngine::calculateTotalEnergy(bodies);
    double drift = std::abs((finalEnergy - initialEnergy) / initialEnergy);
    
    std::cout << "  RK4 Energy Drift (1000 steps): " << (drift * 100) << "%" << std::endl;
    assert(drift < 1e-5);
    
    // Use Validator for longer term (1 year)
    auto validatorBodies = StateManager::loadPreset(PresetType::InnerPlanets);
    convertToBarycentric(validatorBodies);
    auto result = Validator::validateOrbitalPeriods(validatorBodies, 0.001, 1.0);
    Validator::printReport(result);
    assert(result.passed);
    
    std::cout << "[PASS] Physics Stability" << std::endl << std::endl;
}

void test_state_persistence() {
    std::cout << "[TEST] State Persistence (Save/Load)..." << std::endl;
    
    auto bodies = StateManager::loadPreset(PresetType::InnerPlanets);
    std::string testFile = "test_state_verification.csv";
    
    bool saved = StateManager::saveState(bodies, testFile);
    assert(saved);
    
    auto loadedBodies = StateManager::loadState(testFile);
    assert(loadedBodies.size() == bodies.size());
    
    for (size_t i = 0; i < bodies.size(); ++i) {
        assert(bodies[i].name == loadedBodies[i].name);
        assert(std::abs(bodies[i].mass - loadedBodies[i].mass) < 1e-9);
        assert((bodies[i].position - loadedBodies[i].position).length() < 1e-6);
        assert((bodies[i].velocity - loadedBodies[i].velocity).length() < 1e-6);
    }
    
    std::cout << "[PASS] State Persistence" << std::endl << std::endl;
}

void test_presets() {
    std::cout << "[TEST] Preset Integrity..." << std::endl;
    
    PresetType types[] = {
        PresetType::FullSolarSystem,
        PresetType::InnerPlanets,
        PresetType::OuterGiants,
        PresetType::EarthMoonSystem,
        PresetType::BinaryStarTest
    };
    
    for (auto type : types) {
        auto bodies = StateManager::loadPreset(type);
        std::cout << "  Loaded " << StateManager::getPresetName(type) << ": " << bodies.size() << " bodies." << std::endl;
        assert(!bodies.empty());
        if (type == PresetType::BinaryStarTest) assert(bodies.size() == 2);
    }
    
    std::cout << "[PASS] Preset Integrity" << std::endl << std::endl;
}

int main() {
    std::cout << "=== SolarSim Verifier: E2E Suite ===" << std::endl << std::endl;
    
    try {
        test_physics_stability();
        test_state_persistence();
        test_presets();
        
        std::cout << "=====================================" << std::endl;
        std::cout << "✅ ALL TESTS PASSED SUCCESSFULLY" << std::endl;
        std::cout << "=====================================" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "❌ TEST FAILED: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ TEST FAILED: Unknown error" << std::endl;
        return 1;
    }
    
    return 0;
}
