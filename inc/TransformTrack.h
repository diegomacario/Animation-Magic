#ifndef TRANSFORM_TRACK_H
#define TRANSFORM_TRACK_H

#include "Track.h"
#include "Transform.h"

// TODO: If we unify the Track and FastTrack classes, this wouldn't have to be a template anymore
//       and we could delete the OptimizeTransformTrack function
template <typename VTRACK, typename QTRACK>
class TTransformTrack
{
public:

   TTransformTrack();

   unsigned int GetJointID() const;
   void         SetJointID(unsigned int id);

   VTRACK&      GetPositionTrack();
   void         SetPositionTrack(const VTRACK& positionTrack);

   QTRACK&      GetRotationTrack();
   void         SetRotationTrack(const QTRACK& rotationTrack);

   VTRACK&      GetScaleTrack();
   void         SetScaleTrack(const VTRACK& scaleTrack);

   float        GetStartTime() const;
   float        GetEndTime() const;

   bool         IsValid() const;

   Transform    Sample(const Transform& defaultTransform, float time, bool looping) const;

private:

   // Each TransformTrack stores the ID of the joint it animates
   unsigned int mJointID;

   VTRACK       mPosition;
   QTRACK       mRotation;
   VTRACK       mScale;
};

typedef TTransformTrack<VectorTrack, QuaternionTrack>         TransformTrack;
typedef TTransformTrack<FastVectorTrack, FastQuaternionTrack> FastTransformTrack;

FastTransformTrack OptimizeTransformTrack(TransformTrack& transformTrack);

#endif
