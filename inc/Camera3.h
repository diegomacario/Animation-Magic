#ifndef CAMERA3_H
#define CAMERA3_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "quat.h"

class Camera3
{
public:

   Camera3(float            distanceBetweenPlayerAndCamera,
           float            cameraPitch,
           const glm::vec3& playerPosition,
           const Q::quat&   playerOrientation,
           const glm::vec3& offsetFromPlayerPositionToCameraTarget,
           float            distanceBetweenPlayerAndCameraLowerLimit,
           float            distanceBetweenPlayerAndCameraUpperLimit,
           float            cameraPitchLowerLimit,
           float            cameraPitchUpperLimit,
           float            fieldOfViewYInDeg,
           float            aspectRatio,
           float            near,
           float            far,
           float            mouseSensitivity);
   ~Camera3() = default;

   Camera3(const Camera3&) = default;
   Camera3& operator=(const Camera3&) = default;

   Camera3(Camera3&& rhs) noexcept;
   Camera3& operator=(Camera3&& rhs) noexcept;

   glm::vec3 getPosition();
   Q::quat   getOrientation();
   float     getPitch();

   glm::mat4 getViewMatrix();
   glm::mat4 getPerspectiveProjectionMatrix();
   glm::mat4 getPerspectiveProjectionViewMatrix();

   void      reposition(float            distanceBetweenPlayerAndCamera,
                        float            cameraPitch,
                        const glm::vec3& playerPosition,
                        const Q::quat&   playerOrientation,
                        const glm::vec3& offsetFromPlayerPositionToCameraTarget,
                        float            distanceBetweenPlayerAndCameraLowerLimit,
                        float            distanceBetweenPlayerAndCameraUpperLimit,
                        float            cameraPitchLowerLimit,
                        float            cameraPitchUpperLimit);

   enum class MovementDirection
   {
      Forward,
      Backward,
      Left,
      Right
   };

   void      processMouseMovement(float xOffset, float yOffset);
   void      processScrollWheelMovement(float yOffset);
   void      processPlayerMovement(const glm::vec3& playerPosition, const Q::quat& playerOrientation);

private:

   void      calculateInitialCameraOrientationWRTPlayer();
   void      updatePositionAndOrientationOfCamera();
   void      updateViewMatrix();
   void      updatePerspectiveProjectionMatrix();
   void      updatePerspectiveProjectionViewMatrix();

   glm::vec3 mCameraPosition;
   Q::quat   mCameraOrientationWRTPlayer;
   Q::quat   mCameraGlobalOrientation;

   glm::vec3 mPlayerPosition;
   Q::quat   mPlayerOrientation;
   glm::vec3 mOffsetFromPlayerPositionToCameraTarget;

   float     mDistanceBetweenPlayerAndCamera;
   float     mCameraPitch;

   float     mDistanceBetweenPlayerAndCameraLowerLimit;
   float     mDistanceBetweenPlayerAndCameraUpperLimit;
   float     mCameraPitchLowerLimit;
   float     mCameraPitchUpperLimit;

   float     mFieldOfViewYInDeg;
   float     mAspectRatio;
   float     mNear;
   float     mFar;

   float     mMouseSensitivity;

   glm::mat4 mViewMatrix;
   glm::mat4 mPerspectiveProjectionMatrix;
   glm::mat4 mPerspectiveProjectionViewMatrix;

   bool      mNeedToUpdatePositionAndOrientationOfCamera;
   bool      mNeedToUpdateViewMatrix;
   bool      mNeedToUpdatePerspectiveProjectionMatrix;
   bool      mNeedToUpdatePerspectiveProjectionViewMatrix;
};

#endif
