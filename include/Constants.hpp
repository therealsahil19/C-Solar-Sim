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
        /**
         * @brief Gravitational Constant G.
         * 
         * In our chosen unit system (AU, Years, Solar Masses), the gravitational constant 
         * simplifies to 4 * pi^2. This is because the period of Earth's orbit is 
         * defined as 1 year and its distance as 1 AU.
         * 
         * Formula: G = 4 * pi^2 * (a^3 / (M * T^2))
         */
        const double G = 4.0 * M_PI * M_PI;

        // Conversion factors for metric/standard unit compatibility
        const double AU_METERS = 1.495978707e11;    ///< 1 Astronomical Unit in meters
        const double SOLAR_MASS_KG = 1.98847e30;    ///< 1 Solar Mass in kilograms
        const double YEAR_SECONDS = 31557600.0;    ///< 1 Julian Year (365.25 days) in seconds

        /**
         * @brief Small softening parameter to prevent numerical instability.
         * 
         * During close encounters, the 1/r^2 force can approach infinity. Softening 
         * adds a small epsilon to the distance to bound the force. 
         * We use 1e-9 (approx 150 meters) to allow stable moon orbits while 
         * preventing NaN results.
         */
        const double SOFTENING_EPSILON = 1e-9;

        // Default timestep (1 day in years)
        const double DEFAULT_TIMESTEP = 1.0 / 365.25;

        // Reference masses
        const double EARTH_MASS_SOLAR = 3.00348959632e-6; // Earth mass in solar masses
    }
}
