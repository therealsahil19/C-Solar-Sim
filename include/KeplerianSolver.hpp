#pragma once

#include <cmath>
#include <vector>
#include "Vector3.hpp"
#include "Body.hpp"
#include "Constants.hpp"

namespace SolarSim {

/**
 * @brief Keplerian orbital elements structure.
 * These six parameters completely define an elliptical orbit.
 */
struct KeplerianElements {
    double a;      // Semi-major axis (AU)
    double e;      // Eccentricity (0 = circle, <1 = ellipse)
    double i;      // Inclination (degrees)
    double Omega;  // Longitude of ascending node (degrees)
    double omega;  // Argument of periapsis (degrees)
    double M;      // Mean anomaly at epoch (degrees)
};

/**
 * @brief Converts Keplerian orbital elements to Cartesian state vectors.
 * Uses Newton-Raphson iteration to solve Kepler's equation.
 */
class KeplerianSolver {
public:
    static constexpr double DEG_TO_RAD = M_PI / 180.0;
    static constexpr double RAD_TO_DEG = 180.0 / M_PI;
    static constexpr int MAX_ITERATIONS = 100;
    static constexpr double TOLERANCE = 1e-12;

    /**
     * @brief Solves Kepler's equation: M = E - e*sin(E) for eccentric anomaly E.
     * Uses Newton-Raphson iteration.
     * @param M Mean anomaly (radians)
     * @param e Eccentricity
     * @return Eccentric anomaly E (radians)
     */
    static double solveKeplersEquation(double M, double e) {
        // Initial guess
        double E = (e < 0.8) ? M : M_PI;
        
        for (int iter = 0; iter < MAX_ITERATIONS; ++iter) {
            double f = E - e * std::sin(E) - M;       // f(E) = E - e*sin(E) - M
            double fp = 1.0 - e * std::cos(E);        // f'(E) = 1 - e*cos(E)
            double dE = -f / fp;                       // Newton-Raphson step
            E += dE;
            
            if (std::abs(dE) < TOLERANCE) break;
        }
        return E;
    }

    /**
     * @brief Converts eccentric anomaly to true anomaly.
     * @param E Eccentric anomaly (radians)
     * @param e Eccentricity
     * @return True anomaly (radians)
     */
    static double eccentricToTrueAnomaly(double E, double e) {
        double cosE = std::cos(E);
        double sinE = std::sin(E);
        double y = std::sqrt(1.0 - e * e) * sinE;
        double x = cosE - e;
        return std::atan2(y, x);
    }

    /**
     * @brief Converts Keplerian elements to Cartesian position and velocity.
     * @param elements Keplerian orbital elements
     * @param centralMass Mass of central body (solar masses)
     * @return Pair of (position, velocity) in AU and AU/year
     */
    static std::pair<Vector3, Vector3> keplerianToCartesian(
        const KeplerianElements& elements, double centralMass = 1.0) {
        
        double a = elements.a;
        double e = elements.e;
        double i_rad = elements.i * DEG_TO_RAD;
        double Omega_rad = elements.Omega * DEG_TO_RAD;
        double omega_rad = elements.omega * DEG_TO_RAD;
        double M_rad = elements.M * DEG_TO_RAD;

        // Solve Kepler's equation for eccentric anomaly
        double E = solveKeplersEquation(M_rad, e);
        
        // True anomaly
        double nu = eccentricToTrueAnomaly(E, e);

        // Distance from focus
        double r = a * (1.0 - e * std::cos(E));

        // Position in orbital plane (perifocal coordinates)
        double x_orb = r * std::cos(nu);
        double y_orb = r * std::sin(nu);

        // Velocity magnitude factor  
        // v = sqrt(mu * (2/r - 1/a)) where mu = G * M
        double mu = Constants::G * centralMass;
        double h = std::sqrt(mu * a * (1.0 - e * e));  // Specific angular momentum
        
        // Velocity in orbital plane
        double vx_orb = -(mu / h) * std::sin(nu);
        double vy_orb = (mu / h) * (e + std::cos(nu));

        // Rotation matrices components
        double cosO = std::cos(Omega_rad), sinO = std::sin(Omega_rad);
        double cosi = std::cos(i_rad), sini = std::sin(i_rad);
        double cosw = std::cos(omega_rad), sinw = std::sin(omega_rad);

        // Transform to inertial frame (ecliptic J2000)
        // R = R_z(-Omega) * R_x(-i) * R_z(-omega)
        double Px = cosO * cosw - sinO * sinw * cosi;
        double Py = sinO * cosw + cosO * sinw * cosi;
        double Pz = sinw * sini;
        
        double Qx = -cosO * sinw - sinO * cosw * cosi;
        double Qy = -sinO * sinw + cosO * cosw * cosi;
        double Qz = cosw * sini;

        Vector3 position(
            x_orb * Px + y_orb * Qx,
            x_orb * Py + y_orb * Qy,
            x_orb * Pz + y_orb * Qz
        );

        Vector3 velocity(
            vx_orb * Px + vy_orb * Qx,
            vx_orb * Py + vy_orb * Qy,
            vx_orb * Pz + vy_orb * Qz
        );

        return {position, velocity};
    }

    /**
     * @brief Creates a Body from Keplerian elements orbiting the Sun.
     * @param name Body name
     * @param mass Body mass in solar masses
     * @param radius Body radius in AU
     * @param elements Keplerian orbital elements
     * @return Body with position and velocity set from orbital elements
     */
    static Body createBodyFromKeplerian(
        const std::string& name, double mass, double radius,
        const KeplerianElements& elements) {
        
        auto [pos, vel] = keplerianToCartesian(elements, 1.0);
        return Body(name, mass, radius, pos, vel);
    }

    /**
     * @brief Calculates orbital period from semi-major axis (Kepler's 3rd law).
     * @param a Semi-major axis in AU
     * @return Orbital period in years
     */
    static double orbitalPeriod(double a) {
        // T = 2*pi * sqrt(a^3 / (G*M))
        // In our units (AU, years, solar masses): T = sqrt(a^3)
        return std::sqrt(a * a * a);
    }
};

// Pre-defined J2000 Keplerian elements for major bodies (NASA/JPL data)
namespace EphemerisData {
    /** 
     * High-precision J2000 Keplerian elements.
     * Reference: NASA Jet Propulsion Laboratory
     * Epoch: J2000.0 (2000-01-01 12:00 TT)
     */
    const KeplerianElements Mercury = {0.38709927, 0.20563593, 7.00497902, 48.33076593, 77.45779628, 252.25032350};
    const KeplerianElements Venus   = {0.72333566, 0.00677672, 3.39467605, 76.67984255, 131.60246718, 181.97909950};
    const KeplerianElements Earth   = {1.00000261, 0.01671123, -0.00001531, 0.0, 102.93768193, 100.46457166};
    const KeplerianElements Mars    = {1.52371034, 0.09339410, 1.84969142, 49.55953891, 336.04084219, 355.45332854};
    const KeplerianElements Jupiter = {5.20288700, 0.04838624, 1.30439695, 100.47390909, 14.72847983, 34.39644051};
    const KeplerianElements Saturn  = {9.53667594, 0.05386179, 2.48599187, 113.66242448, 92.59887831, 49.95424423};
    const KeplerianElements Uranus  = {19.18916464, 0.04725744, 0.77263783, 74.01692503, 170.95427630, 313.23810451};
    const KeplerianElements Neptune = {30.06992276, 0.00859048, 1.77004347, 131.78422574, 44.96476227, 304.88003086};
    const KeplerianElements Pluto   = {39.48211675, 0.24882730, 17.14001206, 110.30393684, 224.06891629, 238.92903833};

    // Dwarf planets and notable asteroids
    const KeplerianElements Ceres   = {2.7658, 0.0760, 10.59, 80.33, 73.60, 27.19};
    const KeplerianElements Eris    = {67.67, 0.4417, 44.04, 35.95, 151.43, 204.16};
    const KeplerianElements Makemake = {45.79, 0.159, 29.0, 79.3, 298.0, 139.0};
    const KeplerianElements Haumea  = {43.13, 0.195, 28.2, 122.1, 239.5, 205.0};
}

} // namespace SolarSim
