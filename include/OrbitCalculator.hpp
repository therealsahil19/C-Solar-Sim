#pragma once

#include <vector>
#include <cmath>
#include "Vector3.hpp"
#include "Constants.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace SolarSim {

/**
 * @brief Keplerian orbital elements.
 */
struct OrbitalElements {
    double semiMajorAxis;     // a - semi-major axis (AU)
    double eccentricity;      // e - eccentricity (0 = circle, <1 = ellipse)
    double inclination;       // i - inclination (radians)
    double longitudeAscNode;  // Ω - longitude of ascending node (radians)
    double argPeriapsis;      // ω - argument of periapsis (radians)
    double trueAnomaly;       // ν - true anomaly (radians)
    
    bool isValid = false;     // False if calculation failed (e.g., hyperbolic orbit)
};

/**
 * @brief Calculates orbital elements and generates orbit paths for visualization.
 */
class OrbitCalculator {
public:
    /**
     * @brief Converts state vectors (position, velocity) to Keplerian orbital elements.
     * 
     * @details
     * The conversion follows standard astrodynamics algorithms:
     * 1. **Specific Orbital Energy** ($\epsilon = v^2/2 - \mu/r$): Used to find semi-major axis $a$.
     * 2. **Specific Angular Momentum** ($h = r \times v$): Defines the orbital plane orientation.
     * 3. **Eccentricity Vector** ($e = \frac{v \times h}{\mu} - \frac{r}{|r|}$): Points toward periapsis.
     * 4. **Inclination** ($i = \arccos(h_z / |h|)$): Angle relative to the XY plane.
     * 5. **Longitude of Ascending Node** ($\Omega$): Angle from X-axis to where the orbit crosses XY plane.
     * 6. **Argument of Periapsis** ($\omega$): Angle from node vector to periapsis.
     * 
     * @param pos Position vector relative to central body (AU)
     * @param vel Velocity vector (AU/year)
     * @param mu Gravitational parameter (G * M_central), default uses Sun's mass
     * @return OrbitalElements The calculated orbital elements
     */
    static OrbitalElements calculateElements(const Vector3& pos, const Vector3& vel, 
                                             double mu = Constants::G) {
        OrbitalElements elements;
        
        // Distance and speed
        double r = pos.length();
        double v = vel.length();
        
        if (r < 1e-10 || v < 1e-10) {
            return elements; // Invalid, isValid = false
        }
        
        // Specific orbital energy
        double energy = 0.5 * v * v - mu / r;
        
        // Angular momentum vector h = r × v
        Vector3 h = pos.cross(vel);
        double hMag = h.length();
        
        if (hMag < 1e-10) {
            return elements; // Degenerate orbit
        }
        
        // Eccentricity vector e = (v × h) / μ - r / |r|
        Vector3 eVec = vel.cross(h) / mu - pos / r;
        double e = eVec.length();
        
        // Semi-major axis
        double a;
        if (std::abs(e - 1.0) < 1e-10) {
            // Parabolic orbit
            a = std::numeric_limits<double>::infinity();
            return elements; // Can't visualize parabolic/hyperbolic
        } else if (e > 1.0) {
            // Hyperbolic orbit
            return elements; // isValid = false
        } else {
            a = -mu / (2.0 * energy);
        }
        
        if (a <= 0) {
            return elements; // Invalid (hyperbolic)
        }
        
        // Inclination
        double i = std::acos(std::clamp(h.z / hMag, -1.0, 1.0));
        
        // Node vector n = k × h (k is unit z)
        Vector3 k(0, 0, 1);
        Vector3 n = k.cross(h);
        double nMag = n.length();
        
        // Longitude of ascending node
        double Omega = 0;
        if (nMag > 1e-10) {
            Omega = std::acos(std::clamp(n.x / nMag, -1.0, 1.0));
            if (n.y < 0) Omega = 2.0 * M_PI - Omega;
        }
        
        // Argument of periapsis
        double omega = 0;
        if (nMag > 1e-10 && e > 1e-10) {
            double dotNE = n.dot(eVec) / (nMag * e);
            omega = std::acos(std::clamp(dotNE, -1.0, 1.0));
            if (eVec.z < 0) omega = 2.0 * M_PI - omega;
        }
        
        // True anomaly
        double nu = 0;
        if (e > 1e-10) {
            double dotER = eVec.dot(pos) / (e * r);
            nu = std::acos(std::clamp(dotER, -1.0, 1.0));
            if (pos.dot(vel) < 0) nu = 2.0 * M_PI - nu;
        }
        
        elements.semiMajorAxis = a;
        elements.eccentricity = e;
        elements.inclination = i;
        elements.longitudeAscNode = Omega;
        elements.argPeriapsis = omega;
        elements.trueAnomaly = nu;
        elements.isValid = true;
        
        return elements;
    }
    
    /**
     * @brief Generates points along the orbit ellipse for rendering.
     * 
     * @param orbit Orbital elements
     * @param numPoints Number of points to generate (default 64)
     * @return std::vector<Vector3> Points along the orbit path
     */
    static std::vector<Vector3> generateOrbitPath(const OrbitalElements& orbit, int numPoints = 64) {
        std::vector<Vector3> points;
        
        if (!orbit.isValid || orbit.eccentricity >= 1.0) {
            return points; // Can't visualize non-elliptical orbits
        }
        
        double a = orbit.semiMajorAxis;
        double e = orbit.eccentricity;
        double i = orbit.inclination;
        double Omega = orbit.longitudeAscNode;
        double omega = orbit.argPeriapsis;
        
        // Semi-latus rectum
        double p = a * (1.0 - e * e);
        
        // Generate points around the orbit
        for (int j = 0; j <= numPoints; ++j) {
            double nu = 2.0 * M_PI * j / numPoints; // True anomaly
            
            // Distance from focus
            double r = p / (1.0 + e * std::cos(nu));
            
            // Position in orbital plane (perifocal coordinates)
            double xPeri = r * std::cos(nu);
            double yPeri = r * std::sin(nu);
            
            // Rotation matrices to convert to inertial frame
            // R = R_z(-Ω) * R_x(-i) * R_z(-ω)
            double cosO = std::cos(Omega);
            double sinO = std::sin(Omega);
            double cosI = std::cos(i);
            double sinI = std::sin(i);
            double cosW = std::cos(omega);
            double sinW = std::sin(omega);
            
            // Combined rotation
            double x = (cosO * cosW - sinO * sinW * cosI) * xPeri + 
                      (-cosO * sinW - sinO * cosW * cosI) * yPeri;
            double y = (sinO * cosW + cosO * sinW * cosI) * xPeri + 
                      (-sinO * sinW + cosO * cosW * cosI) * yPeri;
            double z = (sinW * sinI) * xPeri + (cosW * sinI) * yPeri;
            
            points.emplace_back(x, y, z);
        }
        
        return points;
    }
};

} // namespace SolarSim
