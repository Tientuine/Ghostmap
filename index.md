# GhostMap

Simulation and visualization of disease spread based on stochastic epidemiological models.

- [API Documentation](./docs/api/)

## Build Dependencies

Visual Studio 2022 project files are provided for building the Ghostmap program.

This application relies on three external libraries for the visualization feature: GLFW, GLEW, and freeglut. These libraries are not included in this repository.

In addition, the Visual Studio project configuration assumes that the following environment variables are correctly definedi:

- `GLFW_DIR` - root directory of a local GLFW installation
- `GLEW_DIR` - root directory of a local GLEW installation
- `GLUT_DIR` - root directory of a local freeglut installation

