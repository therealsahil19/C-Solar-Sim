# Palette's Journal - Design System Decisions

This journal logs major design system choices and accessibility baselines for the Solar System Simulation project.

## 2026-01-06 - [Complete Token System & Theme Engine]
**Problem:** Incomplete design tokens missing focus states, typography scale, and animation timing. No light theme alternative. Limited keyboard accessibility for body selection.
**Solution:** 
- Extended `Theme.hpp` with FocusRing, Disabled, Muted, Accent colors
- Added Typography scale (Small→XL) and Animation timing tokens (Fast/Normal/Slow)
- Implemented Light theme namespace with full color palette
- Added keyboard navigation (Up/Down arrows) for body selection panel
- Implemented theme toggle button with instant feedback
- Added loading states with progress indicator for preset loading
**Standard:** WCAG 2.1 AA (Focus Indicators 2.4.7, Keyboard Navigation 2.1.1), Design Principle (Consistency, Feedback).

## 2026-01-05 - [Initial Audit & Tokenization]
**Problem:** Hardcoded color values and inconsistent UI patterns across `GuiEngine` and `GraphicsEngine`. Lack of accessibility features (keyboard shortcuts, tooltips) and feedback systems (toasts).
**Solution:** Implementing a unified `Theme.hpp` to centralize Design Tokens. Introducing a Toast Notification system and global hotkeys. 
**Standard:** WCAG 2.1 AA (Contrast & Keyboard accessibility).

## 2026-01-05 - [Systemic feedback]
**Problem:** Simulation state changes (Save/Load/Presets) are silent or only logged to console, providing no feedback to the user.
**Solution:** Implement a non-blocking Toast/Notification system within `GuiEngine`.
**Standard:** UX Principle (Feedback).
