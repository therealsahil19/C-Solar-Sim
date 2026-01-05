#include <iostream>
#include <vector>
#include <chrono>
#include "PhysicsEngine.hpp"
#include "Body.hpp"

void run_physics_benchmark(int n_bodies, int steps, const std::string& method) {
    std::vector<SolarSim::Body> bodies;
    for (int i = 0; i < n_bodies; ++i) {
        bodies.emplace_back("Asteroid", 1.0, 0.1, 
                           SolarSim::Vector3(i * 1.0, 0, 0), 
                           SolarSim::Vector3(0, i * 0.1, 0));
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < steps; ++i) {
        if (method == "Verlet") {
            SolarSim::PhysicsEngine::stepVerlet(bodies, 0.01);
        } else if (method == "RK4") {
            SolarSim::PhysicsEngine::stepRK4(bodies, 0.01);
        } else if (method == "BarnesHut") {
            SolarSim::PhysicsEngine::stepBarnesHut(bodies, 0.01, 0.5);
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    std::cout << method << " (" << n_bodies << " bodies, " << steps << " steps): " << diff.count() << "s (" << (diff.count() / steps) * 1000.0 << " ms/step)" << std::endl;
}

int main() {
    std::cout << "--- SolarSim Performance Benchmark ---" << std::endl;
    
    // Benchmark physics
    run_physics_benchmark(100, 100, "Verlet");
    run_physics_benchmark(100, 100, "RK4");
    run_physics_benchmark(100, 100, "BarnesHut");
    
    run_physics_benchmark(500, 50, "Verlet");
    run_physics_benchmark(500, 50, "BarnesHut");

    return 0;
}
