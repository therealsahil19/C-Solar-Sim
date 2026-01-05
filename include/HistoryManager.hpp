#pragma once

#include <vector>
#include <deque>
#include <map>
#include <string>
#include <algorithm>
#include "Vector3.hpp"
#include "Body.hpp"

namespace SolarSim {

/**
 * @brief Represents the state of a single body at a point in time.
 */
struct BodyState {
    Vector3 position;
    Vector3 velocity;
    double rotationAngle;
};

/**
 * @brief A snapshot of the entire system state at a specific simulation time.
 */
struct Snapshot {
    double time;
    std::vector<BodyState> bodyStates;
};

/**
 * @brief Manages simulation history and named epochs for comparison.
 */
class HistoryManager {
public:
    static constexpr size_t DEFAULT_MAX_SNAPSHOTS = 5000;
    
    HistoryManager(size_t maxSnapshots = DEFAULT_MAX_SNAPSHOTS) 
        : maxSnapshots(maxSnapshots) {}

    /**
     * @brief Record current system state.
     */
    void record(double time, const std::vector<Body>& bodies) {
        // Minimal recording interval (e.g., don't record if time hasn't changed much)
        if (!history.empty() && (time - history.back().time) < 0.001) return;

        Snapshot snip;
        snip.time = time;
        snip.bodyStates.reserve(bodies.size());
        for (const auto& body : bodies) {
            snip.bodyStates.push_back({body.position, body.velocity, body.rotationAngle});
        }
        
        history.push_back(snip);
        if (history.size() > maxSnapshots) {
            history.pop_front();
        }
    }

    /**
     * @brief Retrieve interpolated state at a specific time.
     */
    bool getStateAt(double time, std::vector<Body>& bodies) const {
        if (history.empty()) return false;
        if (bodies.size() != history.front().bodyStates.size()) return false;

        // Find the bounding snapshots
        auto it = std::lower_bound(history.begin(), history.end(), time, 
            [](const Snapshot& s, double t) { return s.time < t; });

        if (it == history.begin()) {
            applySnapshot(history.front(), bodies);
            return true;
        }
        if (it == history.end()) {
            applySnapshot(history.back(), bodies);
            return true;
        }

        const Snapshot& s2 = *it;
        const Snapshot& s1 = *std::prev(it);

        double den = s2.time - s1.time;
        double t = (den > 1e-9) ? (time - s1.time) / den : 0.0;
        
        for (size_t i = 0; i < bodies.size(); ++i) {
            bodies[i].position = s1.bodyStates[i].position * (1.0 - t) + s2.bodyStates[i].position * t;
            bodies[i].velocity = s1.bodyStates[i].velocity * (1.0 - t) + s2.bodyStates[i].velocity * t;
            bodies[i].rotationAngle = s1.bodyStates[i].rotationAngle * (1.0 - t) + s2.bodyStates[i].rotationAngle * t;
            // Clear trail to avoid visual glitches when jumping time
            bodies[i].trail.clear();
        }

        return true;
    }

    void markEpoch(const std::string& name, double time, const std::vector<Body>& bodies) {
        Snapshot snip;
        snip.time = time;
        for (const auto& body : bodies) {
            snip.bodyStates.push_back({body.position, body.velocity, body.rotationAngle});
        }
        epochs[name] = snip;
    }

    const std::map<std::string, Snapshot>& getEpochs() const { return epochs; }
    
    double getStartTime() const { return history.empty() ? 0 : history.front().time; }
    double getEndTime() const { return history.empty() ? 0 : history.back().time; }
    size_t getHistorySize() const { return history.size(); }

    void clear() {
        history.clear();
        epochs.clear();
    }

    /**
     * @brief Truncate history after a certain time (useful if resuming from past).
     */
    void truncateAfter(double time) {
        while (!history.empty() && history.back().time > time) {
            history.pop_back();
        }
    }

private:
    void applySnapshot(const Snapshot& snip, std::vector<Body>& bodies) const {
        for (size_t i = 0; i < bodies.size(); ++i) {
            bodies[i].position = snip.bodyStates[i].position;
            bodies[i].velocity = snip.bodyStates[i].velocity;
            bodies[i].rotationAngle = snip.bodyStates[i].rotationAngle;
            bodies[i].trail.clear();
        }
    }

    std::deque<Snapshot> history;
    std::map<std::string, Snapshot> epochs;
    size_t maxSnapshots;
};

} // namespace SolarSim
