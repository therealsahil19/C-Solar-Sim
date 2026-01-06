# Important Instructions

> [!CAUTION]
> **The application MUST run at a minimum of 30 FPS at all times.**

This document contains critical requirements that must be maintained during any development work on this project.

---

## Performance Requirements

### Frame Rate
- **Minimum FPS: 30**
- The simulation must maintain at least 30 frames per second under normal operating conditions
- This requirement takes priority over visual fidelity if trade-offs are necessary

### Optimization Guidelines
When performance issues arise, consider the following in order:

1. **Reduce Sphere Resolution** - Lower latitude/longitude segments for rendered bodies
2. **Reduce Trail Length** - Decrease `MAX_TRAIL_POINTS` in `Body.hpp`
3. **Reduce Asteroid Count** - Fewer asteroids in the simulation
4. **Use Faster Integrator** - Verlet is faster than RK4; Barnes-Hut for large body counts
5. **Batch Rendering** - Use instanced rendering for identical objects
6. **Skip Expensive Calculations** - Cache values that don't change every frame

---

## Current Optimizations Applied

| Area | Original | Optimized | Impact |
|------|----------|-----------|--------|
| Sphere Resolution (Planets) | 32x32 | 16x16 | ~4x fewer triangles |
| Sphere Resolution (Asteroids) | 32x32 | 8x8 | ~16x fewer triangles |
| Trail Points | 1000 | 500 | 50% less trail geometry |
| Asteroid Count | 200 | 100 | 50% fewer bodies |
| Adaptive Timestep | Every frame | Cached | Reduced O(N²) calls |

---

## Benchmarking

Run the benchmark executable to measure physics performance:
```bash
cd build
./benchmark
```

Monitor FPS in the simulation window - it should display in the GUI.

---

## Contact

If the application drops below 30 FPS consistently, this is a **critical bug** that must be addressed before any other feature work.
