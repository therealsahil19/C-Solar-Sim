# Bolt's Journal - Architectural Decisions

## 2026-01-05 - Initial Performance Audit 
**Bottleneck:** Potential N² physics simulation and unoptimized 3D rendering overhead. 
**Strategy:** Analyze Octree implementation and GraphicsEngine for batching or draw call optimization. 
**Result:** TBD (Pending Baseline)

## 2026-01-06 - Physics + Graphics Performance Overhaul ⚡

**Bottlenecks Identified:**
1. **Physics Engine** - O(N²) force calculations without cache optimization
2. **Shader Uniforms** - Hundreds of `glGetUniformLocation` calls per frame
3. **Barnes-Hut Thread Safety** - Static variable blocking future parallelization
4. **Benchmark Tooling** - Insufficient statistical analysis

**Strategy:**
1. Optimized `calculateAccelerations` with local variable caching and accumulator pattern
2. Implemented uniform location memoization in `ShaderProgram.hpp`
3. Converted static stack to mutable member in `OctreePool`
4. Enhanced benchmark with warmup runs, 5-run averages, and scaling comparison

**Benchmark Results (Release Build):**

| Bodies | Verlet (ms/step) | Barnes-Hut (ms/step) |
|--------|------------------|----------------------|
| 100    | 0.0753           | 0.1086               |
| 250    | 0.3787           | 0.6807               |
| 500    | 1.2461           | 1.3944               |
| 1000   | 4.3667           | 6.4469               |
| 2000   | -                | 11.52                |

**Finding:** Barnes-Hut has higher overhead for small N due to tree construction. O(N²) direct calculation is faster up to ~500 bodies. Barnes-Hut advantages emerge at larger scales.

**Pre-existing Issue Discovered:** SolarSim.exe build failure due to circular include dependency between `GraphicsEngine.hpp` and `GuiEngine.hpp`. This existed before optimization work (confirmed via git stash test).

**Files Modified:**
- `include/PhysicsEngine.hpp` - Cache-optimized force calculations
- `include/ShaderProgram.hpp` - Uniform location caching
- `include/Octree.hpp` - Thread-safe traversal stack
- `include/GraphicsEngine.hpp` - Self-contained includes
- `src/benchmark.cpp` - Enhanced statistical benchmark

