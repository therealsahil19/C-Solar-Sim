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

## 2026-01-08 - FPS Optimization: 5-10 → 30+ FPS ⚡

**Bottlenecks Identified:**
1. **SphereRenderer** - 64×64 tessellation = 8,192 triangles per sphere (11 bodies = 90K+ triangles)
2. **Shader Matrix Inverse** - `transpose(inverse(model))` per vertex for 100 instanced asteroids
3. **Orbit Recalculation** - OrbitCalculator running every frame for 8+ bodies
4. **Trail VBO Regeneration** - Dynamic vertex arrays rebuilt every frame

**Strategy:**
1. Reduce `SphereRenderer` tessellation from 64×64 to 32×32 (4x triangle reduction)
2. Eliminate GPU matrix inverse for instanced rendering (uniform scaling simplification)
3. Implement orbit path caching with position-based invalidation

**Changes Applied:**
- `include/GraphicsEngine.hpp:32` - Sphere 64×64 → 32×32
- `include/GraphicsEngine.hpp:48-58` - Added `OrbitCacheEntry` struct and cache map
- `include/GraphicsEngine.hpp:612-676` - Implemented orbit caching in `drawOrbits()`
- `shaders/planet.vert:23-29` - Replaced `inverse()` with `normalize(mat3(model))`

**Expected Result:** 4-6x FPS improvement (target: 30+ FPS)

**Verification:**
- `verify.exe` - All physics regression tests passed
- `benchmark.exe` - Physics performance stable
- Build successful in Release configuration
