#ifndef MODEL_VIEWER_STATE_H
#define MODEL_VIEWER_STATE_H

#include "state.h"
#include "finite_state_machine.h"
#include "window.h"
#include "Camera3.h"
#include "texture.h"
#include "AnimatedMesh.h"
#include "SkeletonViewer.h"
#include "Clip.h"
#include "TrackVisualizer.h"

class ModelViewerState : public State
{
public:

   ModelViewerState(const std::shared_ptr<FiniteStateMachine>& finiteStateMachine,
                    const std::shared_ptr<Window>&             window);
   ~ModelViewerState() = default;

   ModelViewerState(const ModelViewerState&) = delete;
   ModelViewerState& operator=(const ModelViewerState&) = delete;

   ModelViewerState(ModelViewerState&&) = delete;
   ModelViewerState& operator=(ModelViewerState&&) = delete;

   void initializeState();

   void enter() override;
   void processInput(float deltaTime) override;
   void update(float deltaTime) override;
   void render() override;
   void exit() override;

private:

   void configureLights(const std::shared_ptr<Shader>& shader);

   void userInterface();

   void resetScene();

   void resetCamera();

   std::shared_ptr<FiniteStateMachine> mFSM;

   std::shared_ptr<Window>             mWindow;

   Camera3                             mCamera3;

   std::vector<AnimatedMesh>           mGroundMeshes;
   std::shared_ptr<Texture>            mGroundTexture;
   std::shared_ptr<Shader>             mGroundShader;

   struct AnimationData
   {
      AnimationData()
         : currentClipIndex(0)
         , playbackTime(0.0f)
      {

      }

      unsigned int           currentClipIndex;

      float                  playbackTime;
      Pose                   animatedPose;
      std::vector<glm::mat4> animatedPosePalette;
      std::vector<glm::mat4> skinMatrices;
      Transform              modelTransform;
   };

   std::shared_ptr<Shader>   mAnimatedMeshShader;
   std::shared_ptr<Texture>  mDiffuseTexture;

   Skeleton                  mSkeleton;
   std::vector<AnimatedMesh> mAnimatedMeshes;
   SkeletonViewer            mSkeletonViewer;
   std::vector<FastClip>     mClips;
   std::string               mClipNames;
   int                       mSelectedClip;
   float                     mSelectedPlaybackSpeed;
   bool                      mDisplayGround;
   bool                      mDisplayGraphs;
   bool                      mDisplayMesh;
   bool                      mDisplayBones;
   bool                      mDisplayJoints;
#ifndef __EMSCRIPTEN__
   bool                      mWireframeModeForCharacter;
   bool                      mWireframeModeForJoints;
   bool                      mPerformDepthTesting;
#endif

   AnimationData             mAnimationData;

   bool                      mPause = false;

   TrackVisualizer           mTrackVisualizer;
};

#endif
