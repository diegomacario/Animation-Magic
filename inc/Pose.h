#ifndef POSE_H
#define POSE_H

#include <vector>
#include "Transform.h"

/*
   A Pose stores a collection of joints, which are represented as local transforms,
   and a parallel list of their parents, as illustrated below:

      0 (root)
     /
    /
   1
    \
     \
      2

              +----+----+----+
    Joint IDs |  0 |  1 |  2 |
              +----+----+----+
   Parent IDs | -1 |  0 |  1 |
              +----+----+----+
*/

class Pose
{
public:

   Pose() = default;
   Pose(unsigned int numJoints);
   Pose(const Pose& rhs);
   Pose&        operator=(const Pose& rhs);

   bool         operator==(const Pose& rhs);
   bool         operator!=(const Pose& rhs);

   unsigned int GetNumberOfJoints() const;
   void         SetNumberOfJoints(unsigned int numJoints);

   // TODO: Revisit the GetXTransform functions and return const references where possible
   Transform    GetLocalTransform(unsigned int jointIndex) const;
   void         SetLocalTransform(unsigned int jointIndex, const Transform& transform);
   Transform    GetGlobalTransform(unsigned int jointIndex) const;

   void         GetMatrixPalette(std::vector<glm::mat4>& palette) const;

   int          GetParent(unsigned int jointIndex) const;
   void         SetParent(unsigned int jointIndex, int parentIndex);

private:

   std::vector<Transform> mJointLocalTransforms;
   std::vector<int>       mParentIndices;
};

#endif
