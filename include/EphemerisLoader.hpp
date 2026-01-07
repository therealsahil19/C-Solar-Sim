#pragma once

#include <vector>
#include <string>
#include "Body.hpp"
#include "KeplerianSolver.hpp"

namespace SolarSim {

/**
 * @brief Loads solar system bodies using real J2000 ephemeris data.
 * Creates bodies from Keplerian orbital elements for accurate positions.
 */
class EphemerisLoader {
public:
    /**
     * @brief Planet physical properties (mass in solar masses, radius in AU).
     */
    struct PlanetData {
        std::string name;
        double mass;       // Solar masses
        double radius;     // AU
        double rotSpeed;   // Degrees per day
        double tilt;       // Axial tilt degrees
    };

    /**
     * @brief Loads the complete solar system with J2000 ephemeris data.
     * @return Vector of bodies with accurate initial positions and velocities.
     */
    static std::vector<Body> loadSolarSystemJ2000() {
        std::vector<Body> system;

        // Sun at origin (will be adjusted for barycenter later)
        Body sun("Sun", 1.0, 0.00465, Vector3(0, 0, 0), Vector3(0, 0, 0));
        sun.rotationSpeed = 13.0;
        sun.axialTilt = 7.25;
        system.push_back(sun);

        // Planet data: name, mass (solar), radius (AU), rotSpeed (deg/day), tilt (deg)
        const std::vector<PlanetData> planets = {
            {"Mercury", 1.6601e-7,  1.63e-5,  6.0,    0.03},
            {"Venus",   2.4478e-6,  4.04e-5,  -1.4,   177.3},  // Retrograde
            {"Earth",   3.0034e-6,  4.26e-5,  360.0,  23.44},
            {"Mars",    3.2271e-7,  2.26e-5,  350.0,  25.19},
            {"Jupiter", 9.5479e-4,  0.00047,  870.0,  3.13},
            {"Saturn",  2.8588e-4,  0.00040,  810.0,  26.73},
            {"Uranus",  4.3662e-5,  0.00017,  -500.0, 97.77},  // Retrograde
            {"Neptune", 5.1513e-5,  0.00016,  530.0,  28.32}
        };

        // Keplerian elements from EphemerisData namespace
        const KeplerianElements* elements[] = {
            &EphemerisData::Mercury,
            &EphemerisData::Venus,
            &EphemerisData::Earth,
            &EphemerisData::Mars,
            &EphemerisData::Jupiter,
            &EphemerisData::Saturn,
            &EphemerisData::Uranus,
            &EphemerisData::Neptune
        };

        // Create planets from Keplerian elements
        for (size_t i = 0; i < planets.size(); ++i) {
            Body body = KeplerianSolver::createBodyFromKeplerian(
                planets[i].name, planets[i].mass, planets[i].radius, *elements[i]);
            body.rotationSpeed = planets[i].rotSpeed;
            body.axialTilt = planets[i].tilt;
            system.push_back(body);
        }

        // Add the Moon orbiting Earth
        addMoon(system);

        // Add moons of outer planets
        addJupiterMoons(system);
        addSaturnMoons(system);
        addNeptuneMoons(system);

        // Add Pluto and dwarf planets
        addDwarfPlanets(system);

        return system;
    }

private:
    /**
     * @brief Adds Earth's Moon with realistic orbital elements.
     */
    static void addMoon(std::vector<Body>& system) {
        // Find Earth's position
        Vector3 earthPos(0,0,0), earthVel(0,0,0);
        for (const auto& body : system) {
            if (body.name == "Earth") {
                earthPos = body.position;
                earthVel = body.velocity;
                break;
            }
        }

        // Moon orbital elements (relative to Earth)
        // Semi-major axis: 384,400 km = 0.00257 AU
        // Orbital period: 27.3 days
        KeplerianElements moonElements = {
            0.00257,    // a (AU)
            0.0549,     // e (eccentricity)
            5.145,      // i (inclination to ecliptic, degrees)
            125.08,     // Omega (longitude of ascending node)
            318.15,     // omega (argument of periapsis)
            135.27      // M (mean anomaly at J2000)
        };

        // Get Moon's position relative to Earth
        auto [relPos, relVel] = KeplerianSolver::keplerianToCartesian(moonElements, 3.0034e-6);

        Body moon("Moon", 3.694e-8, 1.16e-5, earthPos + relPos, earthVel + relVel);
        moon.rotationSpeed = 13.2;  // Tidally locked, same as orbital period
        moon.axialTilt = 6.68;
        moon.parentName = "Earth";  // Moon orbits Earth, not the Sun
        system.push_back(moon);
    }

    /**
     * @brief Adds Pluto and other dwarf planets.
     */
    static void addDwarfPlanets(std::vector<Body>& system) {
        // Pluto
        Body pluto = KeplerianSolver::createBodyFromKeplerian(
            "Pluto", 6.58e-9, 7.93e-6, EphemerisData::Pluto);
        pluto.rotationSpeed = -56.4;  // Retrograde rotation
        pluto.axialTilt = 122.53;
        system.push_back(pluto);

        // Ceres (in asteroid belt)
        Body ceres = KeplerianSolver::createBodyFromKeplerian(
            "Ceres", 4.7e-10, 3.15e-6, EphemerisData::Ceres);
        ceres.rotationSpeed = 952.0;
        ceres.axialTilt = 4.0;
        system.push_back(ceres);

        // Eris (distant dwarf planet)
        Body eris = KeplerianSolver::createBodyFromKeplerian(
            "Eris", 8.27e-9, 7.77e-6, EphemerisData::Eris);
        eris.rotationSpeed = 14.0;
        eris.axialTilt = 78.0;
        system.push_back(eris);

        // Makemake
        Body makemake = KeplerianSolver::createBodyFromKeplerian(
            "Makemake", 1.5e-9, 4.77e-6, EphemerisData::Makemake);
        makemake.rotationSpeed = 38.0;
        makemake.axialTilt = 0.0;
        system.push_back(makemake);

        // Haumea (elongated shape, fast rotation)
        Body haumea = KeplerianSolver::createBodyFromKeplerian(
            "Haumea", 2.0e-9, 3.34e-6, EphemerisData::Haumea);
        haumea.rotationSpeed = 929.0;  // Very fast (3.9 hour period)
        haumea.axialTilt = 0.0;
        system.push_back(haumea);
    }

    /**
     * @brief Adds Jupiter's four Galilean moons.
     * Io, Europa, Ganymede, Callisto - the largest moons of Jupiter.
     */
    static void addJupiterMoons(std::vector<Body>& system) {
        // Find Jupiter's position
        Vector3 jupiterPos(0,0,0), jupiterVel(0,0,0);
        double jupiterMass = 9.5479e-4;
        for (const auto& body : system) {
            if (body.name == "Jupiter") {
                jupiterPos = body.position;
                jupiterVel = body.velocity;
                break;
            }
        }

        // Io: 421,700 km = 0.00282 AU, diameter 3640 km
        KeplerianElements ioElements = {
            0.00282, 0.0041, 0.036, 43.977, 84.129, 342.021
        };
        auto [ioPos, ioVel] = KeplerianSolver::keplerianToCartesian(ioElements, jupiterMass);
        Body io("Io", 4.5e-8, 1.22e-5, jupiterPos + ioPos, jupiterVel + ioVel);
        io.rotationSpeed = 203.0;  // Tidally locked
        io.parentName = "Jupiter";
        system.push_back(io);

        // Europa: 670,900 km = 0.00449 AU, diameter 3130 km
        KeplerianElements europaElements = {
            0.00449, 0.009, 0.466, 219.106, 88.970, 171.016
        };
        auto [europaPos, europaVel] = KeplerianSolver::keplerianToCartesian(europaElements, jupiterMass);
        Body europa("Europa", 2.4e-8, 1.05e-5, jupiterPos + europaPos, jupiterVel + europaVel);
        europa.rotationSpeed = 101.0;  // Tidally locked
        europa.parentName = "Jupiter";
        system.push_back(europa);

        // Ganymede: 1,070,000 km = 0.00715 AU, diameter 5270 km (largest moon)
        KeplerianElements ganymedeElements = {
            0.00715, 0.0013, 0.177, 63.552, 192.417, 317.540
        };
        auto [ganymedePos, ganymedeVel] = KeplerianSolver::keplerianToCartesian(ganymedeElements, jupiterMass);
        Body ganymede("Ganymede", 7.4e-8, 1.76e-5, jupiterPos + ganymedePos, jupiterVel + ganymedeVel);
        ganymede.rotationSpeed = 50.0;  // Tidally locked
        ganymede.parentName = "Jupiter";
        system.push_back(ganymede);

        // Callisto: 1,883,000 km = 0.01259 AU, diameter 4821 km
        KeplerianElements callistoElements = {
            0.01259, 0.0074, 0.192, 298.848, 52.643, 181.408
        };
        auto [callistoPos, callistoVel] = KeplerianSolver::keplerianToCartesian(callistoElements, jupiterMass);
        Body callisto("Callisto", 5.4e-8, 1.61e-5, jupiterPos + callistoPos, jupiterVel + callistoVel);
        callisto.rotationSpeed = 21.0;  // Tidally locked
        callisto.parentName = "Jupiter";
        system.push_back(callisto);
    }

    /**
     * @brief Adds Saturn's largest moon, Titan.
     */
    static void addSaturnMoons(std::vector<Body>& system) {
        // Find Saturn's position
        Vector3 saturnPos(0,0,0), saturnVel(0,0,0);
        double saturnMass = 2.8588e-4;
        for (const auto& body : system) {
            if (body.name == "Saturn") {
                saturnPos = body.position;
                saturnVel = body.velocity;
                break;
            }
        }

        // Titan: 1,221,850 km = 0.00817 AU, diameter 5150 km
        KeplerianElements titanElements = {
            0.00817, 0.0288, 0.34854, 169.237, 180.532, 163.310
        };
        auto [titanPos, titanVel] = KeplerianSolver::keplerianToCartesian(titanElements, saturnMass);
        Body titan("Titan", 6.75e-8, 1.72e-5, saturnPos + titanPos, saturnVel + titanVel);
        titan.rotationSpeed = 22.6;  // Tidally locked
        titan.parentName = "Saturn";
        system.push_back(titan);
    }

    /**
     * @brief Adds Neptune's largest moon, Triton.
     */
    static void addNeptuneMoons(std::vector<Body>& system) {
        // Find Neptune's position
        Vector3 neptunePos(0,0,0), neptuneVel(0,0,0);
        double neptuneMass = 5.1513e-5;
        for (const auto& body : system) {
            if (body.name == "Neptune") {
                neptunePos = body.position;
                neptuneVel = body.velocity;
                break;
            }
        }

        // Triton: 354,800 km = 0.00237 AU, diameter 2710 km
        // Note: Triton has a retrograde orbit (inclination ~157 degrees)
        KeplerianElements tritonElements = {
            0.00237, 0.000016, 156.865, 177.608, 66.142, 352.257
        };
        auto [tritonPos, tritonVel] = KeplerianSolver::keplerianToCartesian(tritonElements, neptuneMass);
        Body triton("Triton", 1.08e-8, 9.05e-6, neptunePos + tritonPos, neptuneVel + tritonVel);
        triton.rotationSpeed = -61.3;  // Retrograde, tidally locked
        triton.parentName = "Neptune";
        system.push_back(triton);
    }
};

} // namespace SolarSim
