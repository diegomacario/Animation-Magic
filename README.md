# Animation-Magic

A tool that reveals the magic behind animations.

You can run it by clicking [this](https://diegomacario.github.io/Animation-Magic) link.

## Technical details

This project was written using C++ and OpenGL. It was then compiled to WebAssembly using [Emscripten](https://emscripten.org). The open source libraries it uses and their purposes are the following:

- [GLFW](https://www.glfw.org/) is used to receive inputs.
- [Dear ImGui](https://github.com/ocornut/imgui) is used for the user interface.
- [GLAD](https://glad.dav1d.de/) is used to load pointers to OpenGL functions.
- [GLM](https://glm.g-truc.net/0.9.9/index.html) is used to do linear algebra.
- [stb_image](https://github.com/nothings/stb) is used to load textures.
- [cgltf](https://github.com/jkuhlmann/cgltf) is used to parse glTF files.

The animated characters were created by [Quaternius](http://quaternius.com/).
