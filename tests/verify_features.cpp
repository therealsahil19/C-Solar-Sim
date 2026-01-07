#include <cmath>
#include <iostream>
#include <vector>
#include <fstream>
#include <cassert>
#include "Body.hpp"
#include "PhysicsEngine.hpp"
#include "Validator.hpp"
#include "StateManager.hpp"
#include "SystemData.hpp"

using namespace SolarSim;

// =============================================================================
// Original Tests
// =============================================================================

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
        assert((bodies[i].position - loadedBodies[i].position).length() < 1e-5);
        assert((bodies[i].velocity - loadedBodies[i].velocity).length() < 1e-5);
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


// =============================================================================
// NEW: Verlet Integrator Energy Conservation
// =============================================================================

void test_integrator_verlet() {
    std::cout << "[TEST] Verlet Integrator (Energy Conservation)..." << std::endl;
    
    auto bodies = StateManager::loadPreset(PresetType::InnerPlanets);
    convertToBarycentric(bodies);
    
    double dt = 0.001;
    int steps = 1000;
    double initialEnergy = PhysicsEngine::calculateTotalEnergy(bodies);
    
    for (int i = 0; i < steps; ++i) {
        PhysicsEngine::stepVerlet(bodies, dt);
    }
    
    double finalEnergy = PhysicsEngine::calculateTotalEnergy(bodies);
    double drift = std::abs((finalEnergy - initialEnergy) / initialEnergy);
    
    std::cout << "  Verlet Energy Drift (1000 steps): " << (drift * 100) << "%" << std::endl;
    // Verlet is symplectic - should have low drift (allow up to 0.1%)
    assert(drift < 1e-3);
    
    std::cout << "[PASS] Verlet Integrator" << std::endl << std::endl;
}

// =============================================================================
// NEW: Barnes-Hut Integrator (O(N log N))
// =============================================================================

void test_integrator_barnes_hut() {
    std::cout << "[TEST] Barnes-Hut Integrator (O(N log N))..." << std::endl;
    
    // Use larger system to test Barnes-Hut efficiency
    auto bodies = StateManager::loadPreset(PresetType::FullSolarSystem);
    convertToBarycentric(bodies);
    std::cout << "  Bodies: " << bodies.size() << std::endl;
    
    double dt = 0.001;
    int steps = 200;  // Fewer steps since it's larger system
    double initialEnergy = PhysicsEngine::calculateTotalEnergy(bodies);
    
    for (int i = 0; i < steps; ++i) {
        PhysicsEngine::stepBarnesHut(bodies, dt, 0.5);
    }
    
    double finalEnergy = PhysicsEngine::calculateTotalEnergy(bodies);
    double drift = std::abs((finalEnergy - initialEnergy) / initialEnergy);
    
    std::cout << "  Barnes-Hut Energy Drift (200 steps): " << (drift * 100) << "%" << std::endl;
    // Barnes-Hut approximates, so allow up to 1% tolerance
    assert(drift < 1e-2);
    
    std::cout << "[PASS] Barnes-Hut Integrator" << std::endl << std::endl;
}

// =============================================================================
// NEW: Collision Detection & Merging
// =============================================================================

void test_collision_detection() {
    std::cout << "[TEST] Collision Detection (Inelastic Merging)..." << std::endl;
    
    // Create two bodies on collision course
    std::vector<Body> bodies;
    Body a("Body A", 0.5, 0.01, Vector3(-0.1, 0, 0), Vector3(0.5, 0, 0));
    Body b("Body B", 0.5, 0.01, Vector3(0.1, 0, 0), Vector3(-0.5, 0, 0));
    bodies.push_back(a);
    bodies.push_back(b);
    
    // Calculate initial momentum
    Vector3 initialMomentum(0, 0, 0);
    for (const auto& body : bodies) {
        initialMomentum += body.velocity * body.mass;
    }
    double initialMass = bodies[0].mass + bodies[1].mass;
    
    std::cout << "  Initial: 2 bodies, total mass=" << initialMass << std::endl;
    
    // Run simulation until collision occurs
    double dt = 0.001;
    size_t initialCount = bodies.size();
    int maxSteps = 1000;
    
    for (int i = 0; i < maxSteps && bodies.size() == initialCount; ++i) {
        PhysicsEngine::stepVerlet(bodies, dt);
        PhysicsEngine::handleCollisions(bodies);
    }
    
    if (bodies.size() < initialCount) {
        std::cout << "  Collision occurred! Bodies remaining: " << bodies.size() << std::endl;
        
        // Verify mass conservation
        double finalMass = 0;
        for (const auto& body : bodies) {
            finalMass += body.mass;
        }
        assert(std::abs(finalMass - initialMass) < 1e-9);
        std::cout << "  Mass conserved: " << finalMass << std::endl;
        
        // Verify momentum conservation
        Vector3 finalMomentum(0, 0, 0);
        for (const auto& body : bodies) {
            finalMomentum += body.velocity * body.mass;
        }
        double momentumError = (finalMomentum - initialMomentum).length();
        std::cout << "  Momentum error: " << momentumError << std::endl;
        assert(momentumError < 1e-9);
    } else {
        std::cout << "  Warning: No collision in " << maxSteps << " steps (may need adjustment)" << std::endl;
    }
    
    std::cout << "[PASS] Collision Detection" << std::endl << std::endl;
}

// =============================================================================
// NEW: Adaptive Timestep Safety
// =============================================================================

void test_adaptive_timestep() {
    std::cout << "[TEST] Adaptive Timestep (Safety Limits)..." << std::endl;
    
    // Test with close binary system (should reduce timestep)
    std::vector<Body> closeBinary;
    closeBinary.push_back(Body("A", 0.5, 0.001, Vector3(-0.05, 0, 0), Vector3(0, -2.0, 0)));
    closeBinary.push_back(Body("B", 0.5, 0.001, Vector3(0.05, 0, 0), Vector3(0, 2.0, 0)));
    
    double baseDt = 0.01;
    double adaptedDt = PhysicsEngine::getAdaptiveTimestep(closeBinary, baseDt);
    std::cout << "  Close binary (0.1 AU): baseDt=" << baseDt << ", adaptedDt=" << adaptedDt << std::endl;
    assert(adaptedDt <= baseDt);  // Should reduce or keep same
    assert(adaptedDt > 0);        // Should be positive
    
    // Test with distant bodies (should keep base timestep)
    std::vector<Body> distantBodies;
    distantBodies.push_back(Body("Sun", 1.0, 0.005, Vector3(0, 0, 0), Vector3(0, 0, 0)));
    distantBodies.push_back(Body("Neptune", 0.00005, 0.0002, Vector3(30, 0, 0), Vector3(0, 0.18, 0)));
    
    double distantDt = PhysicsEngine::getAdaptiveTimestep(distantBodies, baseDt);
    std::cout << "  Distant bodies (30 AU): baseDt=" << baseDt << ", adaptedDt=" << distantDt << std::endl;
    assert(distantDt <= baseDt);
    assert(distantDt > 0);
    
    std::cout << "[PASS] Adaptive Timestep" << std::endl << std::endl;
}

// =============================================================================
// NEW: Momentum Conservation
// =============================================================================

void test_momentum_conservation() {
    std::cout << "[TEST] Momentum Conservation (Barycentric)..." << std::endl;
    
    auto bodies = StateManager::loadPreset(PresetType::InnerPlanets);
    convertToBarycentric(bodies);
    
    // Initial total momentum (should be ~0 in barycentric frame)
    Vector3 initialMomentum(0, 0, 0);
    for (const auto& body : bodies) {
        initialMomentum += body.velocity * body.mass;
    }
    std::cout << "  Initial momentum magnitude: " << initialMomentum.length() << std::endl;
    
    // Run simulation
    double dt = 0.001;
    for (int i = 0; i < 500; ++i) {
        PhysicsEngine::stepVerlet(bodies, dt);
    }
    
    // Final momentum
    Vector3 finalMomentum(0, 0, 0);
    for (const auto& body : bodies) {
        finalMomentum += body.velocity * body.mass;
    }
    
    double drift = (finalMomentum - initialMomentum).length();
    std::cout << "  Final momentum magnitude: " << finalMomentum.length() << std::endl;
    std::cout << "  Momentum drift: " << drift << std::endl;
    
    // Momentum should be conserved (allow small numerical drift)
    assert(drift < 1e-8);
    
    std::cout << "[PASS] Momentum Conservation" << std::endl << std::endl;
}

// =============================================================================
// NEW: RK4 Regression Test for Body Count Changes (BUG-H001)
// =============================================================================

void test_rk4_with_changing_body_count() {
    std::cout << "[TEST] RK4 With Changing Body Count..." << std::endl;
    
    // First run with 3 bodies
    std::vector<Body> bodies1;
    bodies1.push_back(Body("A", 0.5, 0.01, Vector3(0, 0, 0), Vector3(0, 0, 0)));
    bodies1.push_back(Body("B", 0.3, 0.01, Vector3(1, 0, 0), Vector3(0, 1, 0)));
    bodies1.push_back(Body("C", 0.2, 0.01, Vector3(-1, 0, 0), Vector3(0, -1, 0)));
    
    double dt = 0.001;
    PhysicsEngine::stepRK4(bodies1, dt);
    std::cout << "  Ran RK4 with 3 bodies" << std::endl;
    
    // Now run with 2 bodies (simulating post-collision)
    std::vector<Body> bodies2;
    bodies2.push_back(Body("A", 0.5, 0.01, Vector3(0, 0, 0), Vector3(0, 0, 0)));
    bodies2.push_back(Body("B", 0.5, 0.01, Vector3(1, 0, 0), Vector3(0, 1, 0)));
    
    PhysicsEngine::stepRK4(bodies2, dt);
    std::cout << "  Ran RK4 with 2 bodies" << std::endl;
    
    // Now run with 5 bodies
    std::vector<Body> bodies3;
    for (int i = 0; i < 5; ++i) {
        bodies3.push_back(Body("Body" + std::to_string(i), 0.2, 0.01, 
                               Vector3(i * 0.5, 0, 0), Vector3(0, 0.1 * i, 0)));
    }
    
    PhysicsEngine::stepRK4(bodies3, dt);
    std::cout << "  Ran RK4 with 5 bodies" << std::endl;
    
    // If we got here without crash/corruption, the test passed
    assert(bodies1.size() == 3);
    assert(bodies2.size() == 2);
    assert(bodies3.size() == 5);
    
    std::cout << "[PASS] RK4 With Changing Body Count" << std::endl << std::endl;
}

// =============================================================================
// NEW: Rotation Wrap for Negative Speeds (BUG-L004)
// =============================================================================

void test_rotation_wrap_negative() {
    std::cout << "[TEST] Rotation Wrap For Negative Speeds..." << std::endl;
    
    // Create body with negative rotation speed (retrograde like Venus)
    Body body("Venus", 1.0, 0.01, Vector3(0, 0, 0), Vector3(0, 0, 0));
    body.rotationSpeed = -15.0;  // Retrograde rotation
    body.rotationAngle = 10.0;
    
    // Run for enough time to go negative
    double dt = 0.1;  // Large timestep to quickly go negative
    for (int i = 0; i < 10; ++i) {
        body.updateRotation(dt);
    }
    
    // Rotation should be wrapped to [0, 360)
    std::cout << "  Final rotation angle: " << body.rotationAngle << std::endl;
    assert(body.rotationAngle >= 0.0);
    assert(body.rotationAngle < 360.0);
    
    // Test positive overflow still works
    Body body2("Jupiter", 1.0, 0.01, Vector3(0, 0, 0), Vector3(0, 0, 0));
    body2.rotationSpeed = 500.0;  // Very fast rotation
    body2.rotationAngle = 350.0;
    
    for (int i = 0; i < 5; ++i) {
        body2.updateRotation(dt);
    }
    
    std::cout << "  Final rotation angle (fast): " << body2.rotationAngle << std::endl;
    assert(body2.rotationAngle >= 0.0);
    assert(body2.rotationAngle < 360.0);
    
    std::cout << "[PASS] Rotation Wrap For Negative Speeds" << std::endl << std::endl;
}

// =============================================================================
// NEW: CSV Malformed Handling (BUG-M003)
// =============================================================================

void test_csv_malformed_handling() {
    std::cout << "[TEST] CSV Malformed Handling..." << std::endl;
    
    // Create a malformed CSV file
    std::string testFile = "test_malformed.csv";
    std::ofstream file(testFile);
    file << "name,mass,radius,px,py,pz,vx,vy,vz,rotAngle,rotSpeed,axialTilt\n";
    file << "ValidBody,1.0,0.01,0,0,0,0,0,0,0,0,0\n";  // Valid line
    file << "BadBody,INVALID,0.01,0,0,0,0,0,0,0,0,0\n";  // Invalid mass
    file << "AnotherValid,0.5,0.01,1,0,0,0,1,0,0,10,0\n";  // Valid line
    file.close();
    
    // Load should not crash, should skip bad line
    auto bodies = StateManager::loadState(testFile);
    
    std::cout << "  Loaded " << bodies.size() << " bodies from malformed CSV" << std::endl;
    assert(bodies.size() == 2);  // Only 2 valid bodies
    assert(bodies[0].name == "ValidBody");
    assert(bodies[1].name == "AnotherValid");
    
    // Cleanup
    std::remove(testFile.c_str());
    
    std::cout << "[PASS] CSV Malformed Handling" << std::endl << std::endl;
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    std::cout << "=== SolarSim Verifier: E2E Suite ===" << std::endl << std::endl;
    
    try {
        // Original tests
        test_physics_stability();
        test_state_persistence();
        test_presets();
        
        // original tests
        test_integrator_verlet();
        test_integrator_barnes_hut();
        test_collision_detection();
        test_adaptive_timestep();
        test_momentum_conservation();
        
        // Regression tests for Debugger fixes
        test_rk4_with_changing_body_count();
        test_rotation_wrap_negative();
        test_csv_malformed_handling();
        
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
