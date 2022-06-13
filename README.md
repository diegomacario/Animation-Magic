# Animation-Magic

A cool visualization of all the math that powers 3D character animations.

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

## What do all the little graphs in the background represent?

To answer that question, let's quickly look at how 3D character animations work.

An animated character consists of two things:

- A mesh.
- A skeleton.

The mesh is what gets rendered on the screen. It's just a bunch of triangles.

The skeleton is the interesting part. It consists of a set of joints (joints are represented as little green pyramids in this project). Each joint is a transform matrix, which means that it has:

- A position, which is stored as a vector with **X**, **Y** and **Z** components.
- An orientation, which is stored as a quaternion with **X**, **Y**, **Z** and **W** components.
- A scale, which is stored as a vector with **X**, **Y** and **Z** components.

The joints (transform matrices) that make up the skeleton can be used to deform the mesh into a specific pose.

Now imagine if the position, orientation and scale of each joint changed with time. If that was the case, each time we deformed the mesh using the skeleton we would get a different pose. And what is a sequence of poses? Well an animation, of course! It's as simple as that.

Now going back to the original question, the little graphs in the background of this project show how the orientation of each joint changes with time. There's one graph per joint. Remember that the orientation of each joint is stored as a quaternion, which has **X**, **Y**, **Z** and **W** components. That's why each graph has 4 curves in it:

- The **red** one corresponds to the **X** value.
- The **orange** one corresponds to the **Y** value.
- The **yellow** one corresponds to the **Z** value.
- The **green** one corresponds to the **W** value.

So if we are rendering 60 frames per second, we are basically doing the following 60 times a second:

1. Sample all the curves you see on the graphs to get the orientation of each joint. Also sample the position and scale curves, which I don't display in the graphs, to get the position and scale of each joint.
2. Create all the joints (transform matrices) that make up the skeleton using the position, orientation and scale values that we sampled in the previous step.
3. Deform the mesh using the skeleton.
4. Render the frame.

The character that's labelled as **Robot 1** in this project has 49 joints. Each joint has 3 position curves (a vector), 4 orientation curves (a quaternion) and 3 scale curves (a vector). That means that for every frame we are sampling `49 * (3 + 4 + 3) = 490` curves. That's a wild number of calculations, just to make a single character move.

Modern games can have dozens of characters on the screen at the same time, and each one can have thousands of joints. And they don't just sample curves, they also modify them in real-time using techniques like inverse kinematics so that characters look good when they walk on uneven ground, for example. It's truly incredible what modern animation systems are capable of doing. I hope that this project helps you appreciate that.

## Implementation details

You might be wondering why I don't include the position and scale curves of each joint in the graphs. I have a few reasons:

- Displaying 10 curves (2 vectors and 1 quaternion) in each graph makes them look even more cluttered than they already do.
- It turns out that most character animations are composed by rotating joints, not by moving or scaling them. There are exceptions to this, however. As an example, take a look at the **Reload** animation of the **Pistol** model that's included in this project. No graphs are shown in the background for that animation because none of the joints are rotating. They are only moving. If I plotted the positions of the joints instead of their orientations, we would see some nice graphs.

Also note that the speed at which the graphs move from right to left doesn't match the speed at which they are sampled. In my initial implementation I used the true sampling speed and the graphs moved dizzyingly fast, so I slowed them down considerably.
