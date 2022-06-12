#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "resource_manager.h"
#include "shader_loader.h"
#include "texture_loader.h"
#include "GLTFLoader.h"
#include "RearrangeBones.h"
#include "ModelViewerState.h"

ModelViewerState::ModelViewerState(const std::shared_ptr<FiniteStateMachine>& finiteStateMachine,
                                   const std::shared_ptr<Window>&             window)
   : mFSM(finiteStateMachine)
   , mWindow(window)
   , mCamera3(7.5f, 25.0f, glm::vec3(0.0f), Q::quat(), glm::vec3(0.0f, 2.5f, 0.0f), 2.0f, 20.0f, 0.0f, 90.0f, 45.0f, 1280.0f / 720.0f, 0.1f, 130.0f, 0.25f)
{
   // Initialize the animated mesh shader
   mAnimatedMeshShader = ResourceManager<Shader>().loadUnmanagedResource<ShaderLoader>("resources/shaders/animated_mesh_with_pregenerated_skin_matrices.vert",
                                                                                       "resources/shaders/diffuse_illumination.frag");
   configureLights(mAnimatedMeshShader);

   // Initialize the ground shader
   mGroundShader = ResourceManager<Shader>().loadUnmanagedResource<ShaderLoader>("resources/shaders/static_mesh.vert",
                                                                                 "resources/shaders/ambient_diffuse_illumination.frag");
   configureLights(mGroundShader);

   loadCharacters();
   loadGround();
}

void ModelViewerState::initializeState()
{
   mPause = false;

   // Set the initial character
   mSelectedCharacter = 4;     // Leela
   mCurrentCharacterIndex = 4; // Leela

   // Set the initial clip
   mSelectedClip = 0;         // Leela  - Dance
   mCurrentClipIndex = { 7,   // Woman  - Walk
                         2,   // Man    - Run
                         0,   // Stag   - Idle
                         1,   // George - Hello
                         0,   // Leela  - Dance
                         5,   // Zombie - Walk
                         0 }; // Pistol - Fire

   // Set the initial playback time
   mPlaybackTime = 0.0f;
   // Set the initial playback speed
   mSelectedPlaybackSpeed = 1.0f;
   // Set the initial rendering options
   mDisplayGround = false;
   mDisplayGraphs = true;
   mDisplayMesh = true;
   mDisplayBones = true;
   mDisplayJoints = true;
#ifndef __EMSCRIPTEN__
   mWireframeModeForCharacter = false;
   mWireframeModeForJoints = false;
   mPerformDepthTesting = false;
#endif
   mFillEmptyTilesWithRepeatedGraphs = true;

   mCharacterSkeleton = mCharacterBaseSkeletons[mCurrentCharacterIndex];

   // Set the initial pose
   mPose = mCharacterSkeleton.GetRestPose();

   // Set the model transform
   mModelTransform = { Transform(glm::vec3(0.0f, 0.0f, 0.0f), Q::quat(), glm::vec3(1.0f)),    // Woman
                       Transform(glm::vec3(0.0f, 0.0f, 0.0f), Q::quat(), glm::vec3(0.975f)),  // Man
                       Transform(glm::vec3(0.0f, 0.0f, 0.0f), Q::quat(), glm::vec3(0.95f)),   // Stag
                       Transform(glm::vec3(0.0f, 0.0f, 0.0f), Q::quat(), glm::vec3(0.8f)),    // George
                       Transform(glm::vec3(0.0f, 0.0f, 0.0f), Q::quat(), glm::vec3(0.8f)),    // Leela
                       Transform(glm::vec3(0.0f, 0.0f, 0.0f), Q::quat(), glm::vec3(0.7f)),    // Zombie
                       Transform(glm::vec3(0.2f, 2.45f, 0.0f), Q::quat(), glm::vec3(0.5f)) }; // Pistol

   mJointScaleFactors = { 7.5f,   // Woman
                          0.1f,   // Man
                          0.1f,   // Stag
                          0.1f,   // George
                          0.1f,   // Leela
                          0.1f,   // Zombie
                          0.3f }; // Pistol

   // Initialize the bones of the skeleton viewer
   mSkeletonViewer.InitializeBones(mPose);

   // Sample the clip to get the animated pose
   FastClip& currClip = mCharacterClips[mCurrentCharacterIndex][mCurrentClipIndex[mCurrentCharacterIndex]];
   mPlaybackTime = currClip.Sample(mPose, mPlaybackTime);

   // Get the palette of the animated pose
   mPose.GetMatrixPalette(mPosePalette);

   std::vector<glm::mat4>& inverseBindPose = mCharacterSkeleton.GetInvBindPose();

   // Generate the skin matrices
   mSkinMatrices.resize(mPosePalette.size());
   for (unsigned int i = 0,
        size = static_cast<unsigned int>(mPosePalette.size());
        i < size;
        ++i)
   {
      mSkinMatrices[i] = mPosePalette[i] * inverseBindPose[i];
   }

   // Update the skeleton viewer
   mSkeletonViewer.UpdateBones(mPose, mPosePalette);

   // Reset the track visualizer
   mTrackVisualizer.setTracks(mCharacterClips[mCurrentCharacterIndex][mCurrentClipIndex[mCurrentCharacterIndex]].GetTransformTracks());
}

void ModelViewerState::enter()
{
   initializeState();
   resetCamera();
   resetScene();
}

void ModelViewerState::processInput()
{
   // Close the game
   if (mWindow->keyIsPressed(GLFW_KEY_ESCAPE)) { mWindow->setShouldClose(true); }

#ifndef __EMSCRIPTEN__
   // Make the game full screen or windowed
   if (mWindow->keyIsPressed(GLFW_KEY_F) && !mWindow->keyHasBeenProcessed(GLFW_KEY_F))
   {
      mWindow->setKeyAsProcessed(GLFW_KEY_F);
      mWindow->setFullScreen(!mWindow->isFullScreen());
   }

   // Change the number of samples used for anti aliasing
   if (mWindow->keyIsPressed(GLFW_KEY_1) && !mWindow->keyHasBeenProcessed(GLFW_KEY_1))
   {
      mWindow->setKeyAsProcessed(GLFW_KEY_1);
      mWindow->setNumberOfSamples(1);
   }
   else if (mWindow->keyIsPressed(GLFW_KEY_2) && !mWindow->keyHasBeenProcessed(GLFW_KEY_2))
   {
      mWindow->setKeyAsProcessed(GLFW_KEY_2);
      mWindow->setNumberOfSamples(2);
   }
   else if (mWindow->keyIsPressed(GLFW_KEY_4) && !mWindow->keyHasBeenProcessed(GLFW_KEY_4))
   {
      mWindow->setKeyAsProcessed(GLFW_KEY_4);
      mWindow->setNumberOfSamples(4);
   }
   else if (mWindow->keyIsPressed(GLFW_KEY_8) && !mWindow->keyHasBeenProcessed(GLFW_KEY_8))
   {
      mWindow->setKeyAsProcessed(GLFW_KEY_8);
      mWindow->setNumberOfSamples(8);
   }
#endif

   // Reset the camera
   if (mWindow->keyIsPressed(GLFW_KEY_R)) { resetCamera(); }

   // Orient the camera
   if (mWindow->mouseMoved() && mWindow->isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
   {
      mCamera3.processMouseMovement(mWindow->getCursorXOffset(), mWindow->getCursorYOffset());
      mWindow->resetMouseMoved();
   }

   // Adjust the distance between the player and the camera
   if (mWindow->scrollWheelMoved())
   {
      mCamera3.processScrollWheelMovement(mWindow->getScrollYOffset());
      mWindow->resetScrollWheelMoved();
   }

   if (mWindow->keyIsPressed(GLFW_KEY_P) && !mWindow->keyHasBeenProcessed(GLFW_KEY_P))
   {
      mWindow->setKeyAsProcessed(GLFW_KEY_P);
      mPause = !mPause;
   }
}

void ModelViewerState::update(float deltaTime)
{
   if (mPause)
   {
      return;
   }

   if (mCurrentCharacterIndex != mSelectedCharacter)
   {
      mCurrentCharacterIndex = mSelectedCharacter;
      mCharacterSkeleton = mCharacterBaseSkeletons[mCurrentCharacterIndex];
      mPose = mCharacterSkeleton.GetRestPose();
      mPlaybackTime = 0.0f;

      mSelectedClip = mCurrentClipIndex[mCurrentCharacterIndex];

      // Reset the skeleton viewer
      mSkeletonViewer.InitializeBones(mPose);

      // Reset the track visualizer
      mTrackVisualizer.setTracks(mCharacterClips[mCurrentCharacterIndex][mCurrentClipIndex[mCurrentCharacterIndex]].GetTransformTracks());
   }

   if (mCurrentClipIndex[mCurrentCharacterIndex] != mSelectedClip)
   {
      mCurrentClipIndex[mCurrentCharacterIndex] = mSelectedClip;
      mPose = mCharacterSkeleton.GetRestPose();
      mPlaybackTime = 0.0f;

      // Reset the track visualizer
      mTrackVisualizer.setTracks(mCharacterClips[mCurrentCharacterIndex][mCurrentClipIndex[mCurrentCharacterIndex]].GetTransformTracks());
   }

   // Sample the clip to get the animated pose
   FastClip& currClip = mCharacterClips[mCurrentCharacterIndex][mCurrentClipIndex[mCurrentCharacterIndex]];
   mPlaybackTime = currClip.Sample(mPose, mPlaybackTime + (deltaTime * mSelectedPlaybackSpeed));

   // Get the palette of the animated pose
   mPose.GetMatrixPalette(mPosePalette);

   std::vector<glm::mat4>& inverseBindPose = mCharacterSkeleton.GetInvBindPose();

   // Generate the skin matrices
   mSkinMatrices.resize(mPosePalette.size());
   for (unsigned int i = 0,
        size = static_cast<unsigned int>(mPosePalette.size());
        i < size;
        ++i)
   {
      mSkinMatrices[i] = mPosePalette[i] * inverseBindPose[i];
   }

   // Update the skeleton viewer
   mSkeletonViewer.UpdateBones(mPose, mPosePalette);

   // Update the track visualizer
   mTrackVisualizer.update(deltaTime, mSelectedPlaybackSpeed, mWindow, mFillEmptyTilesWithRepeatedGraphs);
}

void ModelViewerState::render()
{
   ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplGlfw_NewFrame();
   ImGui::NewFrame();

   userInterface();

#ifndef __EMSCRIPTEN__
   mWindow->bindMultisampleFramebuffer();
#endif
   // Set the clear color to black
   glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
   // Clear the current framebuffer
   // At this point the entire framebuffer is black
   glClear(GL_COLOR_BUFFER_BIT);

   // Set the clear color to red
   glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
   // Clear the area within the scissor box, that is, the viewport
   glEnable(GL_SCISSOR_TEST);
   glClear(GL_COLOR_BUFFER_BIT);
   glDisable(GL_SCISSOR_TEST);

   // Clear the depth buffer
   glClear(GL_DEPTH_BUFFER_BIT);

   // Enable depth testing for 3D objects
   glEnable(GL_DEPTH_TEST);

   // Render the graphs
   if (mDisplayGraphs)
   {
      mTrackVisualizer.render(mFillEmptyTilesWithRepeatedGraphs);
   }

   glClear(GL_DEPTH_BUFFER_BIT);

   if (mDisplayGround)
   {
      mGroundShader->use(true);

      glm::mat4 modelMatrix(1.0f);
      modelMatrix = glm::scale(modelMatrix, glm::vec3(0.10f));
      mGroundShader->setUniformMat4("model",      modelMatrix);
      mGroundShader->setUniformMat4("view",       mCamera3.getViewMatrix());
      mGroundShader->setUniformMat4("projection", mCamera3.getPerspectiveProjectionMatrix());
      mGroundTexture->bind(0, mGroundShader->getUniformLocation("diffuseTex"));

      // Loop over the ground meshes and render each one
      for (unsigned int i = 0,
         size = static_cast<unsigned int>(mGroundMeshes.size());
         i < size;
         ++i)
      {
         mGroundMeshes[i].Render();
      }

      mGroundTexture->unbind(0);
      mGroundShader->use(false);
   }

#ifndef __EMSCRIPTEN__
   if (mWireframeModeForCharacter)
   {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   }
#endif

   // Render the animated meshes
   if (mDisplayMesh)
   {
      mAnimatedMeshShader->use(true);
      mAnimatedMeshShader->setUniformMat4("model",      transformToMat4(mModelTransform[mCurrentCharacterIndex]));
      mAnimatedMeshShader->setUniformMat4("view",       mCamera3.getViewMatrix());
      mAnimatedMeshShader->setUniformMat4("projection", mCamera3.getPerspectiveProjectionMatrix());
      mAnimatedMeshShader->setUniformMat4Array("animated[0]", mSkinMatrices);
      mCharacterTextures[mCurrentCharacterIndex]->bind(0, mAnimatedMeshShader->getUniformLocation("diffuseTex"));

      // Loop over the meshes and render each one
      for (unsigned int i = 0,
           size = static_cast<unsigned int>(mCharacterMeshes[mCurrentCharacterIndex].size());
           i < size;
           ++i)
      {
         mCharacterMeshes[mCurrentCharacterIndex][i].Render();
      }

      mCharacterTextures[mCurrentCharacterIndex]->unbind(0);
      mAnimatedMeshShader->use(false);
   }

#ifdef __EMSCRIPTEN__
   glDisable(GL_DEPTH_TEST);
#else
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

   if (!mPerformDepthTesting)
   {
      glDisable(GL_DEPTH_TEST);
   }
#endif

   // Render the bones
   if (mDisplayBones)
   {
      mSkeletonViewer.RenderBones(mModelTransform[mCurrentCharacterIndex], mCamera3.getPerspectiveProjectionViewMatrix());
   }

#ifndef __EMSCRIPTEN__
   if (mWireframeModeForJoints)
   {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   }
#endif

   // Render the joints
   if (mDisplayJoints)
   {
      int indexOfGlowingJoint = -1;
      int indexOfSelectedGraph = mTrackVisualizer.getIndexOfSelectedGraph();
      if (indexOfSelectedGraph != -1)
      {
         indexOfGlowingJoint = mCharacterClips[mCurrentCharacterIndex][mCurrentClipIndex[mCurrentCharacterIndex]].GetJointIDOfTransformTrack(indexOfSelectedGraph);
      }

      mSkeletonViewer.RenderJoints(mModelTransform[mCurrentCharacterIndex], mCamera3.getPerspectiveProjectionViewMatrix(), mPosePalette, mJointScaleFactors[mCurrentCharacterIndex], indexOfGlowingJoint);
   }

#ifndef __EMSCRIPTEN__
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
   glEnable(GL_DEPTH_TEST);

   ImGui::Render();
   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

#ifndef __EMSCRIPTEN__
   mWindow->generateAntiAliasedImage();
#endif

   mWindow->swapBuffers();
   mWindow->pollEvents();
}

void ModelViewerState::exit()
{

}

void ModelViewerState::loadCharacters()
{
   std::vector<std::string> characterTextureFilePaths { "resources/models/woman/woman.png",
                                                        "resources/models/man/man.png",
                                                        //"resources/models/animals/alpaca.png",
                                                        //"resources/models/animals/deer.png",
                                                        //"resources/models/animals/fox.png",
                                                        //"resources/models/animals/horse.png",
                                                        //"resources/models/animals/husky.png",
                                                        "resources/models/animals/stag.png",
                                                        //"resources/models/animals/wolf.png",
                                                        "resources/models/mechs/george.png",
                                                        "resources/models/mechs/leela.png",
                                                        "resources/models/zombie/zombie.png",
                                                        "resources/models/pistol/pistol.png" };

   std::vector<std::string> characterModelFilePaths { "resources/models/woman/woman.glb",
                                                      "resources/models/man/man.glb",
                                                      //"resources/models/animals/alpaca.glb",
                                                      //"resources/models/animals/deer.glb",
                                                      //"resources/models/animals/fox.glb",
                                                      //"resources/models/animals/horse.glb",
                                                      //"resources/models/animals/husky.glb",
                                                      "resources/models/animals/stag.glb",
                                                      //"resources/models/animals/wolf.glb",
                                                      "resources/models/mechs/george.glb",
                                                      "resources/models/mechs/leela.glb",
                                                      "resources/models/zombie/zombie.glb",
                                                      "resources/models/pistol/pistol.glb" };

   std::vector<std::string> characterNames { "Woman", "Man", "Stag", "George", "Leela", "Zombie", "Pistol" };
   for (const std::string& characterName : characterNames)
   {
      mCharacterNames += characterName + '\0';
   }

   // Load the textures of the animated characters
   mCharacterTextures.reserve(characterTextureFilePaths.size());
   for (const std::string& characterTextureFilePath : characterTextureFilePaths)
   {
      mCharacterTextures.emplace_back(ResourceManager<Texture>().loadUnmanagedResource<TextureLoader>(characterTextureFilePath));
   }

   // Load the animated characters
   size_t numModels = characterModelFilePaths.size();
   mCharacterBaseSkeletons.reserve(numModels);
   mCharacterMeshes.reserve(numModels);
   mCharacterClips.reserve(numModels);
   unsigned int modelIndex = 0;
   for (const std::string& characterModelFilePath : characterModelFilePaths)
   {
      // Load the animated character
      cgltf_data* data = LoadGLTFFile(characterModelFilePath.c_str());
      mCharacterBaseSkeletons.emplace_back(LoadSkeleton(data));
      mCharacterMeshes.emplace_back(LoadAnimatedMeshes(data));
      std::vector<Clip> characterClips = LoadClips(data);
      FreeGLTFFile(data);

      // Rearrange the skeleton
      JointMap characterJointMap = RearrangeSkeleton(mCharacterBaseSkeletons[modelIndex]);

      // Rearrange the meshes
      for (unsigned int meshIndex = 0,
           numMeshes = static_cast<unsigned int>(mCharacterMeshes[modelIndex].size());
           meshIndex < numMeshes;
           ++meshIndex)
      {
         RearrangeMesh(mCharacterMeshes[modelIndex][meshIndex], characterJointMap);
         mCharacterMeshes[modelIndex][meshIndex].ClearMeshData();
      }

      // Optimize the clips, rearrange them and store them
      mCharacterClips.emplace_back(std::vector<FastClip>());
      std::string characterClipNames;
      for (unsigned int clipIndex = 0,
           numClips = static_cast<unsigned int>(characterClips.size());
           clipIndex < numClips;
           ++clipIndex)
      {
         FastClip currClip = OptimizeClip(characterClips[clipIndex]);
         RearrangeFastClip(currClip, characterJointMap);
         mCharacterClips[modelIndex].push_back(currClip);
         characterClipNames += currClip.GetName() + '\0';
      }
      mCharacterClipNames.push_back(characterClipNames);

      // Configure the VAOs of the animated meshes
      int positionsAttribLocOfAnimatedShader  = mAnimatedMeshShader->getAttributeLocation("position");
      int normalsAttribLocOfAnimatedShader    = mAnimatedMeshShader->getAttributeLocation("normal");
      int texCoordsAttribLocOfAnimatedShader  = mAnimatedMeshShader->getAttributeLocation("texCoord");
      int weightsAttribLocOfAnimatedShader    = mAnimatedMeshShader->getAttributeLocation("weights");
      int influencesAttribLocOfAnimatedShader = mAnimatedMeshShader->getAttributeLocation("joints");

      for (unsigned int i = 0,
           size = static_cast<unsigned int>(mCharacterMeshes[modelIndex].size());
           i < size;
           ++i)
      {
         mCharacterMeshes[modelIndex][i].ConfigureVAO(positionsAttribLocOfAnimatedShader,
                                                      normalsAttribLocOfAnimatedShader,
                                                      texCoordsAttribLocOfAnimatedShader,
                                                      weightsAttribLocOfAnimatedShader,
                                                      influencesAttribLocOfAnimatedShader);
      }

      ++modelIndex;
   }
}

void ModelViewerState::loadGround()
{
   // Load the texture of the ground
   mGroundTexture = ResourceManager<Texture>().loadUnmanagedResource<TextureLoader>("resources/models/table/wooden_floor.jpg");

   // Load the ground
   cgltf_data* data = LoadGLTFFile("resources/models/table/wooden_floor.glb");
   mGroundMeshes = LoadStaticMeshes(data);
   FreeGLTFFile(data);

   int positionsAttribLocOfStaticShader = mGroundShader->getAttributeLocation("position");
   int normalsAttribLocOfStaticShader   = mGroundShader->getAttributeLocation("normal");
   int texCoordsAttribLocOfStaticShader = mGroundShader->getAttributeLocation("texCoord");
   for (unsigned int i = 0,
        size = static_cast<unsigned int>(mGroundMeshes.size());
        i < size;
        ++i)
   {
      mGroundMeshes[i].ConfigureVAO(positionsAttribLocOfStaticShader,
                                    normalsAttribLocOfStaticShader,
                                    texCoordsAttribLocOfStaticShader,
                                    -1,
                                    -1);
   }
}

void ModelViewerState::configureLights(const std::shared_ptr<Shader>& shader)
{
   shader->use(true);
   shader->setUniformVec3("pointLights[0].worldPos", glm::vec3(0.0f, 2.0f, 10.0f));
   shader->setUniformVec3("pointLights[0].color", glm::vec3(1.0f, 0.95f, 0.9f));
   shader->setUniformFloat("pointLights[0].constantAtt", 1.0f);
   shader->setUniformFloat("pointLights[0].linearAtt", 0.01f);
   shader->setUniformFloat("pointLights[0].quadraticAtt", 0.0f);
   shader->setUniformVec3("pointLights[1].worldPos", glm::vec3(0.0f, 2.0f, -10.0f));
   shader->setUniformVec3("pointLights[1].color", glm::vec3(1.0f, 0.95f, 0.9f));
   shader->setUniformFloat("pointLights[1].constantAtt", 1.0f);
   shader->setUniformFloat("pointLights[1].linearAtt", 0.01f);
   shader->setUniformFloat("pointLights[1].quadraticAtt", 0.0f);
   shader->setUniformInt("numPointLightsInScene", 2);
   shader->use(false);
}

void ModelViewerState::userInterface()
{
   ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Appearing);

   ImGui::Begin("Model Viewer", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

   ImGui::Text("Application Average: %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

   if (ImGui::CollapsingHeader("Controls", nullptr))
   {
      ImGui::BulletText("Hold the left mouse button and move the mouse\n"
                        "to rotate the camera around the character.");
      ImGui::BulletText("Use the scroll wheel to zoom in and out.");
      ImGui::BulletText("Press the P key to pause the animation.");
      ImGui::BulletText("Press the R key to reset the camera.");
   }

   if (ImGui::CollapsingHeader("Settings", nullptr))
   {
      ImGui::Combo("Character", &mSelectedCharacter, mCharacterNames.c_str());

      ImGui::Combo("Clip", &mSelectedClip, mCharacterClipNames[mCurrentCharacterIndex].c_str());

      ImGui::SliderFloat("Playback Speed", &mSelectedPlaybackSpeed, 0.0f, 2.0f, "%.3f");

      float durationOfCurrClip = mCharacterClips[mCurrentCharacterIndex][mCurrentClipIndex[mCurrentCharacterIndex]].GetDuration();
      char progress[32];
      snprintf(progress, 32, "%.3f / %.3f", mPlaybackTime, durationOfCurrClip);
      ImGui::ProgressBar(mPlaybackTime / durationOfCurrClip, ImVec2(0.0f, 0.0f), progress);

      ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
      ImGui::Text("Playback Time");

      ImGui::Checkbox("Display Ground", &mDisplayGround);

      ImGui::Checkbox("Display Graphs", &mDisplayGraphs);

      ImGui::Checkbox("Display Skin", &mDisplayMesh);

      ImGui::Checkbox("Display Bones", &mDisplayBones);

      ImGui::Checkbox("Display Joints", &mDisplayJoints);

#ifndef __EMSCRIPTEN__
      ImGui::Checkbox("Wireframe Mode for Skin", &mWireframeModeForCharacter);

      ImGui::Checkbox("Wireframe Mode for Joints", &mWireframeModeForJoints);

      ImGui::Checkbox("Perform Depth Testing", &mPerformDepthTesting);
#endif

      ImGui::Checkbox("Fill Empty Tiles With Repeated Graphs", &mFillEmptyTilesWithRepeatedGraphs);
   }

   ImGui::End();
}

void ModelViewerState::resetScene()
{

}

void ModelViewerState::resetCamera()
{
   mCamera3.reposition(7.5f, 25.0f, glm::vec3(0.0f), Q::quat(), glm::vec3(0.0f, 2.5f, 0.0f), 2.0f, 20.0f, 0.0f, 90.0f);
   mCamera3.processMouseMovement(180.0f / 0.25f, 0.0f);
}
