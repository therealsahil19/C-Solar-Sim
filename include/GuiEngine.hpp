#pragma once

#include "glad.h"

#include <imgui.h>
#include <imgui-SFML.h>
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <cmath>
#include "Body.hpp"
#include "PhysicsEngine.hpp"
#include "StateManager.hpp"
#include "Theme.hpp"

// Forward declare GraphicsEngine to avoid circular dependency
namespace SolarSim { class GraphicsEngine; }

namespace SolarSim {

/**
 * @brief Dear ImGui-based GUI for simulation controls and statistics.
 * 
 * Reorganized into three fixed-position panels:
 * - Time Controls (bottom-left)
 * - Visibility (top-right)
 * - Body Information (middle-right)
 */
class GuiEngine {
public:
    enum class ToastType { Info, Success, Warning, Error };

    struct Toast {
        std::string message;
        ToastType type;
        float duration;
        float elapsed = 0.0f;
    };

    /**
     * @brief Shared state between the GUI and the Main Loop / Physics Engine.
     * 
     * This structure acts as a "Message Bus" where the GUI writes requests (e.g., presetRequest)
     * and reads simulation status (e.g., elapsedYears, fps).
     */
    struct SimulationState {
        bool paused = false;        ///< Is the physics integration halted?
        float timeRate = 1.0f;      ///< Multiplier for delta time (1.0 = Real-time approx)
        int integrator = 2;         ///< Chosen integration method (0=Verlet, 1=RK4, 2=Barnes-Hut)
        bool showTrails = true;     ///< Toggle for orbital path visualization
        bool showLabels = true;     ///< Toggle for body name tags
        bool showAsteroids = true;  ///< Toggle for orbital belt rendering
        bool showPlanetOrbits = true;///< Toggle for main 8 planets
        bool showOtherOrbits = false;///< Toggle for other bodies (Pluto, Moons, etc)
        float elapsedYears = 0.0f;  ///< Relative simulation time in years
        int fps = 0;                ///< Monitored frames per second
        
        // Body selection
        int selectedBody = 0;           ///< Index of the currently focused body
        int lastSelectedBody = -1;      ///< Track previous selection for change detection
        bool cameraFocused = false;     ///< Is camera following a planet?
        bool requestCameraUnfocus = false; ///< Signal to unfocus camera
        int presetRequest = -1;         ///< ID of preset to load (-1 if none)
        bool requestSave = false;       ///< Signal to trigger state export
        bool requestLoad = false;       ///< Signal to trigger state import
        char saveFilename[256] = "simulation_state.csv"; ///< Target filename for save/load

        // Panel Toggle States (WCAG A11y)
        bool showTimeControls = true;
        bool showVisibility = true;
        bool showBodyInfo = true;

        // UI & UX state
        bool showHelp = false;          ///< Toggle for the help modal
        std::vector<Toast> toasts;      ///< Active notification queue
        
        // Theme & Visual Settings
        bool isLoading = false;         ///< Is a long-running process (like preset loading) active?
        float loadingProgress = 0.0f;   ///< Progress percentage [0..1]
    };

    static SimulationState& getState() {
        static SimulationState state;
        return state;
    }

    /**
     * @brief Add a non-blocking toast notification.
     */
    static void addToast(const std::string& message, ToastType type = ToastType::Info, float duration = 3.0f) {
        getState().toasts.push_back({message, type, duration, 0.0f});
    }

    /**
     * @brief Apply theme colors based on dark/light mode.
     */
    static void applyTheme() {
        ImGuiStyle& style = ImGui::GetStyle();
        
        style.Colors[ImGuiCol_WindowBg] = Theme::Surface;
        style.Colors[ImGuiCol_TitleBg] = Theme::TitleBg;
        style.Colors[ImGuiCol_TitleBgActive] = Theme::TitleBgActive;
        style.Colors[ImGuiCol_FrameBg] = Theme::FrameBg;
        style.Colors[ImGuiCol_Button] = Theme::Primary;
        style.Colors[ImGuiCol_ButtonHovered] = Theme::PrimaryHover;
        style.Colors[ImGuiCol_ButtonActive] = Theme::PrimaryActive;
        style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        
        // Common colors
        style.Colors[ImGuiCol_SliderGrab] = Theme::SliderGrab;
        style.Colors[ImGuiCol_CheckMark] = Theme::CheckboxActive;
        
        // A11y: Focus indicators (WCAG 2.1 AA)
        style.Colors[ImGuiCol_NavHighlight] = Theme::FocusRing;
        style.Colors[ImGuiCol_NavWindowingHighlight] = Theme::FocusRing;
    }
    
    /**
     * @brief Initialize ImGui with SFML window.
     */
    static void init(sf::RenderWindow& window) {
        if (!ImGui::SFML::Init(window)) {
            return;
        }
        
        // Apply Palette Unleashed Design System
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = Theme::WindowRounding;
        style.FrameRounding = Theme::FrameRounding;
        style.GrabRounding = Theme::GrabRounding;
        style.ItemSpacing = ImVec2(Theme::ItemSpacingX, Theme::ItemSpacingY);
        style.FramePadding = ImVec2(Theme::ButtonPaddingX, Theme::ButtonPaddingY);
        
        // Enable keyboard navigation
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        
        // Apply initial dark theme
        applyTheme();
    }

    /**
     * @brief Render labels for celestial bodies.
     */
    static void renderLabels(const std::vector<Body>& bodies, const glm::mat4& viewProj,
                            const sf::Vector2u& windowSize) {
        SimulationState& state = getState();
        if (!state.showLabels) return;

        ImDrawList* drawList = ImGui::GetBackgroundDrawList();

        // Find Earth index for Moon positioning
        int earthIndex = -1;
        for (size_t i = 0; i < bodies.size(); ++i) {
            if (bodies[i].name == "Earth") {
                earthIndex = (int)i;
                break;
            }
        }

        for (const auto& body : bodies) {
            if (body.name == "Asteroid") continue;

            glm::vec3 worldPos;

            if (body.name == "Moon" && earthIndex != -1) {
                // Use shared logic from GraphicsEngine
                worldPos = GraphicsEngine::calculateMoonVisualPosition(body, bodies[earthIndex]);
            } else {
                 worldPos = GraphicsEngine::getVisualPosition(body.position, body.name);
            }

            // Project to screen space
            glm::vec4 clipSpace = viewProj * glm::vec4(worldPos, 1.0f);

            // Check if behind camera
            if (clipSpace.w <= 0.0f) continue;

            glm::vec3 ndc = glm::vec3(clipSpace) / clipSpace.w;

            // Convert NDC to screen coordinates
            float x = (ndc.x + 1.0f) * 0.5f * windowSize.x;
            float y = (1.0f - ndc.y) * 0.5f * windowSize.y; // Flip Y

            // Depth occlusion check: read depth buffer at label position
            // If something is closer than the label (depth < label's depth), skip
            float labelDepth = (ndc.z + 1.0f) * 0.5f; // Convert from [-1,1] to [0,1]
            float depthValue = 1.0f;
            
            // Read pixel depth from OpenGL depth buffer
            int px = (int)x;
            int py = (int)(windowSize.y - y); // OpenGL Y is flipped
            if (px >= 0 && px < (int)windowSize.x && py >= 0 && py < (int)windowSize.y) {
                glReadPixels(px, py, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depthValue);
            }
            
            // If the depth buffer has something closer, skip this label
            // Use small epsilon for floating point comparison
            if (depthValue < labelDepth - 0.001f) continue;

            // Adjust label position (above the planet)
            y -= 20.0f;

            // Draw text with shadow for visibility
            std::string label = body.name;
            ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
            ImVec2 textPos(x - textSize.x * 0.5f, y - textSize.y);

            drawList->AddText(ImVec2(textPos.x + 1, textPos.y + 1), IM_COL32(0, 0, 0, 255), label.c_str());
            drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), label.c_str());
        }
    }

    /**
     * @brief Process SFML event for ImGui.
     */
    static void processEvent(const sf::Event& event) {
        ImGui::SFML::ProcessEvent(event);
    }

    /**
     * @brief Update ImGui frame (call each frame).
     */
    static void update(sf::RenderWindow& window, sf::Time deltaTime) {
        ImGui::SFML::Update(window, deltaTime);
        
        // Update toasts
        auto& state = getState();
        float dt = deltaTime.asSeconds();
        for (auto it = state.toasts.begin(); it != state.toasts.end();) {
            it->elapsed += dt;
            if (it->elapsed >= it->duration) {
                it = state.toasts.erase(it);
            } else {
                ++it;
            }
        }
    }

    /**
     * @brief Render Time Controls panel (bottom-left, fixed position).
     */
    static void renderTimeControlsPanel(SimulationState& state) {
        if (!state.showTimeControls) return;
        
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 panelSize(300, 160);
        ImVec2 panelPos(10, viewport->WorkSize.y - panelSize.y - 10);
        
        ImGui::SetNextWindowPos(panelPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(panelSize, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(250, 140), ImVec2(400, 250));
        
        ImGui::Begin("Time Controls", &state.showTimeControls, 
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
        
        // Play/Pause and Reset buttons
        if (ImGui::Button(state.paused ? "  Play  " : " Pause ", ImVec2(80, 30))) {
            state.paused = !state.paused;
            addToast(state.paused ? "Simulation Paused" : "Simulation Resumed", 
                     state.paused ? ToastType::Warning : ToastType::Success);
        }
        ImGui::SetItemTooltip("Toggle simulation playback (Space)");

        ImGui::SameLine();
        if (ImGui::Button("Reset", ImVec2(80, 30))) {
            state.elapsedYears = 0.0f;
            addToast("Time reset to 0", ToastType::Info);
        }
        ImGui::SetItemTooltip("Reset elapsed time to zero");

        ImGui::Spacing();
        ImGui::SetNextItemWidth(-1);
        ImGui::SliderFloat("##TimeRate", &state.timeRate, 0.1f, 100.0f, "Time Rate: %.1fx");
        ImGui::SetItemTooltip("Adjust the speed of time (1.0 = Realtime approx)");

        ImGui::Spacing();
        ImGui::Text("Elapsed: %.2f years", state.elapsedYears);
        
        ImGui::Spacing();
        ImGui::Text("FPS: %d | Bodies: Active", state.fps);

        ImGui::End();
    }

    /**
     * @brief Render Visibility panel (top-right, fixed position).
     */
    static void renderVisibilityPanel(SimulationState& state) {
        if (!state.showVisibility) return;
        
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 panelSize(260, 240);
        ImVec2 panelPos(viewport->WorkSize.x - panelSize.x - 10, 10);
        
        ImGui::SetNextWindowPos(panelPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(panelSize, ImGuiCond_Always);
        
        ImGui::Begin("Visibility", &state.showVisibility,
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
        
        ImGui::Spacing();
        ImGui::Checkbox("Orbital Trails (T)", &state.showTrails);
        ImGui::Checkbox("Orbital Labels", &state.showLabels);
        ImGui::Checkbox("Planet Orbits", &state.showPlanetOrbits);
        ImGui::Checkbox("Other Orbits", &state.showOtherOrbits);
        ImGui::Checkbox("Asteroids", &state.showAsteroids);

        ImGui::End();
    }

    /**
     * @brief Render Body Information panel (middle-right, fixed position).
     */
    static void renderBodyInfoPanel(const std::vector<Body>& bodies, SimulationState& state) {
        if (!state.showBodyInfo) return;
        
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 panelSize(260, 360);
        // Position below visibility panel with gap (10 + 240 + 10)
        ImVec2 panelPos(viewport->WorkSize.x - panelSize.x - 10, 260);
        
        ImGui::SetNextWindowPos(panelPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(panelSize, ImGuiCond_Always);
        
        ImGui::Begin("Body Information", &state.showBodyInfo,
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
        
        // A11y: Keyboard navigation hints
        ImGui::TextDisabled("Use Up/Down arrows to navigate");
        
        if (ImGui::BeginCombo("##SelectBody", 
            state.selectedBody >= 0 && state.selectedBody < (int)bodies.size() 
                ? bodies[state.selectedBody].name.c_str() : "(none)")) {
            
            for (int i = 0; i < (int)bodies.size(); ++i) {
                if (bodies[i].name == "Asteroid") continue;
                
                bool isSelected = (state.selectedBody == i);
                if (ImGui::Selectable(bodies[i].name.c_str(), isSelected)) {
                    state.selectedBody = i;
                }
            }
            ImGui::EndCombo();
        }
        
        // Unfocus button - only show when focused on a planet (not Sun)
        if (state.cameraFocused && state.selectedBody >= 0 && state.selectedBody < (int)bodies.size() 
            && bodies[state.selectedBody].name != "Sun") {
            ImGui::SameLine();
            if (ImGui::Button("Unfocus")) {
                state.requestCameraUnfocus = true;
            }
            ImGui::SetItemTooltip("Stop following planet, camera becomes free");
        }
        
        // A11y: Keyboard body navigation
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive()) {
            int maxBody = -1;
            for (int i = 0; i < (int)bodies.size(); ++i) {
                if (bodies[i].name != "Asteroid") maxBody = i;
            }
            
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
                do {
                    state.selectedBody = std::min(state.selectedBody + 1, maxBody);
                } while (state.selectedBody >= 0 && state.selectedBody < (int)bodies.size() 
                         && bodies[state.selectedBody].name == "Asteroid");
            }
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
                do {
                    state.selectedBody = std::max(state.selectedBody - 1, 0);
                } while (state.selectedBody >= 0 && bodies[state.selectedBody].name == "Asteroid");
            }
        }
        
        ImGui::Separator();
        
        if (state.selectedBody >= 0 && state.selectedBody < (int)bodies.size()) {
            const Body& b = bodies[state.selectedBody];
            
            ImGui::TextColored(Theme::PrimaryHover, "Name: %s", b.name.c_str());
            ImGui::Separator();
            
            ImGui::Text("Mass: %.6e M", b.mass);
            ImGui::Text("Radius: %.6e AU", b.radius);
            
            if (ImGui::CollapsingHeader("Extra")) {
                ImGui::Separator();
                ImGui::Text("Position (AU):");
                ImGui::Text("  X: %.4f", b.position.x);
                ImGui::Text("  Y: %.4f", b.position.y);
                ImGui::Text("  Z: %.4f", b.position.z);
                
                double dist = b.position.length();
                ImGui::Text("Distance: %.4f AU", dist);
                
                ImGui::Separator();
                ImGui::Text("Velocity (AU/y):");
                ImGui::Text("  X: %.4f", b.velocity.x);
                ImGui::Text("  Y: %.4f", b.velocity.y);
                ImGui::Text("  Z: %.4f", b.velocity.z);
                
                double speed = b.velocity.length();
                ImGui::Text("Speed: %.4f AU/y", speed);
                
                if (dist > 0.01) {
                    double T = std::sqrt(dist * dist * dist);
                    ImGui::Text("Orbital Period: %.2f y", T);
                }
                
                ImGui::Separator();
                ImGui::Text("Rotation: %.1f deg", b.rotationAngle);
                ImGui::Text("Tilt: %.1f deg", b.axialTilt);
            }
        } else {
            ImGui::TextWrapped("Select a body from the dropdown to view its properties.");
        }
        
        ImGui::End();
    }

    /**
     * @brief Render all GUI panels.
     */
    static void render(const std::vector<Body>& bodies, float* scalePtr, 
                       float* rotXPtr, float* rotZPtr) {
        (void)scalePtr; // Camera controls removed
        (void)rotXPtr;
        (void)rotZPtr;
        
        SimulationState& state = getState();

        // Render the three fixed panels
        renderTimeControlsPanel(state);
        renderVisibilityPanel(state);
        renderBodyInfoPanel(bodies, state);
        
        // Panel re-open buttons (when panels are closed)
        bool anyPanelClosed = !state.showTimeControls || !state.showVisibility || !state.showBodyInfo;
        if (anyPanelClosed) {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(ImVec2(viewport->WorkSize.x / 2 - 100, 10), ImGuiCond_Always);
            ImGui::Begin("##PanelToggles", nullptr, 
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
            if (!state.showTimeControls && ImGui::Button("Show Time Controls")) state.showTimeControls = true;
            if (!state.showVisibility && ImGui::Button("Show Visibility")) state.showVisibility = true;
            if (!state.showBodyInfo && ImGui::Button("Show Body Info")) state.showBodyInfo = true;
            ImGui::End();
        }
        
        renderHelpPanel(state);
        renderToasts(state);
    }
    
    /**
     * @brief Renders non-blocking toast notifications.
     */
    static void renderToasts(SimulationState& state) {
        if (state.toasts.empty()) return;

        float padding = 10.0f;
        float yOffset = padding;
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        
        for (const auto& toast : state.toasts) {
            ImVec4 color = (toast.type == ToastType::Success) ? Theme::Success :
                          (toast.type == ToastType::Warning) ? Theme::Warning :
                          (toast.type == ToastType::Error)   ? Theme::Error : Theme::Info;

            ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - padding, 
                                           viewport->WorkPos.y + yOffset), 
                                    ImGuiCond_Always, ImVec2(1.0f, 0.0f));
            
            std::string id = "##toast" + std::to_string(&toast - &state.toasts[0]);
            ImGui::Begin(id.c_str(), nullptr, 
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | 
                         ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing | 
                         ImGuiWindowFlags_NoNav);
            
            ImGui::TextColored(color, "●");
            ImGui::SameLine();
            ImGui::Text("%s", toast.message.c_str());
            
            // Progress bar for duration
            float progress = 1.0f - (toast.elapsed / toast.duration);
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
            ImGui::ProgressBar(progress, ImVec2(-1, 2), "");
            ImGui::PopStyleColor();

            yOffset += ImGui::GetWindowHeight() + padding;
            ImGui::End();
        }
    }

    /**
     * @brief Renders the Help & Shortcuts modal.
     */
    static void renderHelpPanel(SimulationState& state) {
        if (!state.showHelp) return;

        ImGui::OpenPopup("Help & Shortcuts");
        if (ImGui::BeginPopupModal("Help & Shortcuts", &state.showHelp, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(Theme::PrimaryHover, "Keyboard Shortcuts:");
            ImGui::Separator();
            ImGui::Columns(2, "helpcolumns", false);
            ImGui::Text("Space"); ImGui::NextColumn(); ImGui::Text("Toggle Pause"); ImGui::NextColumn();
            ImGui::Text("T");     ImGui::NextColumn(); ImGui::Text("Toggle Trails"); ImGui::NextColumn();
            ImGui::Text("H");     ImGui::NextColumn(); ImGui::Text("Toggle Help"); ImGui::NextColumn();
            ImGui::Columns(1);
            
            ImGui::Spacing();
            ImGui::TextColored(Theme::PrimaryHover, "Mouse Controls:");
            ImGui::Separator();
            ImGui::Columns(2, "mousecolumns", false);
            ImGui::Text("Scroll"); ImGui::NextColumn(); ImGui::Text("Zoom In/Out"); ImGui::NextColumn();
            ImGui::Text("Left Drag"); ImGui::NextColumn(); ImGui::Text("Orbit Camera"); ImGui::NextColumn();
            ImGui::Text("Right Drag"); ImGui::NextColumn(); ImGui::Text("Pan View"); ImGui::NextColumn();
            ImGui::Columns(1);
            
            ImGui::Spacing();
            if (ImGui::Button("Close", ImVec2(120, 0))) {
                state.showHelp = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    /**
     * @brief Render ImGui draw data to SFML window.
     */
    static void display(sf::RenderWindow& window) {
        ImGui::SFML::Render(window);
    }

    /**
     * @brief Cleanup ImGui resources.
     */
    static void shutdown() {
        ImGui::SFML::Shutdown();
    }

    /**
     * @brief Console stats output (fallback / legacy).
     */
    static void renderStats(const std::vector<Body>& bodies, double dt) {
        (void)bodies;
        (void)dt;
        // Legacy console output - now handled by ImGui
    }
};

} // namespace SolarSim
