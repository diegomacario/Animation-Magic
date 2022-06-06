#ifndef ANIMATED_MESH_H
#define ANIMATED_MESH_H

#include <array>

#include "Skeleton.h"
#include "Pose.h"

class AnimatedMesh
{
public:

   AnimatedMesh();
   ~AnimatedMesh();

   AnimatedMesh(const AnimatedMesh&) = delete;
   AnimatedMesh& operator=(const AnimatedMesh&) = delete;

   AnimatedMesh(AnimatedMesh&& rhs) noexcept;
   AnimatedMesh& operator=(AnimatedMesh&& rhs) noexcept;

   std::vector<glm::vec3>&    GetPositions()  { return mPositions;  }
   std::vector<glm::vec3>&    GetNormals()    { return mNormals;    }
   std::vector<glm::vec2>&    GetTexCoords()  { return mTexCoords;  }
   std::vector<glm::vec4>&    GetWeights()    { return mWeights;    }
   std::vector<glm::ivec4>&   GetInfluences() { return mInfluences; }
   std::vector<unsigned int>& GetIndices()    { return mIndices;    }

   void                       LoadBuffers();

   void                       ConfigureVAO(int posAttribLocation,
                                           int normalAttribLocation,
                                           int texCoordsAttribLocation,
                                           int weightsAttribLocation,
                                           int influencesAttribLocation);

   void                       UnconfigureVAO(int posAttribLocation,
                                             int normalAttribLocation,
                                             int texCoordsAttribLocation,
                                             int weightsAttribLocation,
                                             int influencesAttribLocation);

   void                       BindFloatAttribute(int attribLocation, unsigned int VBO, int numComponents);
   void                       BindIntAttribute(int attribLocation, unsigned int VBO, int numComponents);
   void                       UnbindAttribute(int attribLocation, unsigned int VBO);

   void                       Render();
   void                       RenderInstanced(unsigned int numInstances);

   void                       SkinMeshOnTheCPUUsingMatrices(Skeleton& skeleton, Pose& animatedPose);
   void                       SkinMeshOnTheCPUUsingTransforms(Skeleton& skeleton, Pose& animatedPose);
   void                       SkinMeshOnTheCPU(std::vector<glm::mat4>& skinMatrices);

private:

   std::vector<glm::vec3>      mPositions;
   std::vector<glm::vec3>      mNormals;
   std::vector<glm::vec2>      mTexCoords;
   std::vector<glm::vec4>      mWeights;
   std::vector<glm::ivec4>     mInfluences;
   std::vector<unsigned int>   mIndices;

   enum VBOTypes : unsigned int
   {
      positions  = 0,
      normals    = 1,
      texCoords  = 2,
      weights    = 3,
      influences = 4
   };

   unsigned int                mNumIndices; // TODO: Unused and never initialized for now
   unsigned int                mVAO;
   std::array<unsigned int, 5> mVBOs;
   unsigned int                mEBO;

   std::vector<glm::vec3>      mSkinnedPositions;
   std::vector<glm::vec3>      mSkinnedNormals;
   std::vector<glm::mat4>      mAnimatedPosePalette;
};

#endif
