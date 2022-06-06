#ifndef TRACK_H
#define TRACK_H

#include <vector>

#include "Frame.h"
#include "quat.h"
#include "Interpolation.h"

// Track

template<typename T, unsigned int N>
class Track
{
public:

   Track();
   virtual ~Track() = default;

   Frame<N>&       GetFrame(unsigned int frameIndex);
   void            SetFrame(unsigned int frameIndex, const Frame<N>& frame);

   unsigned int    GetNumberOfFrames() const;
   void            SetNumberOfFrames(unsigned int numFrames);

   Interpolation   GetInterpolation() const;
   void            SetInterpolation(Interpolation interpolation);

   float           GetStartTime() const;
   float           GetEndTime() const;

   T               Sample(float time, bool looping) const;

protected:

   virtual int     GetIndexOfLastFrameBeforeTime(float time, bool looping) const;
   float           AdjustTimeToBeWithinTrack(float time, bool looping) const;

   T               InterpolateUsingCubicHermiteSpline(float t, const T& p1, const T& outTangentOfP1, const T& p2, const T& inTangentOfP2) const;

   // TODO: Is there a cleaner way of doing this?
   T               Cast(const float* value) const;

   T               SampleConstant(float time, bool looping) const;
   T               SampleLinear(float time, bool looping) const;
   T               SampleCubic(float time, bool looping) const;

   std::vector<Frame<N>> mFrames;
   Interpolation         mInterpolation;
};

typedef Track<float, 1>     ScalarTrack;
typedef Track<glm::vec3, 3> VectorTrack;
typedef Track<Q::quat, 4>   QuaternionTrack;

// FastTrack

// Remember how the Track::GetIndexOfLastFrameBeforeTime method works:
// It loops backwards over the list of frames comparing their times until it finds the right one
// That linear search is very slow, specially for long animations with many frames
// One improvement would be to use binary search instead of linear search to find the right frame,
// but it's actually possible to make finding the right frame a constant lookup operation
// Achieving that is simple:
// - While loading a track, we sample it with a uniform interval (e.g. 60 times per second)
// - For every sample, we store the index of the frame that came right before it
// - By doing this we end up with a map that maps samples into frames
// - At run-time, when trying to find the index of the last frame before a specific time, we can
//   transform the time into its closest sample, and then we can use the map we generated at
//   load-time to find the right frame
// The only drawback of this technique is the additional memory used by the map of samples to frame indices

// TODO: We can probably unify the Track and FastTrack classes, which would allow us to delete the OptimizeTrack function
template<typename T, unsigned int N>
class FastTrack : public Track<T, N>
{
public:

   FastTrack() = default;
   virtual ~FastTrack() = default;

   void        GenerateSampleToFrameIndexMap();

protected:

   virtual int GetIndexOfLastFrameBeforeTime(float time, bool looping) const override;

   std::vector<unsigned int> mSampleToFrameIndexMap;
};

typedef FastTrack<float, 1>     FastScalarTrack;
typedef FastTrack<glm::vec3, 3> FastVectorTrack;
typedef FastTrack<Q::quat, 4>   FastQuaternionTrack;

template<typename T, unsigned int N>
FastTrack<T, N> OptimizeTrack(Track<T, N>& track);

#endif
