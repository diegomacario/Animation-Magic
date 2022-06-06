#include "RearrangeBones.h"
#include <list>

/*
   The RearrangeSkeleton function rearranges the array of joints of a skeleton so that the parent joints
   always have a lower index than their child joints in the array of joints

   By rearranging things this way, we speed up track sampling and pose palette generation considerably

   So this function can take a pose like this one:

   (root) 3 ---- 2
                 |
                 |
                 1 ---- 0

              +----+----+----+----+
    Joint IDs |  0 |  1 |  2 |  3 |
              +----+----+----+----+
   Parent IDs |  1 |  2 |  3 | -1 |
              +----+----+----+----+

   Where the child joints always come first in the array of joints, and rearrange it so that it looks like this:

   (root) 0 ---- 1
                 |
                 |
                 2 ---- 3

              +----+----+----+----+
    Joint IDs |  0 |  1 |  2 |  3 |
              +----+----+----+----+
   Parent IDs | -1 |  0 |  1 |  2 |
              +----+----+----+----+

   Where the parent joints always come first in the array of joints
*/
JointMap RearrangeSkeleton(Skeleton& skeleton)
{
   Pose& restPose = skeleton.GetRestPose();
   Pose& bindPose = skeleton.GetBindPose();

   unsigned int numJoints = restPose.GetNumberOfJoints();
   if (numJoints == 0)
   {
      // If the number of joints is zero, return an empty JointMap,
      // as there are no joints to rearrange
      return JointMap();
   }

   /*
      The jointHierarchy vector of vectors is a two-dimensional array
      whose first dimension corresponds to the index of each joint in the rest pose,
      and whose second dimension corresponds to the indices of the direct children of each joint in the rest pose:

      jointHierarchy[indices of joints][indices of the direct children of joints]

      The loop below simply fills the jointHierarchy with the appropriate values

      For example, for a pose like this one:

      (root) 3 ---- 2
                    |
                    |
                    1 ---- 4
                    |
                    |
                    0

      The jointHierarchy would look like this:

                                 +---+---+---+---+---+
      Indices of Joints          | 0 | 1 | 2 | 3 | 4 |
                                 +---+---+---+---+---+
      Indices of Direct Children |   | 0 | 1 | 2 |   |
                                 |   | 4 |   |   |   |
                                 +---+---+---+---+---+

      Also note how the loop below adds the roots to the jointsToBeMapped list,
      which we will use to process the jointHierarchy later
   */
   std::vector<std::vector<int>> jointHierarchy(numJoints);
   std::list<int> jointsToBeMapped;
   for (unsigned int jointIndex = 0; jointIndex < numJoints; ++jointIndex)
   {
      // Get the parent of the current joint
      int parentIndex = restPose.GetParent(jointIndex);
      if (parentIndex >= 0)
      {
         // If the joint has a parent, then add it under its parent in the jointHierarchy
         jointHierarchy[parentIndex].push_back(static_cast<int>(jointIndex));
      }
      else
      {
         // If the joint is a root, add it to the jointsToBeMapped list
         jointsToBeMapped.push_back(static_cast<int>(jointIndex));
      }
   }

   /*
      The loop below fills the two JointMaps that we will use to rearrange
      the array of joints so that the parent joints always have a lower index than their child joints in the array of joints

      For example, for the pose from the previous example:

      (root) 3 ---- 2
                    |
                    |
                    1 ---- 4
                    |
                    |
                    0

                                 +---+---+---+---+---+
      Indices of Joints          | 0 | 1 | 2 | 3 | 4 |
                                 +---+---+---+---+---+
      Indices of Direct Children |   | 0 | 1 | 2 |   |
                                 |   | 4 |   |   |   |
                                 +---+---+---+---+---+

      The loop below would do this:

      Iteration 0: Map the root (joint 3) - Add its direct child (joint 2) to the jointsToBeMapped list
      Iteration 1: Map joint 2            - Add its direct child (joint 1) to the jointsToBeMapped list
      Iteration 2: Map joint 1            - Add its direct children (joints 0 and 4) to the jointsToBeMapped list
      Iteration 3: Map joint 0            - Add nothing to the jointsToBeMapped list
      Iteration 4: Map joint 4            - Add nothing to the jointsToBeMapped list
   */

   // The JointMaps below can be used to map new (rearranged) joint indices
   // to old joint indices and vice versa
   JointMap mapNewToOld;
   JointMap mapOldToNew;
   int newIndexOfCurrJoint = 0;
   while (jointsToBeMapped.size() > 0)
   {
      // In the first iteration we start with the root, and if there's more than one,
      // the next iterations process the other ones
      int oldIndexOfCurrJoint = *jointsToBeMapped.begin();
      jointsToBeMapped.pop_front();

      // Below we get the indices of the direct children of the current joint
      // and add them at the back of the process list
      // By doing this we ensure that we will process the entire joint hierarchy and in the correct order:
      // parents first, then children
      std::vector<int>& children = jointHierarchy[oldIndexOfCurrJoint];
      unsigned int numChildren = static_cast<unsigned int>(children.size());
      for (unsigned int i = 0; i < numChildren; ++i)
      {
         jointsToBeMapped.push_back(children[i]);
      }

      // Map the new index of the current joint to the old one and vice versa
      // We are not rearranging the array of joints yet
      // We are just storing how the indices will need to change to achieve the rearrangement
      mapNewToOld[newIndexOfCurrJoint] = oldIndexOfCurrJoint;
      mapOldToNew[oldIndexOfCurrJoint] = newIndexOfCurrJoint;
      newIndexOfCurrJoint += 1;
   }

   // TODO: Not sure why this is necessary
   mapNewToOld[-1] = -1;
   mapOldToNew[-1] = -1;

   // Use the JointMaps that were filled in the previous loop to create the rearranged rest pose, bind pose and list of names
   Pose newRestPose(numJoints);
   Pose newBindPose(numJoints);
   std::vector<std::string> newNames(numJoints);
   for (unsigned int newJointIndex = 0; newJointIndex < numJoints; ++newJointIndex)
   {
      // Get the old joint index that corresponds to the new one
      int oldJointIndex = mapNewToOld[newJointIndex];

      // Store the rest and bind local transforms of the old joint index at the new joint index
      newRestPose.SetLocalTransform(newJointIndex, restPose.GetLocalTransform(oldJointIndex));
      newBindPose.SetLocalTransform(newJointIndex, bindPose.GetLocalTransform(oldJointIndex));

      // Store the joint name of the old joint index at the new joint index
      newNames[newJointIndex] = skeleton.GetJointName(oldJointIndex);

      int newParentIndex = mapOldToNew[bindPose.GetParent(oldJointIndex)];
      newRestPose.SetParent(newJointIndex, newParentIndex);
      newBindPose.SetParent(newJointIndex, newParentIndex);
   }

   // Update the rest pose, bind pose and list of names of the skeleton
   skeleton.Set(newRestPose, newBindPose, newNames);

   return mapOldToNew;
}

void RearrangeClip(Clip& clip, JointMap& jointMap)
{
   // Loop over all the transform tracks of the clip and update the indices of the joints that they target
   for (unsigned int transfTrackIndex = 0,
        numTransfTracks = clip.GetNumberOfTransformTracks();
        transfTrackIndex < numTransfTracks;
        ++transfTrackIndex)
   {
      int oldJointIndex = static_cast<int>(clip.GetJointIDOfTransformTrack(transfTrackIndex));
      unsigned int newJointIndex = static_cast<unsigned int>(jointMap[oldJointIndex]);
      clip.SetJointIDOfTransformTrack(transfTrackIndex, newJointIndex);
   }
}

void RearrangeFastClip(FastClip& fastClip, JointMap& jointMap)
{
   // Loop over all the transform tracks of the fast clip and update the indices of the joints that they target
   for (unsigned int transfTrackIndex = 0,
        numTransfTracks = fastClip.GetNumberOfTransformTracks();
        transfTrackIndex < numTransfTracks;
        ++transfTrackIndex)
   {
      int oldJointIndex = static_cast<int>(fastClip.GetJointIDOfTransformTrack(transfTrackIndex));
      unsigned int newJointIndex = static_cast<unsigned int>(jointMap[oldJointIndex]);
      fastClip.SetJointIDOfTransformTrack(transfTrackIndex, newJointIndex);
   }
}

void RearrangeMesh(AnimatedMesh& mesh, JointMap& jointMap)
{
   std::vector<glm::ivec4>& influences = mesh.GetInfluences();

   // Loop over all the influences of the mesh
   for (unsigned int influenceIndex = 0,
        numInfluences = static_cast<unsigned int>(influences.size());
        influenceIndex < numInfluences;
        ++influenceIndex)
   {
      // Update the indices of the influencing joints
      influences[influenceIndex].x = jointMap[influences[influenceIndex].x];
      influences[influenceIndex].y = jointMap[influences[influenceIndex].y];
      influences[influenceIndex].z = jointMap[influences[influenceIndex].z];
      influences[influenceIndex].w = jointMap[influences[influenceIndex].w];
   }

   // TODO: This is inefficient. We only need to load the influences.
   //       To fix this, create functions like LoadPositions, LoadNormals, etc.
   mesh.LoadBuffers();
}
