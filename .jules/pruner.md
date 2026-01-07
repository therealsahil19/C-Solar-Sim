# PRUNER'S JOURNAL - PRUNING LOG

## 2026-01-07 - [Dead Code Cleanup Session 2]
**Scope:** `GraphicsEngine.hpp`, `GuiEngine.hpp`
**Removed:**
- `GraphicsEngine::drawAxes` (29 lines) - Unused debug axis renderer
- `GuiEngine::renderStats` (8 lines) - Legacy console output fallback

**Impact:**
- Reduced technical debt by removing unused debug/legacy code.
- Lines removed: 37
- Build time Δ: Negligible.
- Bundle size Δ: Negligible.

**Notes:**
- Comprehensive codebase scan found only 2 unused functions.
- No orphaned files, unused dependencies, or dead imports detected.
- All 13 E2E tests pass after removal.

---

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
