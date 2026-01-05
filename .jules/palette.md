# Palette's Journal - Design System Decisions

This journal logs major design system choices and accessibility baselines for the Solar System Simulation project.

## 2026-01-05 - [Initial Audit & Tokenization]
**Problem:** Hardcoded color values and inconsistent UI patterns across `GuiEngine` and `GraphicsEngine`. Lack of accessibility features (keyboard shortcuts, tooltips) and feedback systems (toasts).
**Solution:** Implementing a unified `Theme.hpp` to centralize Design Tokens. Introducing a Toast Notification system and global hotkeys. 
**Standard:** WCAG 2.1 AA (Contrast & Keyboard accessibility).

## 2026-01-05 - [Systemic feedback]
**Problem:** Simulation state changes (Save/Load/Presets) are silent or only logged to console, providing no feedback to the user.
**Solution:** Implement a non-blocking Toast/Notification system within `GuiEngine`.
**Standard:** UX Principle (Feedback).
