# Contributing to Solar System Simulation ✍️

First off, thank you for considering contributing to SolarSim! It's people like you that make the simulation better for everyone.

## Code of Conduct
This project and everyone participating in it is governed by our Code of Conduct. By participating, you are expected to uphold this code.

## How Can I Contribute?

### Reporting Bugs
- Use a clear and descriptive title.
- Describe the exact steps which reproduce the problem.
- Explain which behavior you expected to see and why.

### Suggesting Enhancements
- Explain why this enhancement would be useful.
- Provide step-by-step descriptions/mockups if possible.

### Pull Requests
1. Fork the repo and create your branch from `main`.
2. If you've added code that should be tested, add tests to `tests/verify_features.cpp`.
3. If you've changed APIs, update the documentation in the headers.
4. Ensure the test suite passes (see below).
5. Make sure your code follows the existing style (we use Doxygen for comments).
6. Log your changes in `.jules/scribe.md` if applicable.

## Verification Suite

Before submitting a PR, you MUST run the verification suite to ensure no regressions in physics or stability.

```bash
# Build the verification executable
cmake -B build -D BUILD_TESTS=ON
cmake --build build --target verify

# Run the tests
./build/verify           # Linux/macOS
./build/Debug/verify.exe # Windows
```

The suite checks:
- **Energy Conservation**: Ensures total mechanical energy drift is within acceptable bounds.
- **Momentum**: Verifies that total system momentum is conserved during integration.
- **State Persistence**: Checks that save/load cycles are bit-perfect.
- **Integrator Stability**: Validates orbital periods for known configurations.

## Documentation Standards
As **Scribe** says: "If it isn't documented, it doesn't exist."

### The Scribe Checklist:
- **Language**: Technical but accessible.
- **Doxygen**: Required for all public headers.
- **Formulae**: If an algorithm is from a paper or textbook, cite it or explain the math.
- **Why > What**: Don't just say `// i++`. Say `// Increment to next octant`.

## Thinking in SolarSim 🌌

When adding a new feature, keep these internal standards in mind:

### 1. The Barycentric Frame
Everything in the simulation is calculated relative to the Solar System Barycenter (SSB). If you add a body, ensure you re-calculate the system's center of mass and momentum.

### 2. Space is Huge, Sim is Small
We use **Double Precision** (`double`) for all physics but **Single Precision** (`float`) for most graphics. Be mindful of casting and precision loss during the Bridge (Physics -> Graphics).

### 3. Coordinate System
- **Physics**: standard $X, Y, Z$.
- **Graphics (OpenGL)**: $Y$ is UP. Our bridge biasanya maps Physics $Z$ to Graphics $Y$.

## Development Setup
Check the `README.md` for build instructions. We recommend using Visual Studio 2022 on Windows or GCC/Clang on Linux.
