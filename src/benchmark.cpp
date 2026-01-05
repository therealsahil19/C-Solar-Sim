#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <string>
#include "PhysicsEngine.hpp"
#include "Body.hpp"

/**
 * @brief Enhanced physics benchmark with statistical analysis.
 * 
 * Bolt Unleashed: Measures actual performance with warmup iterations,
 * multiple sample runs, and statistical reporting (min, max, avg, stddev).
 */

struct BenchmarkResult {
    std::string name;
    int bodies;
    int steps;
    double minMs;
    double maxMs;
    double avgMs;
    double stddevMs;
    double totalS;
};

std::vector<SolarSim::Body> createTestBodies(int n) {
    std::vector<SolarSim::Body> bodies;
    bodies.reserve(n);
    for (int i = 0; i < n; ++i) {
        double angle = (2.0 * 3.14159265359 * i) / n;
        double radius = 1.0 + (i % 10) * 0.5;
        SolarSim::Vector3 pos(radius * std::cos(angle), radius * std::sin(angle), (i % 5) * 0.1 - 0.25);
        SolarSim::Vector3 vel(-std::sin(angle) * 0.5, std::cos(angle) * 0.5, 0);
        bodies.push_back(SolarSim::Body("Body" + std::to_string(i), 1.0 / n, 0.001, pos, vel));
    }
    return bodies;
}

BenchmarkResult runBenchmark(const std::string& method, int nBodies, int steps, int warmupRuns = 3, int measureRuns = 5) {
    std::vector<double> timings;
    timings.reserve(measureRuns);
    
    // Warmup runs (not measured)
    for (int w = 0; w < warmupRuns; ++w) {
        auto bodies = createTestBodies(nBodies);
        SolarSim::PhysicsEngine::calculateAccelerations(bodies);
        for (int i = 0; i < steps / 2; ++i) {
            if (method == "Verlet") {
                SolarSim::PhysicsEngine::stepVerlet(bodies, 0.01);
            } else if (method == "RK4") {
                SolarSim::PhysicsEngine::stepRK4(bodies, 0.01);
            } else if (method == "BarnesHut") {
                SolarSim::PhysicsEngine::stepBarnesHut(bodies, 0.01, 0.5);
            }
        }
    }
    
    // Measurement runs
    for (int r = 0; r < measureRuns; ++r) {
        auto bodies = createTestBodies(nBodies);
        SolarSim::PhysicsEngine::calculateAccelerations(bodies);
        
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
        std::chrono::duration<double, std::milli> diff = end - start;
        timings.push_back(diff.count() / steps);
    }
    
    // Calculate statistics
    double sum = std::accumulate(timings.begin(), timings.end(), 0.0);
    double avg = sum / timings.size();
    
    double sqSum = 0.0;
    for (double t : timings) sqSum += (t - avg) * (t - avg);
    double stddev = std::sqrt(sqSum / timings.size());
    
    double minT = *std::min_element(timings.begin(), timings.end());
    double maxT = *std::max_element(timings.begin(), timings.end());
    
    return {method, nBodies, steps, minT, maxT, avg, stddev, sum * steps / 1000.0};
}

void printResult(const BenchmarkResult& r) {
    std::cout << std::setw(12) << r.name 
              << " | " << std::setw(6) << r.bodies << " bodies"
              << " | " << std::setw(4) << r.steps << " steps"
              << " | avg: " << std::fixed << std::setprecision(4) << std::setw(8) << r.avgMs << " ms/step"
              << " | min: " << std::setw(8) << r.minMs
              << " | max: " << std::setw(8) << r.maxMs
              << " | σ: " << std::setw(6) << r.stddevMs
              << std::endl;
}

int main() {
    std::cout << "============================================================" << std::endl;
    std::cout << "⚡ Bolt Unleashed: SolarSim Performance Benchmark ⚡" << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << "(Warmup: 3 runs | Measured: 5 runs per config)" << std::endl;
    std::cout << std::endl;
    
    std::vector<BenchmarkResult> results;
    
    // Standard benchmarks
    std::cout << "--- Standard Benchmarks (100-500 bodies) ---" << std::endl;
    results.push_back(runBenchmark("Verlet", 100, 100));
    printResult(results.back());
    results.push_back(runBenchmark("RK4", 100, 100));
    printResult(results.back());
    results.push_back(runBenchmark("BarnesHut", 100, 100));
    printResult(results.back());
    results.push_back(runBenchmark("Verlet", 500, 50));
    printResult(results.back());
    results.push_back(runBenchmark("BarnesHut", 500, 50));
    printResult(results.back());
    
    std::cout << std::endl;
    std::cout << "--- Stress Test (1000+ bodies) ---" << std::endl;
    results.push_back(runBenchmark("Verlet", 1000, 20));
    printResult(results.back());
    results.push_back(runBenchmark("BarnesHut", 1000, 20));
    printResult(results.back());
    results.push_back(runBenchmark("BarnesHut", 2000, 10));
    printResult(results.back());
    
    std::cout << std::endl;
    std::cout << "--- O(N²) vs O(N log N) Scaling Comparison ---" << std::endl;
    std::cout << "Bodies | Verlet (ms/step) | Barnes-Hut (ms/step) | Speedup" << std::endl;
    std::cout << "-------|------------------|----------------------|--------" << std::endl;
    for (int n : {100, 250, 500, 1000}) {
        auto verlet = runBenchmark("Verlet", n, 20, 2, 3);
        auto bh = runBenchmark("BarnesHut", n, 20, 2, 3);
        double speedup = verlet.avgMs / bh.avgMs;
        std::cout << std::setw(6) << n << " | " 
                  << std::setw(16) << std::fixed << std::setprecision(4) << verlet.avgMs << " | "
                  << std::setw(20) << bh.avgMs << " | "
                  << std::setw(6) << std::setprecision(2) << speedup << "x"
                  << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << "Benchmark complete." << std::endl;
    
    return 0;
}
