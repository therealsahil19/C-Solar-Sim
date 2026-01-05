#pragma once

#include <cmath>

namespace SolarSim {
    /**
     * @brief Physical constants for the Solar System simulation.
     * 
     * We use Astronomical Units (AU) for distance, Solar Masses for mass, 
     * and Years for time. This simplifies the gravitational constant G to 4*pi^2.
     */
    namespace Constants {
        // Gravitational Constant G in AU^3 / (Solar Mass * Year^2)
        // G = 4 * pi^2 approx 39.4784176
        const double G = 4.0 * M_PI * M_PI;

        // Conversion factors
        const double AU_METERS = 1.495978707e11;
        const double SOLAR_MASS_KG = 1.98847e30;
        const double YEAR_SECONDS = 31557600.0; // 365.25 days

        // Small softening parameter to prevent numerical instability during close encounters
        const double SOFTENING_EPSILON = 1e-4;

        // Default timestep (1 day in years)
        const double DEFAULT_TIMESTEP = 1.0 / 365.25;

        // Reference masses
        const double EARTH_MASS_SOLAR = 3.00348959632e-6; // Earth mass in solar masses
    }
}
