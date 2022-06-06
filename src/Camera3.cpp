#include <utility>

#include "Camera3.h"

Camera3::Camera3(float            distanceBetweenPlayerAndCamera,
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
                 float            mouseSensitivity)
   : mCameraPosition()
   , mCameraOrientationWRTPlayer()
   , mCameraGlobalOrientation()
   , mPlayerPosition(playerPosition + offsetFromPlayerPositionToCameraTarget)
   , mPlayerOrientation(playerOrientation)
   , mOffsetFromPlayerPositionToCameraTarget(offsetFromPlayerPositionToCameraTarget)
   , mDistanceBetweenPlayerAndCamera(distanceBetweenPlayerAndCamera)
   , mCameraPitch(cameraPitch)
   , mDistanceBetweenPlayerAndCameraLowerLimit(distanceBetweenPlayerAndCameraLowerLimit)
   , mDistanceBetweenPlayerAndCameraUpperLimit(distanceBetweenPlayerAndCameraUpperLimit)
   , mCameraPitchLowerLimit(cameraPitchLowerLimit)
   , mCameraPitchUpperLimit(cameraPitchUpperLimit)
   , mFieldOfViewYInDeg(fieldOfViewYInDeg)
   , mAspectRatio(aspectRatio)
   , mNear(near)
   , mFar(far)
   , mMouseSensitivity(mouseSensitivity)
   , mViewMatrix()
   , mPerspectiveProjectionMatrix()
   , mPerspectiveProjectionViewMatrix()
   , mNeedToUpdatePositionAndOrientationOfCamera(true)
   , mNeedToUpdateViewMatrix(true)
   , mNeedToUpdatePerspectiveProjectionMatrix(true)
   , mNeedToUpdatePerspectiveProjectionViewMatrix(true)
{
   calculateInitialCameraOrientationWRTPlayer();
}

Camera3::Camera3(Camera3&& rhs) noexcept
   : mCameraPosition(std::exchange(rhs.mCameraPosition, glm::vec3(0.0f)))
   , mCameraOrientationWRTPlayer(std::exchange(rhs.mCameraOrientationWRTPlayer, Q::quat()))
   , mCameraGlobalOrientation(std::exchange(rhs.mCameraGlobalOrientation, Q::quat()))
   , mPlayerPosition(std::exchange(rhs.mPlayerPosition, glm::vec3(0.0f)))
   , mPlayerOrientation(std::exchange(rhs.mPlayerOrientation, Q::quat()))
   , mOffsetFromPlayerPositionToCameraTarget(std::exchange(rhs.mOffsetFromPlayerPositionToCameraTarget, glm::vec3(0.0f)))
   , mDistanceBetweenPlayerAndCamera(std::exchange(rhs.mDistanceBetweenPlayerAndCamera, 0.0f))
   , mCameraPitch(std::exchange(rhs.mCameraPitch, 0.0f))
   , mDistanceBetweenPlayerAndCameraLowerLimit(std::exchange(rhs.mDistanceBetweenPlayerAndCameraLowerLimit, 0.0f))
   , mDistanceBetweenPlayerAndCameraUpperLimit(std::exchange(rhs.mDistanceBetweenPlayerAndCameraUpperLimit, 0.0f))
   , mCameraPitchLowerLimit(std::exchange(rhs.mCameraPitchLowerLimit, 0.0f))
   , mCameraPitchUpperLimit(std::exchange(rhs.mCameraPitchUpperLimit, 0.0f))
   , mFieldOfViewYInDeg(std::exchange(rhs.mFieldOfViewYInDeg, 0.0f))
   , mAspectRatio(std::exchange(rhs.mAspectRatio, 0.0f))
   , mNear(std::exchange(rhs.mNear, 0.0f))
   , mFar(std::exchange(rhs.mFar, 0.0f))
   , mMouseSensitivity(std::exchange(rhs.mMouseSensitivity, 0.0f))
   , mViewMatrix(std::exchange(rhs.mViewMatrix, glm::mat4(0.0f)))
   , mPerspectiveProjectionMatrix(std::exchange(rhs.mPerspectiveProjectionMatrix, glm::mat4(0.0f)))
   , mPerspectiveProjectionViewMatrix(std::exchange(rhs.mPerspectiveProjectionViewMatrix, glm::mat4(0.0f)))
   , mNeedToUpdatePositionAndOrientationOfCamera(std::exchange(rhs.mNeedToUpdatePositionAndOrientationOfCamera, true))
   , mNeedToUpdateViewMatrix(std::exchange(rhs.mNeedToUpdateViewMatrix, true))
   , mNeedToUpdatePerspectiveProjectionMatrix(std::exchange(rhs.mNeedToUpdatePerspectiveProjectionMatrix, true))
   , mNeedToUpdatePerspectiveProjectionViewMatrix(std::exchange(rhs.mNeedToUpdatePerspectiveProjectionViewMatrix, true))
{

}

Camera3& Camera3::operator=(Camera3&& rhs) noexcept
{
   mCameraPosition                              = std::exchange(rhs.mCameraPosition, glm::vec3(0.0f));
   mCameraOrientationWRTPlayer                  = std::exchange(rhs.mCameraOrientationWRTPlayer, Q::quat());
   mCameraGlobalOrientation                     = std::exchange(rhs.mCameraGlobalOrientation, Q::quat());
   mPlayerPosition                              = std::exchange(rhs.mPlayerPosition, glm::vec3(0.0f));
   mPlayerOrientation                           = std::exchange(rhs.mPlayerOrientation, Q::quat());
   mOffsetFromPlayerPositionToCameraTarget      = std::exchange(rhs.mOffsetFromPlayerPositionToCameraTarget, glm::vec3(0.0f));
   mDistanceBetweenPlayerAndCamera              = std::exchange(rhs.mDistanceBetweenPlayerAndCamera, 0.0f);
   mCameraPitch                                 = std::exchange(rhs.mCameraPitch, 0.0f);
   mDistanceBetweenPlayerAndCameraLowerLimit    = std::exchange(rhs.mDistanceBetweenPlayerAndCameraLowerLimit, 0.0f);
   mDistanceBetweenPlayerAndCameraUpperLimit    = std::exchange(rhs.mDistanceBetweenPlayerAndCameraUpperLimit, 0.0f);
   mCameraPitchLowerLimit                       = std::exchange(rhs.mCameraPitchLowerLimit, 0.0f);
   mCameraPitchUpperLimit                       = std::exchange(rhs.mCameraPitchUpperLimit, 0.0f);
   mFieldOfViewYInDeg                           = std::exchange(rhs.mFieldOfViewYInDeg, 0.0f);
   mAspectRatio                                 = std::exchange(rhs.mAspectRatio, 0.0f);
   mNear                                        = std::exchange(rhs.mNear, 0.0f);
   mFar                                         = std::exchange(rhs.mFar, 0.0f);
   mMouseSensitivity                            = std::exchange(rhs.mMouseSensitivity, 0.0f);
   mViewMatrix                                  = std::exchange(rhs.mViewMatrix, glm::mat4(0.0f));
   mPerspectiveProjectionMatrix                 = std::exchange(rhs.mPerspectiveProjectionMatrix, glm::mat4(0.0f));
   mPerspectiveProjectionViewMatrix             = std::exchange(rhs.mPerspectiveProjectionViewMatrix, glm::mat4(0.0f));
   mNeedToUpdatePositionAndOrientationOfCamera  = std::exchange(rhs.mNeedToUpdatePositionAndOrientationOfCamera, true);
   mNeedToUpdateViewMatrix                      = std::exchange(rhs.mNeedToUpdateViewMatrix, true);
   mNeedToUpdatePerspectiveProjectionMatrix     = std::exchange(rhs.mNeedToUpdatePerspectiveProjectionMatrix, true);
   mNeedToUpdatePerspectiveProjectionViewMatrix = std::exchange(rhs.mNeedToUpdatePerspectiveProjectionViewMatrix, true);
   return *this;
}

glm::vec3 Camera3::getPosition()
{
   updatePositionAndOrientationOfCamera();

   return mCameraPosition;
}

Q::quat Camera3::getOrientation()
{
   updatePositionAndOrientationOfCamera();

   return mCameraGlobalOrientation;
}

float Camera3::getPitch()
{
   return mCameraPitch;
}

glm::mat4 Camera3::getViewMatrix()
{
   updatePositionAndOrientationOfCamera();
   updateViewMatrix();

   return mViewMatrix;
}

glm::mat4 Camera3::getPerspectiveProjectionMatrix()
{
   updatePerspectiveProjectionMatrix();

   return mPerspectiveProjectionMatrix;
}

glm::mat4 Camera3::getPerspectiveProjectionViewMatrix()
{
   updatePerspectiveProjectionViewMatrix();

   return mPerspectiveProjectionViewMatrix;
}

void Camera3::reposition(float            distanceBetweenPlayerAndCamera,
                         float            cameraPitch,
                         const glm::vec3& playerPosition,
                         const Q::quat&   playerOrientation,
                         const glm::vec3& offsetFromPlayerPositionToCameraTarget,
                         float            distanceBetweenPlayerAndCameraLowerLimit,
                         float            distanceBetweenPlayerAndCameraUpperLimit,
                         float            cameraPitchLowerLimit,
                         float            cameraPitchUpperLimit)
{
   mDistanceBetweenPlayerAndCamera              = distanceBetweenPlayerAndCamera;
   mCameraPitch                                 = cameraPitch;
   mPlayerPosition                              = playerPosition + offsetFromPlayerPositionToCameraTarget;
   mPlayerOrientation                           = playerOrientation;
   mOffsetFromPlayerPositionToCameraTarget      = offsetFromPlayerPositionToCameraTarget;
   mDistanceBetweenPlayerAndCameraLowerLimit    = distanceBetweenPlayerAndCameraLowerLimit;
   mDistanceBetweenPlayerAndCameraUpperLimit    = distanceBetweenPlayerAndCameraUpperLimit;
   mCameraPitchLowerLimit                       = cameraPitchLowerLimit;
   mCameraPitchUpperLimit                       = cameraPitchUpperLimit;
   mNeedToUpdatePositionAndOrientationOfCamera  = true;
   mNeedToUpdateViewMatrix                      = true;
   mNeedToUpdatePerspectiveProjectionViewMatrix = true;

   calculateInitialCameraOrientationWRTPlayer();
}

void Camera3::processMouseMovement(float xOffset, float yOffset)
{
   // When the user holds the right mouse button down and...
   // drags the cursor right, we want the camera to move left  (to rotate CWISE around the Y axis of the world)
   // drags the cursor left,  we want the camera to move right (to rotate CCWISE around the Y axis of the world)
   // drags the cursor up,    we want the camera to move down  (to rotate CWISE around the X axis of the camera)
   // drags the cursor down,  we want the camera to move up    (to rotate CCWISE around the X axis of the camera)

   // Cursor moves right (xOffset is positive) -> Need camera to rotate CWISE
   // Cursor moves left  (xOffset is negative) -> Need camera to rotate CCWISE
   // Since Q::angleAxis(yaw, ...) results in a CCWISE rotation when it's positive,
   // we need to negate the xOffset to achieve the behaviour that's described above
   float yawChangeInDeg = -xOffset * mMouseSensitivity;

   // Cursor moves up   (yOffset is positive) -> Need camera to rotate CWISE
   // Cursor moves down (yOffset is negative) -> Need camera to rotate CCWISE
   // Since Q::angleAxis(pitch, ...) results in a CCWISE rotation when it's positive,
   // we need to negate the yOffset to achieve the behaviour that's described above
   float pitchChangeInDeg = -yOffset * mMouseSensitivity;

   // Lower and upper limit checks for the pitch
   if (mCameraPitch + pitchChangeInDeg > mCameraPitchUpperLimit)
   {
      pitchChangeInDeg = mCameraPitchUpperLimit - mCameraPitch;
      mCameraPitch = mCameraPitchUpperLimit;
   }
   else if (mCameraPitch + pitchChangeInDeg < mCameraPitchLowerLimit)
   {
      pitchChangeInDeg = mCameraPitchLowerLimit - mCameraPitch;
      mCameraPitch = mCameraPitchLowerLimit;
   }
   else
   {
      mCameraPitch += pitchChangeInDeg;
   }

   // The yaw is defined as a CCWISE rotation around the Y axis
   Q::quat yawRot = Q::angleAxis(glm::radians(yawChangeInDeg),   glm::vec3(0.0f, 1.0f, 0.0f));

   // The pitch is defined as a CWISE rotation around the X axis
   // Since the rotation is CWISE, the angle is negated in the call to Q::angleAxis below
   Q::quat pitchRot = Q::angleAxis(glm::radians(-pitchChangeInDeg), glm::vec3(1.0f, 0.0f, 0.0f));

   // To avoid introducing roll, the yaw is applied globally while the pitch is applied locally
   // In other words, the yaw is applied with respect to the world's Y axis, while the pitch is applied with respect to the camera's X axis
   mCameraOrientationWRTPlayer = Q::normalized(pitchRot * mCameraOrientationWRTPlayer * yawRot);

   mNeedToUpdatePositionAndOrientationOfCamera = true;
   mNeedToUpdateViewMatrix = true;
   mNeedToUpdatePerspectiveProjectionViewMatrix = true;
}

void Camera3::processScrollWheelMovement(float yOffset)
{
   // If the scroll wheel moves up, the yOffset is positive
   // If the scroll wheel moves down, the yOffset is negative
   // We subtract the yOffset from the distance between the player and the camera so that:
   // - When the scroll wheel moves up, the distance between the player and the camera decreases
   // - When the scroll wheel moves down, the distance between the player and the camera increases

   mDistanceBetweenPlayerAndCamera -= yOffset;

   // Lower and upper limit checks
   if (mDistanceBetweenPlayerAndCamera < mDistanceBetweenPlayerAndCameraLowerLimit)
   {
      mDistanceBetweenPlayerAndCamera = mDistanceBetweenPlayerAndCameraLowerLimit;
   }
   else if (mDistanceBetweenPlayerAndCamera > mDistanceBetweenPlayerAndCameraUpperLimit)
   {
      mDistanceBetweenPlayerAndCamera = mDistanceBetweenPlayerAndCameraUpperLimit;
   }

   mNeedToUpdatePositionAndOrientationOfCamera = true;
   mNeedToUpdateViewMatrix = true;
   mNeedToUpdatePerspectiveProjectionViewMatrix = true;
}

void Camera3::processPlayerMovement(const glm::vec3& playerPosition, const Q::quat& playerOrientation)
{
   // Here we simply store the world position and orientation of the player

   mPlayerPosition    = playerPosition + mOffsetFromPlayerPositionToCameraTarget;
   mPlayerOrientation = playerOrientation;

   mNeedToUpdatePositionAndOrientationOfCamera = true;
   mNeedToUpdateViewMatrix = true;
   mNeedToUpdatePerspectiveProjectionViewMatrix = true;
}

void Camera3::calculateInitialCameraOrientationWRTPlayer()
{
   // There are two important things to note here:
   // - In OpenGL, the camera always looks down the -Z axis, which means that the camera's Z axis is the opposite of the view direction
   // - Q::lookRotation calculates a quaternion that rotates from the world's Z axis to a desired direction,
   //   which makes that direction the Z axis of the rotated coordinate frame
   // That's why the orientation of the camera is calculated using the camera's Z axis instead of the view direction

   // The pitch is usually defined as a CWISE rotation around the X axis,
   // which means that you can think of it as being measured starting at the Z axis and increasing as you move along an arc to the Y axis
   // For the value received by the constructor of this 3rd person camera, however, it's defined as a CCWISE rotation around the X axis,
   // which means that you can think of it as being measured starting at the -Z axis and increasing as you move along an arc to the Y axis
   // That way, the pitch increases when the camera moves up behind the player's back, and it decreases when it moves down,
   // which makes it easier to specify an initial value
   Q::quat pitchRotation = Q::angleAxis(glm::radians(mCameraPitch), glm::vec3(1.0f, 0.0f, 0.0f));

   // Here we want to calculate the camera's orientation with respect to the player's coordinate frame
   // If you picture the player at the origin and the camera floating behind the player's back with no yaw,
   // it's clear that the camera's Z axis (the opposite of the view direction) is equal to the -Z axis rotated up by the pitch
   glm::vec3 cameraZ = pitchRotation * glm::vec3(0.0f, 0.0f, -1.0f);

   // This is the orientation of the camera WRT the player, that is, the local orientation of the camera, not the global one
   mCameraOrientationWRTPlayer = Q::lookRotation(cameraZ, glm::vec3(0.0f, 1.0f, 0.0f));
}

void Camera3::updatePositionAndOrientationOfCamera()
{
   if (mNeedToUpdatePositionAndOrientationOfCamera)
   {
      // Combine the orientation of the camera WRT the player with the orientation of the player
      // Note how the orientation of the camera WRT the player is applied as a child rotation
      mCameraGlobalOrientation = mCameraOrientationWRTPlayer * mPlayerOrientation;

      // Calculate the Z axis of the camera, which points from the player to the camera, since the view direction is the -Z axis of the camera
      glm::vec3 cameraZ = mCameraGlobalOrientation * glm::vec3(0.0f, 0.0f, 1.0f);

      // Calculate the position of the camera by starting at the position of the player and moving the distance between the player and the camera
      // along the Z axis of the camera
      mCameraPosition = mPlayerPosition + (cameraZ * mDistanceBetweenPlayerAndCamera);

      mNeedToUpdatePositionAndOrientationOfCamera = false;
   }
}

void Camera3::updateViewMatrix()
{
   /*
      The camera's model matrix can be calculated as follows (first rotate, then translate):

         cameraModelMat = cameraTranslation * cameraRotation

      The view matrix is equal to the inverse of the camera's model matrix
      Think about it this way:
      - We know the translation and rotation necessary to place and orient the camera
      - If we apply the opposite translation and rotation to the world, it's as if we were looking at it through the camera

      So with that it mind, we can calculate the view matrix as follows:

         viewMat = cameraModelMat^-1 = (cameraTranslation * cameraRotation)^-1 = cameraRotation^-1 * cameraTranslation^-1

      Which looks like this in code:

         glm::mat4 inverseCameraRotation = Q::quatToMat4(Q::conjugate(mCameraGlobalOrientation));
         glm::mat4 inverseCameraTranslation = glm::translate(glm::mat4(1.0), -mCameraPosition);

         mViewMatrix = inverseCameraRotation * inverseCameraTranslation;

      The implementation below is simply an optimized version of the above
   */

   if (mNeedToUpdateViewMatrix)
   {
      mViewMatrix       = Q::quatToMat4(Q::conjugate(mCameraGlobalOrientation));
      // Dot product of X axis with negated position
      mViewMatrix[3][0] = -(mViewMatrix[0][0] * mCameraPosition.x + mViewMatrix[1][0] * mCameraPosition.y + mViewMatrix[2][0] * mCameraPosition.z);
      // Dot product of Y axis with negated position
      mViewMatrix[3][1] = -(mViewMatrix[0][1] * mCameraPosition.x + mViewMatrix[1][1] * mCameraPosition.y + mViewMatrix[2][1] * mCameraPosition.z);
      // Dot product of Z axis with negated position
      mViewMatrix[3][2] = -(mViewMatrix[0][2] * mCameraPosition.x + mViewMatrix[1][2] * mCameraPosition.y + mViewMatrix[2][2] * mCameraPosition.z);

      mNeedToUpdateViewMatrix = false;
   }
}

void Camera3::updatePerspectiveProjectionMatrix()
{
   if (mNeedToUpdatePerspectiveProjectionMatrix)
   {
      mPerspectiveProjectionMatrix = glm::perspective(glm::radians(mFieldOfViewYInDeg),
                                                      mAspectRatio,
                                                      mNear,
                                                      mFar);

      mNeedToUpdatePerspectiveProjectionMatrix = false;
   }
}

void Camera3::updatePerspectiveProjectionViewMatrix()
{
   if (mNeedToUpdatePerspectiveProjectionViewMatrix)
   {
      mPerspectiveProjectionViewMatrix = getPerspectiveProjectionMatrix() * getViewMatrix();

      mNeedToUpdatePerspectiveProjectionViewMatrix = false;
   }
}
