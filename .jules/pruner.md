# PRUNER'S JOURNAL - PRUNING LOG

## 2026-01-07 - [Dead Code Cleanup] 
**Scope:** `GraphicsEngine.hpp`, `main.cpp`
**Removed:** 
- `GraphicsEngine::calculateMoonVisualPosition(const Body&, const Body&)` (Legacy overload)
- `GraphicsEngine::drawAxes` (Unused debug helper)
- Verified that all major headers in `include/` are actively used by the core simulation or tests.

**Impact:** 
- Reduced technical debt by removing unmaintained debug/legacy code.
- Cleaned up internal API of `GraphicsEngine`.
- Build time Δ: Negligible.
- Bundle size Δ: Negligible.

**Notes:** 
- `KeplerianSolver.hpp` was flagged but confirmed to be used by `EphemerisLoader.hpp` for initial body state generation.
- `captureScreen` in `main.cpp` was kept as it is utilized by the `--mission` mode.
- E2E tests confirmed no regression in physics or rendering.
