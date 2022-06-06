#ifndef SKELETON_VIEWER_H
#define SKELETON_VIEWER_H

#include <array>

#include "Pose.h"
#include "shader.h"

class SkeletonViewer
{
public:

   SkeletonViewer();
   ~SkeletonViewer();

   SkeletonViewer(const SkeletonViewer&) = delete;
   SkeletonViewer& operator=(const SkeletonViewer&) = delete;

   SkeletonViewer(SkeletonViewer&& rhs) noexcept;
   SkeletonViewer& operator=(SkeletonViewer&& rhs) noexcept;

   void InitializeBones(const Pose& pose);

   void UpdateBones(const Pose& animatedPose, const std::vector<glm::mat4>& animatedPosePalette);

   void RenderBones(const Transform& model, const glm::mat4& projectionView);
   void RenderJoints(const Transform& model, const glm::mat4& projectionView, const std::vector<glm::mat4>& animatedPosePalette);

private:

   void LoadBoneBuffers();
   void ConfigureBonesVAO(int posAttribLocation, int colorAttribLocation);

   void LoadJointBuffers();
   void ConfigureJointsVAO(int posAttribLocation, int normalAttribLocation);

   void ResizeBoneContainers(const Pose& animatedPose);

   unsigned int             mBonesVAO;
   unsigned int             mBonesVBO;

   unsigned int             mJointsVAO;
   unsigned int             mJointsVBO;
   unsigned int             mJointsEBO;

   std::shared_ptr<Shader>  mBoneShader;
   std::shared_ptr<Shader>  mJointShader;

   std::array<glm::vec3, 3> mBoneColorPalette;
   std::vector<glm::vec3>   mBonePositions;
   std::vector<glm::vec3>   mBoneColors;
};

#endif
