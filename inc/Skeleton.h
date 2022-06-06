#ifndef SKELETON_H
#define SKELETON_H

#include "Pose.h"
#include <string>

// TODO: Check const-correctness
class Skeleton
{
public:

   Skeleton() = default;
   Skeleton(const Pose& restPose, const Pose& bindPose, const std::vector<std::string>& jointNames);

   void                      Set(const Pose& restPose, const Pose& bindPose, const std::vector<std::string>& jointNames);

   Pose&                     GetRestPose();
   Pose&                     GetBindPose();
   std::vector<glm::mat4>&   GetInvBindPose();
   std::vector<std::string>& GetJointNames();
   std::string&              GetJointName(unsigned int jointIndex);

protected:

   void                      UpdateInverseBindPose();

   Pose                     mRestPose;
   Pose                     mBindPose;
   std::vector<glm::mat4>   mInvBindPose;
   std::vector<std::string> mJointNames;
};

#endif
