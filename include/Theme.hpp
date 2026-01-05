#pragma once

#include <imgui.h>
#include <SFML/Graphics/Color.hpp>
#include <map>
#include <string>

namespace SolarSim {
namespace Theme {

// ═══════════════════════════════════════════════════════════════════════════
// SEMANTIC COLORS - Core Palette
// ═══════════════════════════════════════════════════════════════════════════

static const ImVec4 Primary = ImVec4(0.30f, 0.40f, 0.60f, 1.00f);
static const ImVec4 PrimaryHover = ImVec4(0.40f, 0.50f, 0.70f, 1.00f);
static const ImVec4 PrimaryActive = ImVec4(0.20f, 0.30f, 0.50f, 1.00f);

static const ImVec4 Background = ImVec4(0.02f, 0.02f, 0.06f, 1.00f);
static const ImVec4 Surface = ImVec4(0.10f, 0.10f, 0.15f, 0.90f);
static const ImVec4 TitleBg = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
static const ImVec4 TitleBgActive = ImVec4(0.20f, 0.20f, 0.30f, 1.00f);
static const ImVec4 FrameBg = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);

// Status Colors
static const ImVec4 Success = ImVec4(0.20f, 0.70f, 0.30f, 1.00f);
static const ImVec4 Warning = ImVec4(0.90f, 0.60f, 0.10f, 1.00f);
static const ImVec4 Error = ImVec4(0.80f, 0.20f, 0.20f, 1.00f);
static const ImVec4 Info = ImVec4(0.20f, 0.60f, 0.90f, 1.00f);

// ═══════════════════════════════════════════════════════════════════════════
// ACCESSIBILITY - Focus & Interaction States (WCAG 2.1 AA)
// ═══════════════════════════════════════════════════════════════════════════

static const ImVec4 FocusRing = ImVec4(0.30f, 0.60f, 0.90f, 0.80f);
static const ImVec4 Disabled = ImVec4(0.40f, 0.40f, 0.45f, 0.60f);
static const ImVec4 Muted = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);
static const ImVec4 Accent = ImVec4(0.70f, 0.40f, 0.80f, 1.00f);

// Interaction
static const ImVec4 SliderGrab = ImVec4(0.50f, 0.60f, 0.80f, 1.00f);
static const ImVec4 CheckboxActive = ImVec4(0.30f, 0.50f, 0.80f, 1.00f);

// ═══════════════════════════════════════════════════════════════════════════
// TYPOGRAPHY SCALE (relative sizes from 13px base)
// ═══════════════════════════════════════════════════════════════════════════

static const float FontSizeSmall = 11.0f;
static const float FontSizeBase = 13.0f;
static const float FontSizeMedium = 15.0f;
static const float FontSizeLarge = 18.0f;
static const float FontSizeXL = 22.0f;

// ═══════════════════════════════════════════════════════════════════════════
// ANIMATION TIMING (seconds) - Consistent motion design
// ═══════════════════════════════════════════════════════════════════════════

static const float AnimFast = 0.15f;
static const float AnimNormal = 0.3f;
static const float AnimSlow = 0.5f;

// ═══════════════════════════════════════════════════════════════════════════
// SPACING & ROUNDING - Consistent geometry
// ═══════════════════════════════════════════════════════════════════════════

static const float WindowRounding = 8.0f;
static const float FrameRounding = 4.0f;
static const float GrabRounding = 4.0f;
static const float ItemSpacingX = 8.0f;
static const float ItemSpacingY = 6.0f;

static const float ButtonPaddingX = 12.0f;
static const float ButtonPaddingY = 6.0f;

// ═══════════════════════════════════════════════════════════════════════════
// LIGHT THEME - Alternative color scheme
// ═══════════════════════════════════════════════════════════════════════════

namespace Light {
    static const ImVec4 Background = ImVec4(0.94f, 0.94f, 0.96f, 1.00f);
    static const ImVec4 Surface = ImVec4(1.00f, 1.00f, 1.00f, 0.95f);
    static const ImVec4 TitleBg = ImVec4(0.85f, 0.85f, 0.88f, 1.00f);
    static const ImVec4 TitleBgActive = ImVec4(0.75f, 0.75f, 0.80f, 1.00f);
    static const ImVec4 FrameBg = ImVec4(0.90f, 0.90f, 0.92f, 1.00f);
    static const ImVec4 Text = ImVec4(0.10f, 0.10f, 0.15f, 1.00f);
    static const ImVec4 TextMuted = ImVec4(0.40f, 0.40f, 0.45f, 1.00f);
    
    // Slightly adjusted primaries for light bg
    static const ImVec4 Primary = ImVec4(0.25f, 0.35f, 0.55f, 1.00f);
    static const ImVec4 PrimaryHover = ImVec4(0.30f, 0.40f, 0.60f, 1.00f);
    static const ImVec4 PrimaryActive = ImVec4(0.20f, 0.30f, 0.50f, 1.00f);
}

// ═══════════════════════════════════════════════════════════════════════════
// BODY COLORS (for Rendering and UI)
// ═══════════════════════════════════════════════════════════════════════════

inline std::map<std::string, sf::Color> getBodyColors() {
    return {
        {"Sun", sf::Color::Yellow},
        {"Earth", sf::Color(100, 149, 237)},
        {"Mars", sf::Color(193, 68, 14)},
        {"Jupiter", sf::Color(216, 202, 157)},
        {"Saturn", sf::Color(225, 216, 168)},
        {"Mercury", sf::Color(169, 169, 169)},
        {"Venus", sf::Color(255, 198, 73)},
        {"Uranus", sf::Color(209, 231, 231)},
        {"Neptune", sf::Color(63, 84, 186)},
        {"Moon", sf::Color(169, 169, 169)},
        {"Pluto", sf::Color(205, 186, 172)},
        {"Ceres", sf::Color(155, 155, 155)},
        {"Eris", sf::Color(235, 235, 235)},
        {"Makemake", sf::Color(200, 150, 120)},
        {"Haumea", sf::Color(180, 180, 200)}
    };
}

} // namespace Theme
} // namespace SolarSim

