#include "Skeleton.h"

Skeleton::Skeleton(const Pose& restPose, const Pose& bindPose, const std::vector<std::string>& jointNames)
   : mRestPose(restPose)
   , mBindPose(bindPose)
   , mJointNames(jointNames)
{
   UpdateInverseBindPose();
}

void Skeleton::Set(const Pose& restPose, const Pose& bindPose, const std::vector<std::string>& jointNames)
{
   mRestPose   = restPose;
   mBindPose   = bindPose;
   mJointNames = jointNames;
   UpdateInverseBindPose();
}

Pose& Skeleton::GetRestPose()
{
   return mRestPose;
}

Pose& Skeleton::GetBindPose()
{
   return mBindPose;
}

std::vector<glm::mat4>& Skeleton::GetInvBindPose()
{
   return mInvBindPose;
}

std::vector<std::string>& Skeleton::GetJointNames()
{
   return mJointNames;
}

std::string& Skeleton::GetJointName(unsigned int jointIndex)
{
   return mJointNames[jointIndex];
}

void Skeleton::UpdateInverseBindPose()
{
   unsigned int numJoints = mBindPose.GetNumberOfJoints();

   mInvBindPose.resize(numJoints);

   // The bind pose is stored as a set of local transforms,
   // while the inverse bind pose is stored as an array of global transform matrices
   // Below we do the following to calculate the inverse bind pose:
   // - Get the global transform of each joint in the bind pose
   // - Convert it into a transform matrix
   // - Invert it
   for (unsigned int jointIndex = 0; jointIndex < numJoints; ++jointIndex)
   {
      Transform globalBindTransf = mBindPose.GetGlobalTransform(jointIndex);

      // TODO: Perhaps add error for matrices with determinant equal to zero
      mInvBindPose[jointIndex] = glm::inverse(transformToMat4(globalBindTransf));
   }
}
