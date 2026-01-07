# BugHunter's Journal - Bug Log

## 2026-01-07 - [GUI & Graphics Deep Scan]
**Scope:** include/GuiEngine.hpp, include/GraphicsEngine.hpp, include/Octree.hpp
**Found:** 1 Critical, 3 High, 5 Medium, 4 Low
**Notable:** Resource leak in `GraphicsEngine` (~missing VBO deletion); OOB potential in `OctreeNode` if logic fails; Hardcoded epsilon in depth check.
**Coverage Notes:** `verify_features.cpp` likely doesn't test edge-case collisions.

---

## 2026-01-07 - [Bug Fix Session]
**Bugs Fixed:** BUG-001 (VBO Leak), BUG-002 (Division-by-Zero), BUG-003 (Undocumented Epsilon)
**Approach:** Minimal - single-line additions, no behavioral changes to happy path
**Tests Added:** Leveraged existing `test_integrator_barnes_hut()` for regression testing
**Impact:** Critical GPU memory leak resolved; division-by-zero edge case eliminated; code quality improved with documented constant
