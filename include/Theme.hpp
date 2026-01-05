#pragma once

#include <imgui.h>
#include <SFML/Graphics/Color.hpp>
#include <map>
#include <string>

namespace SolarSim {
namespace Theme {

// Semantic Colors
static const ImVec4 Primary = ImVec4(0.30f, 0.40f, 0.60f, 1.00f);
static const ImVec4 PrimaryHover = ImVec4(0.40f, 0.50f, 0.70f, 1.00f);
static const ImVec4 PrimaryActive = ImVec4(0.20f, 0.30f, 0.50f, 1.00f);

static const ImVec4 Background = ImVec4(0.02f, 0.02f, 0.06f, 1.00f);
static const ImVec4 Surface = ImVec4(0.10f, 0.10f, 0.15f, 0.90f);
static const ImVec4 TitleBg = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
static const ImVec4 TitleBgActive = ImVec4(0.20f, 0.20f, 0.30f, 1.00f);
static const ImVec4 FrameBg = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);

static const ImVec4 Success = ImVec4(0.20f, 0.70f, 0.30f, 1.00f);
static const ImVec4 Warning = ImVec4(0.90f, 0.60f, 0.10f, 1.00f);
static const ImVec4 Error = ImVec4(0.80f, 0.20f, 0.20f, 1.00f);
static const ImVec4 Info = ImVec4(0.20f, 0.60f, 0.90f, 1.00f);

// Interaction
static const ImVec4 SliderGrab = ImVec4(0.50f, 0.60f, 0.80f, 1.00f);
static const ImVec4 CheckboxActive = ImVec4(0.30f, 0.50f, 0.80f, 1.00f);

// Spacing & Rounding
static const float WindowRounding = 8.0f;
static const float FrameRounding = 4.0f;
static const float GrabRounding = 4.0f;
static const float ItemSpacingX = 8.0f;
static const float ItemSpacingY = 6.0f;

// Body Colors (for Rendering and UI)
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
