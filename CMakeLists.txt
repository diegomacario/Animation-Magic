cmake_minimum_required(VERSION 3.11.1 FATAL_ERROR)

project(Animation-Magic)

include_directories("${CMAKE_SOURCE_DIR}/inc"
                    "${CMAKE_SOURCE_DIR}/dependencies/cgltf"
                    "${CMAKE_SOURCE_DIR}/dependencies/glad"
                    "${CMAKE_SOURCE_DIR}/dependencies/GLFW"
                    "${CMAKE_SOURCE_DIR}/dependencies/glm"
                    "${CMAKE_SOURCE_DIR}/dependencies/imgui"
                    "${CMAKE_SOURCE_DIR}/dependencies/KHR"
                    "${CMAKE_SOURCE_DIR}/dependencies/stb_image")

set(project_headers
    inc/AnimatedMesh.h
    inc/Blending.h
    inc/Camera3.h
    inc/CCDSolver.h
    inc/Clip.h
    inc/CrossFadeControllerMultiple.h
    inc/CrossFadeTarget.h
    inc/FABRIKSolver.h
    inc/finite_state_machine.h
    inc/Frame.h
    inc/game.h
    inc/GLTFLoader.h
    inc/IKCrossFadeController.h
    inc/IKCrossFadeTarget.h
    inc/IKLeg.h
    inc/IKMovementState.h
    inc/IKState.h
    inc/Interpolation.h
    inc/Intersection.h
    inc/ModelViewerState.h
    inc/MovementState.h
    inc/Pose.h
    inc/quat.h
    inc/Ray.h
    inc/RearrangeBones.h
    inc/resource_manager.h
    inc/shader.h
    inc/shader_loader.h
    inc/Skeleton.h
    inc/SkeletonViewer.h
    inc/SkeletonViewerClipped.h
    inc/Sky.h
    inc/state.h
    inc/texture.h
    inc/texture_loader.h
    inc/Track.h
    inc/TrackVisualizer.h
    inc/Transform.h
    inc/TransformTrack.h
    inc/Triangle.h
    inc/Water.h
    inc/window.h)

set(project_sources
    src/AnimatedMesh.cpp
    src/Blending.cpp
    src/Camera3.cpp
    src/CCDSolver.cpp
    src/Clip.cpp
    src/CrossFadeControllerMultiple.cpp
    src/CrossFadeTarget.cpp
    src/FABRIKSolver.cpp
    src/finite_state_machine.cpp
    src/game.cpp
    src/GLTFLoader.cpp
    src/IKCrossFadeController.cpp
    src/IKCrossFadeTarget.cpp
    src/IKLeg.cpp
    src/IKMovementState.cpp
    src/IKState.cpp
    src/Intersection.cpp
    src/main.cpp
    src/ModelViewerState.cpp
    src/MovementState.cpp
    src/Pose.cpp
    src/quat.cpp
    src/Ray.cpp
    src/RearrangeBones.cpp
    src/shader.cpp
    src/shader_loader.cpp
    src/Skeleton.cpp
    src/SkeletonViewer.cpp
    src/SkeletonViewerClipped.cpp
    src/Sky.cpp
    src/texture.cpp
    src/texture_loader.cpp
    src/Track.cpp
    src/TrackVisualizer.cpp
    src/Transform.cpp
    src/TransformTrack.cpp
    src/Triangle.cpp
    src/Water.cpp
    src/window.cpp
    dependencies/cgltf/cgltf/cgltf.c
    dependencies/imgui/imgui/imgui.cpp
    dependencies/imgui/imgui/imgui_demo.cpp
    dependencies/imgui/imgui/imgui_draw.cpp
    dependencies/imgui/imgui/imgui_impl_glfw.cpp
    dependencies/imgui/imgui/imgui_impl_opengl3.cpp
    dependencies/imgui/imgui/imgui_tables.cpp
    dependencies/imgui/imgui/imgui_widgets.cpp
    dependencies/stb_image/stb_image/stb_image.cpp)

# This path must be relative to the location of the build folder
set(project_resources "../resources@resources")

set(CMAKE_EXECUTABLE_SUFFIX ".html")

# For debugging
#set(CMAKE_CXX_FLAGS "-O3 -s USE_WEBGL2=1 -s FULL_ES3=1 -s USE_GLFW=3 -s WASM=1 -s ASSERTIONS=1 -s ALLOW_MEMORY_GROWTH=1 -o index.html --preload-file ${project_resources} --use-preload-plugins")
# For releasing
set(CMAKE_CXX_FLAGS "-O3 -s USE_WEBGL2=1 -s FULL_ES3=1 -s USE_GLFW=3 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -o index.html --preload-file ${project_resources} --use-preload-plugins")

add_executable(${PROJECT_NAME} ${project_headers} ${project_sources})
