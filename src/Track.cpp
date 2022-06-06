#include <glm/gtx/compatibility.hpp>

#include "Track.h"

namespace TrackHelpers
{
   float     Interpolate(float a, float b, float t)                       { return a + (b - a) * t; }
   glm::vec3 Interpolate(const glm::vec3& a, const glm::vec3& b, float t) { return glm::lerp(a, b, t); }
   Q::quat   Interpolate(const Q::quat& a, const Q::quat& b, float t)     { return (Q::dot(a, b) < 0) ? Q::nlerp(a, -b, t) : Q::nlerp(a, b, t); }

   // TODO: Is there a cleaner way of doing this?
   float     AdjustResultOfCubicHermiteSpline(float f)            { return f; }
   glm::vec3 AdjustResultOfCubicHermiteSpline(const glm::vec3& v) { return v; }
   Q::quat   AdjustResultOfCubicHermiteSpline(const Q::quat& q)   { return Q::normalized(q); }

   // TODO: Is there a cleaner way of doing this?
   void      NeighborhoodCheck(const float& a, float& b)         { }
   void      NeighborhoodCheck(const glm::vec3& a, glm::vec3& b) { }
   void      NeighborhoodCheck(const Q::quat& a, Q::quat& b)     { if (Q::dot(a, b) < 0) b = -b; }
};

// Track

template<typename T, unsigned int N>
Track<T, N>::Track()
   : mInterpolation(Interpolation::Linear)
{

}

template<typename T, unsigned int N>
Frame<N>& Track<T, N>::GetFrame(unsigned int frameIndex)
{
   return mFrames[frameIndex];
}

template<typename T, unsigned int N>
void Track<T, N>::SetFrame(unsigned int frameIndex, const Frame<N>& frame)
{
   mFrames[frameIndex] = frame;
}

template<typename T, unsigned int N>
unsigned int Track<T, N>::GetNumberOfFrames() const
{
   return static_cast<unsigned int>(mFrames.size());
}

template<typename T, unsigned int N>
void Track<T, N>::SetNumberOfFrames(unsigned int numFrames)
{
   mFrames.resize(numFrames);
}

template<typename T, unsigned int N>
Interpolation Track<T, N>::GetInterpolation() const
{
   return mInterpolation;
}

template<typename T, unsigned int N>
void Track<T, N>::SetInterpolation(Interpolation interpolation)
{
   mInterpolation = interpolation;
}

template<typename T, unsigned int N>
float Track<T, N>::GetStartTime() const
{
   return mFrames[0].mTime;
}

template<typename T, unsigned int N>
float Track<T, N>::GetEndTime() const
{
   return mFrames[mFrames.size() - 1].mTime;
}

template<typename T, unsigned int N>
T Track<T, N>::Sample(float time, bool looping) const
{
   if (mInterpolation == Interpolation::Constant)
   {
      return SampleConstant(time, looping);
   }
   else if (mInterpolation == Interpolation::Linear)
   {
      return SampleLinear(time, looping);
   }
   else
   {
      return SampleCubic(time, looping);
   }
}

template<typename T, unsigned int N>
int Track<T, N>::GetIndexOfLastFrameBeforeTime(float time, bool looping) const
{
   unsigned int numFrames = static_cast<unsigned int>(mFrames.size());
   if (numFrames <= 1)
   {
      // If the track has one frame or less, it's invalid
      // The reason why a track with one frame is invalid is that we need at least two frames to interpolate
      return -1;
   }

   if (looping)
   {
      // If looping, adjust the time so that it's inside the range of the track
      // The code below can take a time before the start, in between the start and the end, or after the end
      // and produce a properly looped value
      float startTime = mFrames[0].mTime;
      float endTime   = mFrames[numFrames - 1].mTime;
      float duration  = endTime - startTime;

      // TODO: Add duration check here? Like the one in AdjustTimeToFitTrack

      time = glm::mod(time - startTime, duration);
      if (time < 0.0f)
      {
         time += duration;
      }
      time += startTime;
   }
   else
   {
      // If not looping, any time before the start should clamp to frame zero
      // and any time after the second to last frame should clamp to that frame
      // We clamp to the second to last frame because we need a frame after it to interpolate
      if (time <= mFrames[0].mTime)
      {
         return 0;
      }

      if (time >= mFrames[numFrames - 2].mTime)
      {
         return static_cast<int>(numFrames - 2);
      }
   }

   // Loop backwards over the array of frames until we find the first one that comes
   // before the given time
   for (int i = static_cast<int>(numFrames) - 1; i >= 0; --i)
   {
      if (time >= mFrames[i].mTime)
      {
         return i;
      }
   }

   // TODO: Add error message here. We should never reach this point
   return -1;
}

template<typename T, unsigned int N>
float Track<T, N>::AdjustTimeToBeWithinTrack(float time, bool looping) const
{
   unsigned int numFrames = static_cast<unsigned int>(mFrames.size());
   if (numFrames <= 1)
   {
      // If the track has one frame or less, it's invalid
      // The reason why a track with one frame is invalid is that we need at least two frames to interpolate
      return 0.0f;
   }

   float startTime = mFrames[0].mTime;
   float endTime   = mFrames[numFrames - 1].mTime;
   float duration  = endTime - startTime;
   if (duration <= 0.0f)
   {
      // If the duration of the track is smaller than or equal to zero, it's invalid
      return 0.0f;
   }

   if (looping)
   {
      // If looping, adjust the time so that it's inside the range of the track
      // The code below can take a time before the start, in between the start and the end, or after the end
      // and produce a properly looped value
      time = glm::mod(time - startTime, duration);
      if (time < 0.0f)
      {
         time += duration;
      }
      time += startTime;
   }
   else
   {
      // If not looping, any time before the start should clamp to the start time
      // and any time after the end should clamp to the end time
      if (time <= mFrames[0].mTime)
      {
         time = startTime;
      }

      if (time >= mFrames[numFrames - 1].mTime)
      {
         time = endTime;
      }
   }

   return time;
}

template<typename T, unsigned int N>
T Track<T, N>::InterpolateUsingCubicHermiteSpline(float t, const T& p1, const T& outTangentOfP1, const T& p2, const T& inTangentOfP2) const
{
   // A cubic Hermite spline is defined using two points and two tangents:
   // - Start point p1 and its output tangent
   // - End point p2 and its input tangent

   // Quaternion neighborhood check
   // Whenever we interpolate quaternions, regardless of the interpolation method, we need to perform a neighborhood check
   // Note that the function below doesn't do anything for floats and vectors
   T adjustedP2 = p2;
   TrackHelpers::NeighborhoodCheck(p1, adjustedP2);

   float tt  = t * t;
   float ttt = tt * t;

   // Evaluate the basis functions of the cubic Hermite spline
   float basisFuncOfP1             = 2.0f * ttt - 3.0f * tt + 1.0f;
   float basisFuncOfOutTangentOfP1 = ttt - 2.0f * tt + t;
   float basisFuncOfP2             = -2.0f * ttt + 3.0f * tt;
   float basisFuncOfInTangentOfP2  = ttt - tt;

   T result = (p1 * basisFuncOfP1) +
              (outTangentOfP1 * basisFuncOfOutTangentOfP1) +
              (adjustedP2 * basisFuncOfP2) +
              (inTangentOfP2 * basisFuncOfInTangentOfP2);

   return TrackHelpers::AdjustResultOfCubicHermiteSpline(result);
}

template<>
float Track<float, 1>::Cast(const float* value) const
{
   return value[0];
}

template<>
glm::vec3 Track<glm::vec3, 3>::Cast(const float* value) const
{
   return glm::vec3(value[0], value[1], value[2]);
}

template<>
Q::quat Track<Q::quat, 4>::Cast(const float* value) const
{
   // Note that this explicit specialization of the Cast function for quaternions normalizes them
   Q::quat r = Q::quat(value[0], value[1], value[2], value[3]);
   return normalized(r);
}

template<typename T, unsigned int N>
T Track<T, N>::SampleConstant(float time, bool looping) const
{
   int frame = GetIndexOfLastFrameBeforeTime(time, looping);
   if (frame < 0 || frame >= static_cast<int>(mFrames.size()))
   {
      // If the frame index is negative or greater than the index of the last frame (numFrames - 1),
      // return a zero float, zero vector or unit quaternion
      return T();
   }

   // Return the value of the frame we found, which remains constant until the next frame
   return Cast(&mFrames[frame].mValue[0]);
}

template<typename T, unsigned int N>
T Track<T, N>::SampleLinear(float time, bool looping) const
{
   int thisFrame = GetIndexOfLastFrameBeforeTime(time, looping);
   if (thisFrame < 0 || thisFrame >= static_cast<int>(mFrames.size() - 1))
   {
      // If the frame index is negative or greater than the index of the second to last frame (numFrames - 2),
      // return a zero float, zero vector or unit quaternion
      // Note that this check is done with the second to last frame because we need a next frame to be able to interpolate
      return T();
   }

   int nextFrame = thisFrame + 1;
   float timeBetweenFrames = mFrames[nextFrame].mTime - mFrames[thisFrame].mTime;
   if (timeBetweenFrames <= 0.0f)
   {
      // If the time between frames is negative or equal to zero, return a zero float, zero vector or unit quaternion
      return T();
   }

   // Calculate the interpolation factor
   float trackTime = AdjustTimeToBeWithinTrack(time, looping);
   float t = (trackTime - mFrames[thisFrame].mTime) / timeBetweenFrames;

   // Cast the values of the frames we found to be able to call the appropriate interpolation function
   T start = Cast(&mFrames[thisFrame].mValue[0]);
   T end = Cast(&mFrames[nextFrame].mValue[0]);

   // lerp floats and vectors or nlerp quaternions with a neighborhood check
   return TrackHelpers::Interpolate(start, end, t);
}

template<typename T, unsigned int N>
T Track<T, N>::SampleCubic(float time, bool looping) const
{
   int thisFrame = GetIndexOfLastFrameBeforeTime(time, looping);
   if (thisFrame < 0 || thisFrame >= static_cast<int>(mFrames.size() - 1))
   {
      // If the frame index is negative or greater than the index of the second to last frame (numFrames - 2),
      // return a zero float, zero vector or unit quaternion
      // Note that this check is done with the second to last frame because we need a next frame to be able to interpolate
      return T();
   }

   int nextFrame = thisFrame + 1;
   float timeBetweenFrames = mFrames[nextFrame].mTime - mFrames[thisFrame].mTime;
   if (timeBetweenFrames <= 0.0f)
   {
      // If the time between frames is negative or equal to zero, return a zero float, zero vector or unit quaternion
      return T();
   }

   // Calculate the interpolation factor
   float trackTime = AdjustTimeToBeWithinTrack(time, looping);
   float t = (trackTime - mFrames[thisFrame].mTime) / timeBetweenFrames;

   // Get the first point and its output tangent from the first frame
   T p1 = Cast(&mFrames[thisFrame].mValue[0]);
   // We use memcpy instead of the Cast function to get the slope because
   // the Cast function normalizes its result when working with quaternions,
   // which shouldn't be done for slopes
   T outSlopeOfP1;
   memcpy(&outSlopeOfP1, mFrames[thisFrame].mOutSlope, N * sizeof(float));
   T outTangentOfP1 = outSlopeOfP1 * timeBetweenFrames;

   // Get the second point and its input tangent from the second frame
   T p2 = Cast(&mFrames[nextFrame].mValue[0]);
   // We use memcpy instead of the Cast function to get the slope because
   // the Cast function normalizes its result when working with quaternions,
   // which shouldn't be done for slopes
   T inSlopeOfP2;
   memcpy(&inSlopeOfP2, mFrames[nextFrame].mInSlope, N * sizeof(float));
   T inTangentOfP2 = inSlopeOfP2 * timeBetweenFrames;

   return InterpolateUsingCubicHermiteSpline(t, p1, outTangentOfP1, p2, inTangentOfP2);
}

// FastTrack

template<typename T, unsigned int N>
int FastTrack<T, N>::GetIndexOfLastFrameBeforeTime(float time, bool looping) const
{
   unsigned int numFrames = static_cast<unsigned int>(this->mFrames.size());
   if (numFrames <= 1)
   {
      // If the track has one frame or less, it's invalid
      // The reason why a track with one frame is invalid is that we need at least two frames to interpolate
      return -1;
   }

   if (looping)
   {
      // If looping, adjust the time so that it's inside the range of the track
      // The code below can take a time before the start, in between the start and the end, or after the end
      // and produce a properly looped value
      float startTime = this->mFrames[0].mTime;
      float endTime   = this->mFrames[numFrames - 1].mTime;
      float duration  = endTime - startTime;

      // TODO: Add duration check here? Like the one in AdjustTimeToFitTrack

      time = glm::mod(time - startTime, duration);
      if (time < 0.0f)
      {
         time += duration;
      }
      time += startTime;
   }
   else
   {
      // If not looping, any time before the start should clamp to frame zero
      // and any time after the second to last frame should clamp to that frame
      // We clamp to the second to last frame because we need a frame after it to interpolate
      if (time <= this->mFrames[0].mTime)
      {
         return 0;
      }

      if (time >= this->mFrames[numFrames - 2].mTime)
      {
         return static_cast<int>(numFrames - 2);
      }
   }

   float durationOfTrack = this->GetEndTime() - this->GetStartTime();

   // Calculate the number of samples
   // TODO: Create a constant for the number of samples per second
   // TODO: Not sure why we add 60 here
   //       I guess it's to make sure that all tracks have at least 60 samples,
   //       even if they are shorter than 1 second
   unsigned int numSamples = 60 + static_cast<unsigned int>(durationOfTrack * 60.0f);

   // Calculate the normalized time
   float nTime = time / durationOfTrack;

   // Transform the time into its closest sample
   unsigned int indexOfSample = static_cast<unsigned int>(nTime * static_cast<float>(numSamples));
   if (indexOfSample >= mSampleToFrameIndexMap.size())
   {
      return -1;
   }

   return static_cast<int>(mSampleToFrameIndexMap[indexOfSample]);
}

template<typename T, unsigned int N>
void FastTrack<T, N>::GenerateSampleToFrameIndexMap()
{
   int numFrames = static_cast<int>(this->mFrames.size());
   if (numFrames <= 1)
   {
      return;
   }

   float durationOfTrack = this->GetEndTime() - this->GetStartTime();

   // TODO: Create a constant for the number of samples per second
   // TODO: Not sure why we add 60 here
   //       I guess it's to make sure that all tracks have at least 60 samples,
   //       even if they are shorter than 1 second
   unsigned int numSamples = 60 + static_cast<unsigned int>(durationOfTrack * 60.0f);
   // Loop over all the samples and store their corresponding frame indices in the map
   mSampleToFrameIndexMap.resize(numSamples);
   for (unsigned int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
   {
      // Calculate the normalized sample
      float nSample = static_cast<float>(sampleIndex) / static_cast<float>(numSamples - 1);

      // Calculate the time of the sample
      float sampleTime = nSample * durationOfTrack + this->GetStartTime();

      // Loop backwards over the array of frames until we find the first one that comes
      // before the time of the sample
      // TODO: This can be optimized further
      //       If we remember the indexOfFirstFrameBeforeSample of the previous iteration,
      //       we can start the next iteration there, since the time of each sample only increases
      unsigned int indexOfFirstFrameBeforeSample = 0;
      for (int frameIndex = numFrames - 1; frameIndex >= 0; --frameIndex)
      {
         if (sampleTime >= this->mFrames[frameIndex].mTime)
         {
            indexOfFirstFrameBeforeSample = static_cast<unsigned int>(frameIndex);

            // If the first frame that comes before the time of the sample is the last frame,
            // store the index of the second to last frame instead
            if (static_cast<int>(indexOfFirstFrameBeforeSample) >= numFrames - 2)
            {
               indexOfFirstFrameBeforeSample = numFrames - 2;
            }
            break;
         }
      }

      mSampleToFrameIndexMap[sampleIndex] = indexOfFirstFrameBeforeSample;
   }
}

template<typename T, unsigned int N>
FastTrack<T, N> OptimizeTrack(Track<T, N>& track)
{
   FastTrack<T, N> result;

   // Copy the interpolation mode
   result.SetInterpolation(track.GetInterpolation());

   // Copy the frames
   unsigned int numFrames = track.GetNumberOfFrames();
   result.SetNumberOfFrames(numFrames);
   for (unsigned int frameIndex = 0; frameIndex < numFrames; ++frameIndex)
   {
      result.SetFrame(frameIndex, track.GetFrame(frameIndex));
   }

   // Generate the map
   result.GenerateSampleToFrameIndexMap();

   return result;
}

// Instantiate the desired Track classes from the Track class template
template class Track<float, 1>;
template class Track<glm::vec3, 3>;
template class Track<Q::quat, 4>;

// Instantiate the desired FastTrack classes from the FastTrack class template
template class FastTrack<float, 1>;
template class FastTrack<glm::vec3, 3>;
template class FastTrack<Q::quat, 4>;

// Instantiate the desired OptimizeTrack functions from the OptimizeTrack function template
template FastTrack<float, 1>     OptimizeTrack(Track<float, 1>& input);
template FastTrack<glm::vec3, 3> OptimizeTrack(Track<glm::vec3, 3>& input);
template FastTrack<Q::quat, 4>   OptimizeTrack(Track<Q::quat, 4>& input);
