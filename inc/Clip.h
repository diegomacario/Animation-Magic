#ifndef CLIP_H
#define CLIP_H

#include <vector>
#include <string>
#include "TransformTrack.h"
#include "Pose.h"

// TODO: If we unify the Track and FastTrack classes, this wouldn't have to be a template anymore
//       and we could delete the OptimizeClip function
template <typename TRACK>
class TClip
{
public:

   TClip();

   unsigned int        GetNumberOfTransformTracks() const;

   unsigned int        GetJointIDOfTransformTrack(unsigned int transfTrackIndex) const;
   void                SetJointIDOfTransformTrack(unsigned int transfTrackIndex, unsigned int jointID);

   TRACK&              GetTransformTrackOfJoint(unsigned int jointID);
   void                SetTransformTrackOfJoint(unsigned int jointID, const TRACK& transfTrack);

   std::vector<TRACK>& GetTransformTracks() { return mTransformTracks; }

   std::string         GetName() const;
   void                SetName(const std::string& name);

   float               GetStartTime() const;
   float               GetEndTime() const;
   float               GetDuration() const;
   void                RecalculateDuration();
   bool                IsTimePastEnd(float time);

   bool                GetLooping() const;
   void                SetLooping(bool looping);

   float               Sample(Pose& ioPose, float time) const;

private:

   float               AdjustTimeToBeWithinClip(float time) const;

   std::vector<TRACK> mTransformTracks;
   std::string        mName;
   float              mStartTime;
   float              mEndTime;
   bool               mLooping;
};

typedef TClip<TransformTrack> Clip;
typedef TClip<FastTransformTrack> FastClip;

FastClip OptimizeClip(Clip& clip);

#endif
