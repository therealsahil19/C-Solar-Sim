# Solar System Simulation

A professional-grade 3D solar system simulation built in C++ with OpenGL, featuring realistic physics, real ephemeris data, and an interactive Dear ImGui interface.

![OpenGL 4.5](https://img.shields.io/badge/OpenGL-4.5-blue) ![C++17](https://img.shields.io/badge/C%2B%2B-17-orange) ![CMake](https://img.shields.io/badge/CMake-3.14+-green)

## Features

### Physics Engine
- **Multiple Integrators**: Velocity Verlet, 4th-order Runge-Kutta (RK4), and Barnes-Hut (O(N log N))
- **Adaptive Timestepping**: Automatically adjusts timestep based on body proximity
- **Collision Detection**: Inelastic merging with conservation of momentum
- **Energy Conservation**: Symplectic integration maintains energy over long timescales

### Graphics
- **3D OpenGL Rendering**: Hardware-accelerated with shaders
- **Phong Lighting**: Realistic lighting with the Sun as light source
- **Textures**: NASA planetary textures for visual realism
- **Orbital Trails**: Fading trails showing orbital paths
- **3D Camera**: Orbit, free-fly, and follow modes

### Data & Accuracy
- **J2000 Ephemeris**: Real NASA/JPL Keplerian orbital elements
- **Keplerian Solver**: Newton-Raphson iteration for Kepler's equation
- **All Major Bodies**: Sun, 8 planets, Moon, dwarf planets (Pluto, Ceres, Eris, Makemake, Haumea)
- **Asteroid Belt**: 200 procedurally placed asteroids

### Interactive GUI (Dear ImGui)
- Time controls (pause, play, time rate adjustment)
- Camera controls with live adjustment
- Visibility toggles (trails, axes, asteroids)
- Integrator selection
- Body information panel with orbital details
- Preset scenarios (Inner Planets, Outer Giants, Earth-Moon, Binary Star)
- Save/Load simulation state

## Building

### Prerequisites
- CMake 3.14+
- C++17 compatible compiler (MSVC, GCC, Clang)
- OpenGL 4.5+ capable graphics card (using Compatibility Profile)
- Linux dependencies (Debian/Ubuntu): `libgl1-mesa-dev`, `libxrandr-dev`, `libxcursor-dev`, `libxi-dev`, `libudev-dev`, `libopenal-dev`, `libvorbis-dev`, `libflac-dev`, `xvfb` (for headless)

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/yourusername/Solar-Sim.git
cd Solar-Sim

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
cmake --build .

# Run
./SolarSim  # or SolarSim.exe on Windows
```

Dependencies (SFML 2.6.1, ImGui 1.89.9, ImGui-SFML v2.6, GLM 0.9.9.8) are automatically fetched via CMake FetchContent.

## Controls

### Keyboard
| Key | Action |
|-----|--------|
| W/S | Pitch camera up/down |
| A/D | Rotate camera left/right |
| Scroll | Zoom in/out |
| Space | Toggle pause (via GUI) |

### GUI Panels
- **Simulation Controls**: Time rate, pause/play, presets, save/load
- **Camera Controls**: Distance, pitch, yaw adjustments
- **Visibility**: Toggle trails, axes, labels, asteroids
- **Body Information**: Select body to view detailed stats
- **Statistics**: FPS, body count, energy drift, integrator info

## Project Structure

```
Solar-Sim/
├── include/           # Header files
│   ├── Body.hpp           # Celestial body class
│   ├── Camera3D.hpp       # 3D camera system
│   ├── ConfigLoader.hpp   # Configuration loading
│   ├── ConsoleRenderer.hpp# Console output (legacy)
│   ├── Constants.hpp      # Physical constants
│   ├── DataExporter.hpp   # CSV export
│   ├── EphemerisLoader.hpp# J2000 data loader
│   ├── GraphicsEngine.hpp # OpenGL rendering
│   ├── GuiEngine.hpp      # ImGui interface
│   ├── KeplerianSolver.hpp# Orbital elements solver
│   ├── Octree.hpp         # Barnes-Hut algorithm
│   ├── PhysicsEngine.hpp  # Physics calculations
│   ├── ShaderProgram.hpp  # Shader management
│   ├── SphereRenderer.hpp # Sphere geometry
│   ├── StateManager.hpp   # Save/load functionality
│   ├── SystemData.hpp     # System initialization
│   ├── Validator.hpp      # Physics validation
│   ├── Vector3.hpp        # 3D vector math
│   └── glad.h             # Custom OpenGL loader header
├── src/
│   ├── main.cpp          # Application entry point
│   └── glad.cpp          # Custom OpenGL loader implementation
├── shaders/
│   ├── planet.vert/frag  # Planet rendering shaders
│   ├── sun.frag          # Sun emission shader
│   └── trail.vert/frag   # Orbital trail shaders
├── textures/             # Planetary textures (add your own)
├── data/
│   └── system.csv        # Body configuration data
└── CMakeLists.txt        # Build configuration
```

## Physics Model

### Units
- **Distance**: Astronomical Units (AU)
- **Time**: Years
- **Mass**: Solar masses

In these units, the gravitational constant G = 4π² ≈ 39.478.

### Integration Methods

| Method | Complexity | Energy Drift | Best For |
|--------|------------|--------------|----------|
| Verlet | O(N²) | Very Low | Long-term stability |
| RK4 | O(N²) | Low | High accuracy |
| Barnes-Hut | O(N log N) | Low | Large N simulations |

## Preset Scenarios

1. **Full Solar System**: All planets, dwarf planets, and 200 asteroids
2. **Inner Planets**: Mercury, Venus, Earth, Mars
3. **Outer Giants**: Jupiter, Saturn, Uranus, Neptune
4. **Earth-Moon System**: Focused Earth-Moon dynamics
5. **Binary Star Test**: Two equal-mass stars in orbit

## License

MIT License - see LICENSE file for details.

## Acknowledgments

- NASA/JPL for ephemeris data
- SFML team for multimedia library
- Dear ImGui for immediate mode GUI
- GLM for mathematics library
