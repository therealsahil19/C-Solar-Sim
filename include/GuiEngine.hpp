#pragma once

#include <imgui.h>
#include <imgui-SFML.h>
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <cmath>
#include "Body.hpp"
#include "PhysicsEngine.hpp"
#include "StateManager.hpp"
#include "HistoryManager.hpp"

namespace SolarSim {

/**
 * @brief Dear ImGui-based GUI for simulation controls and statistics.
 */
class GuiEngine {
public:
    // Simulation state exposed to GUI
    struct SimulationState {
        bool paused = false;
        float timeRate = 1.0f;
        int integrator = 2;  // 0=Verlet, 1=RK4, 2=Barnes-Hut
        bool showTrails = true;
        bool showAxes = true;
        bool showLabels = false;
        bool showAsteroids = true;
        float elapsedYears = 0.0f;
        int fps = 0;
        
        // New Step 19 features
        int selectedBody = -1;          // Index of selected body (-1 = none)
        int presetRequest = -1;         // Request to load preset (-1 = none)
        bool requestSave = false;       // Request to save state
        bool requestLoad = false;       // Request to load state
        char saveFilename[256] = "simulation_state.csv";

        // Time-Travel features
        bool timeTravelActive = false;  // Whether we are currently scrubbing history
        float scrubTime = 0.0f;         // The time we are scrubbing to
        bool requestMarkEpoch = false;  // Request to mark current as epoch
        char epochName[64] = "Meeting of Worlds";
        std::string requestEpochJump = ""; // Name of epoch to jump to
    };

    static SimulationState& getState() {
        static SimulationState state;
        return state;
    }

    /**
     * @brief Initialize ImGui with SFML window.
     */
    static void init(sf::RenderWindow& window) {
        if (!ImGui::SFML::Init(window)) {
            // Log error but we can't easily propagate failure from void function without changing signature
            // In a real app we might throw or return bool
            return;
        }
        
        // Configure style for a professional look
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 8.0f;
        style.FrameRounding = 4.0f;
        style.GrabRounding = 4.0f;
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.15f, 0.9f);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.2f, 1.0f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.2f, 0.2f, 0.3f, 1.0f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.2f, 0.2f, 0.25f, 1.0f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.3f, 0.4f, 0.6f, 1.0f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.4f, 0.5f, 0.7f, 1.0f);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.5f, 0.6f, 0.8f, 1.0f);
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
    }

    /**
     * @brief Render all GUI panels.
     */
    static void render(const std::vector<Body>& bodies, HistoryManager& history, float* scalePtr, 
                       float* rotXPtr, float* rotZPtr) {
        SimulationState& state = getState();

        // Control Panel
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(280, 380), ImGuiCond_FirstUseEver);
        ImGui::Begin("Simulation Controls", nullptr, ImGuiWindowFlags_NoCollapse);

        // Time Controls
        if (ImGui::CollapsingHeader("Time Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::Button(state.paused ? "  Play  " : " Pause ")) {
                state.paused = !state.paused;
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset")) {
                state.elapsedYears = 0.0f;
            }
            ImGui::SliderFloat("Time Rate", &state.timeRate, 0.1f, 100.0f, "%.1fx");
            ImGui::Text("Elapsed: %.2f years", state.elapsedYears);
        }

        // Camera Controls
        if (ImGui::CollapsingHeader("Camera Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (scalePtr) ImGui::SliderFloat("Distance", scalePtr, 1.0f, 500.0f);
            if (rotXPtr) ImGui::SliderFloat("Pitch", rotXPtr, -89.0f, 89.0f);
            if (rotZPtr) ImGui::SliderFloat("Yaw", rotZPtr, -180.0f, 180.0f);
            if (ImGui::Button("Reset Camera")) {
                if (scalePtr) *scalePtr = 80.0f;
                if (rotXPtr) *rotXPtr = 30.0f;
                if (rotZPtr) *rotZPtr = -90.0f;
            }
        }

        // Visibility Toggles
        if (ImGui::CollapsingHeader("Visibility", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Orbital Trails", &state.showTrails);
            ImGui::Checkbox("Coordinate Axes", &state.showAxes);
            ImGui::Checkbox("Body Labels", &state.showLabels);
            ImGui::Checkbox("Asteroids", &state.showAsteroids);
        }

        // Integration Method
        if (ImGui::CollapsingHeader("Integrator")) {
            ImGui::RadioButton("Velocity Verlet", &state.integrator, 0);
            ImGui::RadioButton("Runge-Kutta 4", &state.integrator, 1);
            ImGui::RadioButton("Barnes-Hut (O(N log N))", &state.integrator, 2);
        }
        
        // Preset Scenarios (NEW)
        if (ImGui::CollapsingHeader("Preset Scenarios")) {
            if (ImGui::Button("Full Solar System")) state.presetRequest = 0;
            ImGui::SameLine();
            if (ImGui::Button("Inner Planets")) state.presetRequest = 1;
            if (ImGui::Button("Outer Giants")) state.presetRequest = 2;
            ImGui::SameLine();
            if (ImGui::Button("Earth-Moon")) state.presetRequest = 3;
            if (ImGui::Button("Binary Star Test")) state.presetRequest = 4;
        }
        
        // Save/Load State (NEW)
        if (ImGui::CollapsingHeader("Save/Load")) {
            ImGui::InputText("Filename", state.saveFilename, 256);
            if (ImGui::Button("Save State")) state.requestSave = true;
            ImGui::SameLine();
            if (ImGui::Button("Load State")) state.requestLoad = true;
        }

        // Time-Travel Panel
        if (ImGui::CollapsingHeader("Time-Travel & Epochs", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Time-Travel Mode", &state.timeTravelActive);
            
            if (state.timeTravelActive) {
                ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.8f, 0.4f, 0.4f, 1.0f));
                ImGui::SliderFloat("History Scrub", &state.scrubTime, 0.0f, state.elapsedYears, "Year %.2f");
                ImGui::PopStyleColor();
                ImGui::TextColored(ImVec4(1, 0.5, 0.5, 1), "SIMULATION PAUSED IN HISTORY");
            } else {
                ImGui::Text("Simulating: Active");
            }

            ImGui::Separator();
            ImGui::InputText("Epoch Name", state.epochName, 64);
            if (ImGui::Button("Mark Current Epoch")) {
                state.requestMarkEpoch = true;
            }
        }

        ImGui::End();

        // Statistics Panel
        ImGui::SetNextWindowPos(ImVec2(10, 400), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(280, 150), ImGuiCond_FirstUseEver);
        ImGui::Begin("Statistics", nullptr, ImGuiWindowFlags_NoCollapse);

        ImGui::Text("Bodies: %zu", bodies.size());
        ImGui::Text("FPS: %d", state.fps);
        double energy = PhysicsEngine::calculateTotalEnergy(bodies);
        ImGui::Text("Total Energy: %.6f", energy);
        
        // Energy conservation indicator
        static double initialEnergy = energy;
        static bool first = true;
        if (first) { initialEnergy = energy; first = false; }
        double drift = std::abs((energy - initialEnergy) / initialEnergy) * 100.0;
        ImGui::Text("Energy Drift: %.6f%%", drift);
        
        ImGui::Separator();
        ImGui::Text("Integrator: %s", 
            state.integrator == 0 ? "Verlet" : 
            state.integrator == 1 ? "RK4" : "Barnes-Hut");

        ImGui::End();
        
        // Body Information Panel (NEW)
        renderBodyInfoPanel(bodies, state);

        // Epoch Comparison Panel
        renderEpochPanel(state, history);
    }
    
    /**
     * @brief Renders the Epoch comparison/jump panel.
     */
    static void renderEpochPanel(SimulationState& state, HistoryManager& history) {
        ImGui::SetNextWindowPos(ImVec2(1000, 370), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(270, 180), ImGuiCond_FirstUseEver);
        ImGui::Begin("Epochs", nullptr, ImGuiWindowFlags_NoCollapse);

        ImGui::TextWrapped("Saved Epochs:");
        ImGui::Separator();
        
        auto& epochs = history.getEpochs();
        if (epochs.empty()) {
            ImGui::TextDisabled("(No epochs saved)");
        } else {
            for (auto const& [name, snip] : epochs) {
                if (ImGui::Button(name.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                    state.scrubTime = (float)snip.time;
                    state.timeTravelActive = true;
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Jump to Year %.2f", snip.time);
                }
            }
        }

        ImGui::Separator();
        if (ImGui::Button("Clear History", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            state.elapsedYears = 0;
            state.timeTravelActive = false;
            history.clear();
        }

        ImGui::End();
    }
    
    /**
     * @brief Renders the body information panel.
     */
    static void renderBodyInfoPanel(const std::vector<Body>& bodies, SimulationState& state) {
        ImGui::SetNextWindowPos(ImVec2(1000, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(270, 350), ImGuiCond_FirstUseEver);
        ImGui::Begin("Body Information", nullptr, ImGuiWindowFlags_NoCollapse);
        
        // Body selector combo
        if (ImGui::BeginCombo("Select Body", 
            state.selectedBody >= 0 && state.selectedBody < (int)bodies.size() 
                ? bodies[state.selectedBody].name.c_str() : "(none)")) {
            
            if (ImGui::Selectable("(none)", state.selectedBody == -1)) {
                state.selectedBody = -1;
            }
            for (int i = 0; i < (int)bodies.size(); ++i) {
                // Skip asteroids in the list
                if (bodies[i].name == "Asteroid") continue;
                
                bool isSelected = (state.selectedBody == i);
                if (ImGui::Selectable(bodies[i].name.c_str(), isSelected)) {
                    state.selectedBody = i;
                }
            }
            ImGui::EndCombo();
        }
        
        ImGui::Separator();
        
        // Show info for selected body
        if (state.selectedBody >= 0 && state.selectedBody < (int)bodies.size()) {
            const Body& b = bodies[state.selectedBody];
            
            ImGui::Text("Name: %s", b.name.c_str());
            ImGui::Separator();
            
            ImGui::Text("Mass: %.6e Solar Masses", b.mass);
            ImGui::Text("Radius: %.6e AU", b.radius);
            
            ImGui::Separator();
            ImGui::Text("Position (AU):");
            ImGui::Text("  X: %.4f", b.position.x);
            ImGui::Text("  Y: %.4f", b.position.y);
            ImGui::Text("  Z: %.4f", b.position.z);
            
            double dist = b.position.length();
            ImGui::Text("Distance from Origin: %.4f AU", dist);
            
            ImGui::Separator();
            ImGui::Text("Velocity (AU/year):");
            ImGui::Text("  X: %.4f", b.velocity.x);
            ImGui::Text("  Y: %.4f", b.velocity.y);
            ImGui::Text("  Z: %.4f", b.velocity.z);
            
            double speed = b.velocity.length();
            ImGui::Text("Speed: %.4f AU/year", speed);
            
            // Approximate orbital period (if orbiting Sun)
            if (dist > 0.01) {
                double T = std::sqrt(dist * dist * dist);  // Kepler's 3rd law
                ImGui::Text("Est. Orbital Period: %.2f years", T);
            }
            
            ImGui::Separator();
            ImGui::Text("Rotation: %.1f deg", b.rotationAngle);
            ImGui::Text("Axial Tilt: %.1f deg", b.axialTilt);
        } else {
            ImGui::TextWrapped("Select a body from the dropdown to view its properties.");
        }
        
        ImGui::End();
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
        // Legacy console output - now handled by ImGui
    }
};

} // namespace SolarSim
