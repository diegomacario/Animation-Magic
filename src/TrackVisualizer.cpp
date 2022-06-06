#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

#include <glm/gtc/matrix_transform.hpp>

#include "resource_manager.h"
#include "shader_loader.h"
#include "TrackVisualizer.h"

TrackVisualizer::TrackVisualizer()
   : mWidthOfGraphSpace(100.0f)
   , mHeightOfGraphSpace(100.0f)
   , mNumGraphs(0)
   , mNumCurves(0)
   , mNumTiles(0)
   , mTileWidth(0.0f)
   , mTileHeight(0.0f)
   , mTileHorizontalOffset(0.0f)
   , mTileVerticalOffset(0.0f)
   , mGraphWidth(0.0f)
   , mGraphHeight(0.0f)
   , mReferenceLinesVAO(0)
   , mReferenceLinesVBO(0)
   , mEmptyLinesVAO(0)
   , mEmptyLinesVBO(0)
   , mTrackLinesVAOs()
   , mTrackLinesVBOs()
   , mTracks()
   , mReferenceLines()
   , mEmptyLines()
   , mTrackLines()
   , mTrackLinesColorPalette{glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.65f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)}
   , mInitialized(false)
{
   mTrackShader = ResourceManager<Shader>().loadUnmanagedResource<ShaderLoader>("resources/shaders/graph.vert",
                                                                                "resources/shaders/graph.frag");
}

TrackVisualizer::~TrackVisualizer()
{
   deleteBuffers();
}

void TrackVisualizer::setTracks(std::vector<FastTransformTrack>& tracks)
{
   if (mInitialized)
   {
      // Reset
      mTracks.clear();
      mReferenceLines.clear();
      mEmptyLines.clear();
      mTrackLines.clear();
      deleteBuffers();
      mReferenceLinesVAO = 0;
      mReferenceLinesVBO = 0;
      mEmptyLinesVAO = 0;
      mEmptyLinesVBO = 0;
      mTrackLinesVAOs.clear();
      mTrackLinesVBOs.clear();
   }

   for (int i = 0; i < tracks.size(); ++i)
   {
      mTracks.push_back(tracks[i].GetRotationTrack());
   }

   mNumGraphs = static_cast<unsigned int>(mTracks.size());
   mNumCurves = mNumGraphs * 4;
   mNumTiles  = static_cast<unsigned int>(glm::sqrt(mNumGraphs));
   while (mNumGraphs > (mNumTiles * mNumTiles))
   {
      ++mNumTiles;
   }

   mTileWidth  = (mWidthOfGraphSpace * (1280.0f / 720.0f)) / mNumTiles;
   mTileHeight = mHeightOfGraphSpace / mNumTiles;
   mTileHorizontalOffset = mTileWidth * 0.1f * (720.0f / 1280.0f);
   mTileVerticalOffset   = mTileHeight * 0.1f;
   mGraphWidth  = mTileWidth - mTileHorizontalOffset;
   mGraphHeight = mTileHeight - mTileVerticalOffset;

   initializeReferenceLines();
   initializeTrackLines();

   // Create buffers

   glGenVertexArrays(1, &mReferenceLinesVAO);
   glGenBuffers(1, &mReferenceLinesVBO);

   glGenVertexArrays(1, &mEmptyLinesVAO);
   glGenBuffers(1, &mEmptyLinesVBO);

   mTrackLinesVAOs.resize(mNumCurves);
   mTrackLinesVBOs.resize(mNumCurves);
   glGenVertexArrays(mNumCurves, &(mTrackLinesVAOs[0]));
   glGenBuffers(mNumCurves, &(mTrackLinesVBOs[0]));

   // Load buffers

   glBindVertexArray(mReferenceLinesVAO);
   glBindBuffer(GL_ARRAY_BUFFER, mReferenceLinesVBO);
   glBufferData(GL_ARRAY_BUFFER, mReferenceLines.size() * sizeof(glm::vec3), &mReferenceLines[0], GL_STATIC_DRAW);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindVertexArray(0);

   glBindVertexArray(mEmptyLinesVAO);
   glBindBuffer(GL_ARRAY_BUFFER, mEmptyLinesVBO);
   glBufferData(GL_ARRAY_BUFFER, mEmptyLines.size() * sizeof(glm::vec3), &mEmptyLines[0], GL_STATIC_DRAW);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindVertexArray(0);

   for (int i = 0; i < mNumCurves; ++i)
   {
      glBindVertexArray(mTrackLinesVAOs[i]);
      glBindBuffer(GL_ARRAY_BUFFER, mTrackLinesVBOs[i]);
      glBufferData(GL_ARRAY_BUFFER, mTrackLines[i].size() * sizeof(glm::vec3), &(mTrackLines[i][0]), GL_DYNAMIC_DRAW);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindVertexArray(0);
   }

   // Configure VAOs

   int posAttribLocation = mTrackShader->getAttributeLocation("position");

   glBindVertexArray(mReferenceLinesVAO);
   glBindBuffer(GL_ARRAY_BUFFER, mReferenceLinesVBO);
   glEnableVertexAttribArray(posAttribLocation);
   glVertexAttribPointer(posAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindVertexArray(0);

   glBindVertexArray(mEmptyLinesVAO);
   glBindBuffer(GL_ARRAY_BUFFER, mEmptyLinesVBO);
   glEnableVertexAttribArray(posAttribLocation);
   glVertexAttribPointer(posAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindVertexArray(0);

   for (int i = 0; i < mNumCurves; ++i)
   {
      glBindVertexArray(mTrackLinesVAOs[i]);
      glBindBuffer(GL_ARRAY_BUFFER, mTrackLinesVBOs[i]);
      glEnableVertexAttribArray(posAttribLocation);
      glVertexAttribPointer(posAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindVertexArray(0);
   }

   mInitialized = true;
}

void TrackVisualizer::update(float deltaTime, float playbackSpeed)
{
   float speedFactor = (playbackSpeed == 0.0f) ? 0.0f : 1.0f + (4.0f * playbackSpeed);
   float xOffset = deltaTime * speedFactor;
   unsigned int trackIndex = 0;
   unsigned int curveIndex = 0;
   for (int j = 0; j < mNumTiles; ++j)
   {
      for (int i = 0; i < mNumTiles; ++i)
      {
         if (trackIndex >= mNumGraphs)
         {
            break;
         }

         float xPosOfOriginOfGraph = (mTileWidth * i) + (mTileHorizontalOffset / 2.0f);

         for (int k = 0; k < 4; ++k)
         {
            std::vector<glm::vec3>& curve = mTrackLines[curveIndex + k];
            unsigned int indexOfFirstSampleAfterReferenceLine = 0;
            bool foundFirstSampleAfterReferenceLine = false;
            // Move the entire curve to the left by xOffset
            for (int l = 0; l < curve.size(); ++l)
            {
               curve[l].x = curve[l].x - xOffset;
               if (!foundFirstSampleAfterReferenceLine && (curve[l].x > xPosOfOriginOfGraph))
               {
                  // Store the index of the first sample that's after the vertical reference line
                  indexOfFirstSampleAfterReferenceLine = l;
                  foundFirstSampleAfterReferenceLine = true;
               }
            }

            // If the first sample that's after the vertical reference line is sample 0, then we don't have to shift any samples to the right,
            // so we continue here
            if (indexOfFirstSampleAfterReferenceLine == 0)
            {
               continue;
            }

            // If the first sample that's after the vertical reference line is odd, then we use to the next one, since odd samples are always the
            // end points of all the little line segments
            if (indexOfFirstSampleAfterReferenceLine % 2 != 0)
            {
               ++indexOfFirstSampleAfterReferenceLine;
            }

            // Extract all the samples that are to the left of the vertical reference line into a separate vector
            std::vector<glm::vec3> frontOfCurve(curve.begin(), curve.begin() + indexOfFirstSampleAfterReferenceLine);
            curve.erase(curve.begin(), curve.begin() + indexOfFirstSampleAfterReferenceLine);

            // Shift all the samples that are to the left of the vertical reference line to the tail of the curve
            float xOffsetOfLastSample = curve[curve.size() - 1].x - frontOfCurve[0].x;
            for (int l = 0; l < frontOfCurve.size(); ++l)
            {
               frontOfCurve[l].x = frontOfCurve[l].x + xOffsetOfLastSample;
            }

            // Add the tail to the curve
            curve.insert(curve.end(), frontOfCurve.begin(), frontOfCurve.end());

            // Upload the updated curve to the GPU
            glBindVertexArray(mTrackLinesVAOs[curveIndex + k]);
            glBindBuffer(GL_ARRAY_BUFFER, mTrackLinesVBOs[curveIndex + k]);
            glBufferSubData(GL_ARRAY_BUFFER, 0, mTrackLines[curveIndex + k].size() * sizeof(glm::vec3), &(mTrackLines[curveIndex + k][0]));
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
         }

         ++trackIndex;
         curveIndex += 4;
      }
   }
}

void TrackVisualizer::render()
{
   mTrackShader->use(true);

   glm::vec3 eye    = glm::vec3(0.0f, 0.0f, 5.0f);
   glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);
   glm::vec3 up     = glm::vec3(0.0f, 1.0f, 0.0f);
   glm::mat4 view   = glm::lookAt(eye, center, up);

   float left   = 0.0f;
   float right  = mWidthOfGraphSpace * (1280.0f / 720.0f);
   float bottom = 0.0f;
   float top    = mWidthOfGraphSpace;
   float near   = 0.001f;
   float far    = 10.0f;
   glm::mat4 projection = glm::ortho(left, right, bottom, top, near, far);
   mTrackShader->setUniformMat4("projectionView", projection * view);

   mTrackShader->setUniformVec3("color", glm::vec3(1.0f, 1.0f, 1.0f));
   glBindVertexArray(mReferenceLinesVAO);
   glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(mReferenceLines.size()));
   glBindVertexArray(0);

   //mTrackShader->setUniformVec3("color", glm::vec3(0.0f, 1.0f, 0.0f));
   //glBindVertexArray(mEmptyLinesVAO);
   //glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(mEmptyLines.size()));
   //glBindVertexArray(0);

   for (int i = 0; i < mNumCurves; ++i)
   {
      mTrackShader->setUniformVec3("color", mTrackLinesColorPalette[i % 4]);
      glBindVertexArray(mTrackLinesVAOs[i]);
      glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(mTrackLines[i].size()));
      glBindVertexArray(0);
   }

   mTrackShader->use(false);
}

void TrackVisualizer::initializeReferenceLines()
{
   int trackIndex = 0;
   for (int j = 0; j < mNumTiles; ++j)
   {
      float yPosOfOriginOfGraph = (mTileHeight * j) + (mTileVerticalOffset / 2.0f);

      for (int i = 0; i < mNumTiles; ++i)
      {
         float xPosOfOriginOfGraph = (mTileWidth * i) + (mTileHorizontalOffset / 2.0f);

         if (trackIndex >= mNumGraphs)
         {
            // Uncomment this line if you don't want to see reference lines for empty graphs
            //break;

            // Bottom to top diagonal
            mEmptyLines.push_back(glm::vec3(xPosOfOriginOfGraph, yPosOfOriginOfGraph, 4.0f));
            mEmptyLines.push_back(glm::vec3(xPosOfOriginOfGraph + mGraphWidth, yPosOfOriginOfGraph + mGraphHeight, 4.0f));

            // Top to bottom diagonal
            mEmptyLines.push_back(glm::vec3(xPosOfOriginOfGraph, yPosOfOriginOfGraph + mGraphHeight, 4.0f));
            mEmptyLines.push_back(glm::vec3(xPosOfOriginOfGraph + mGraphWidth, yPosOfOriginOfGraph, 4.0f));
         }

         // Y axis (left)
         mReferenceLines.push_back(glm::vec3(xPosOfOriginOfGraph, yPosOfOriginOfGraph, 4.0f));
         mReferenceLines.push_back(glm::vec3(xPosOfOriginOfGraph, yPosOfOriginOfGraph + mGraphHeight, 4.0f));

         // X axis (bottom)
         mReferenceLines.push_back(glm::vec3(xPosOfOriginOfGraph, yPosOfOriginOfGraph, 4.0f));
         mReferenceLines.push_back(glm::vec3(xPosOfOriginOfGraph + mGraphWidth, yPosOfOriginOfGraph, 4.0f));

         // Uncomment these lines if you want each graph to be inside a box

         // Y axis (right)
         //mReferenceLines.push_back(glm::vec3(xPosOfOriginOfGraph + mGraphWidth, yPosOfOriginOfGraph, 4.0f));
         //mReferenceLines.push_back(glm::vec3(xPosOfOriginOfGraph + mGraphWidth, yPosOfOriginOfGraph + mGraphHeight, 4.0f));

         // X axis (top)
         //mReferenceLines.push_back(glm::vec3(xPosOfOriginOfGraph, yPosOfOriginOfGraph + mGraphHeight, 4.0f));
         //mReferenceLines.push_back(glm::vec3(xPosOfOriginOfGraph + mGraphWidth, yPosOfOriginOfGraph + mGraphHeight, 4.0f));

         ++trackIndex;
      }
   }
}

void TrackVisualizer::initializeTrackLines()
{
   mTrackLines.resize(mNumCurves);
   unsigned int trackIndex = 0;
   unsigned int curveIndex = 0;
   for (int j = 0; j < mNumTiles; ++j)
   {
      float yPosOfOriginOfGraph = (mTileHeight * j) + (mTileVerticalOffset / 2.0f);

      for (int i = 0; i < mNumTiles; ++i)
      {
         if (trackIndex >= mNumGraphs)
         {
            break;
         }

         float xPosOfOriginOfGraph = (mTileWidth * i) + (mTileHorizontalOffset / 2.0f);

         float trackDuration = mTracks[trackIndex].GetEndTime() - mTracks[trackIndex].GetStartTime();

         glm::vec4 minSamples = glm::vec4(std::numeric_limits<float>::max());
         glm::vec4 maxSamples = glm::vec4(std::numeric_limits<float>::lowest());
         for (int sampleIndex = 0; sampleIndex < 600; ++sampleIndex)
         {
            float sampleIndexNormalized = static_cast<float>(sampleIndex) / 599.0f;

            Q::quat sample = mTracks[trackIndex].Sample(sampleIndexNormalized * trackDuration, false);

            if (sample.x < minSamples.x) minSamples.x = sample.x;
            if (sample.y < minSamples.y) minSamples.y = sample.y;
            if (sample.z < minSamples.z) minSamples.z = sample.z;
            if (sample.w < minSamples.w) minSamples.w = sample.w;
            if (sample.x > maxSamples.x) maxSamples.x = sample.x;
            if (sample.y > maxSamples.y) maxSamples.y = sample.y;
            if (sample.z > maxSamples.z) maxSamples.z = sample.z;
            if (sample.w > maxSamples.w) maxSamples.w = sample.w;
         }

         for (int sampleIndex = 1; sampleIndex < 600; ++sampleIndex)
         {
            float currSampleIndexNormalized = static_cast<float>(sampleIndex - 1) / 599.0f;
            float nextSampleIndexNormalized = static_cast<float>(sampleIndex) / 599.0f;

            float currX = xPosOfOriginOfGraph + (currSampleIndexNormalized * mGraphWidth);
            float nextX = xPosOfOriginOfGraph + (nextSampleIndexNormalized * mGraphWidth);

            Q::quat currSampleQuat = mTracks[trackIndex].Sample(currSampleIndexNormalized * trackDuration, false);
            Q::quat nextSampleQuat = mTracks[trackIndex].Sample(nextSampleIndexNormalized * trackDuration, false);

            glm::vec4 currSample = glm::vec4(currSampleQuat.x, currSampleQuat.y, currSampleQuat.z, currSampleQuat.w);
            glm::vec4 nextSample = glm::vec4(nextSampleQuat.x, nextSampleQuat.y, nextSampleQuat.z, nextSampleQuat.w);

            glm::vec4 currSampleNormalized, nextSampleNormalized;
            for (int k = 0; k < 4; ++k)
            {
               currSampleNormalized[k] = yPosOfOriginOfGraph + (((currSample[k] - minSamples[k]) / (maxSamples[k] - minSamples[k])) * mGraphHeight);
               nextSampleNormalized[k] = yPosOfOriginOfGraph + (((nextSample[k] - minSamples[k]) / (maxSamples[k] - minSamples[k])) * mGraphHeight);
            }

            mTrackLines[curveIndex].push_back(glm::vec3(currX, currSampleNormalized.x, 3.0f));
            mTrackLines[curveIndex].push_back(glm::vec3(nextX, nextSampleNormalized.x, 3.0f));

            mTrackLines[curveIndex + 1].push_back(glm::vec3(currX, currSampleNormalized.y, 2.0f));
            mTrackLines[curveIndex + 1].push_back(glm::vec3(nextX, nextSampleNormalized.y, 2.0f));

            mTrackLines[curveIndex + 2].push_back(glm::vec3(currX, currSampleNormalized.z, 1.0f));
            mTrackLines[curveIndex + 2].push_back(glm::vec3(nextX, nextSampleNormalized.z, 1.0f));

            mTrackLines[curveIndex + 3].push_back(glm::vec3(currX, currSampleNormalized.w, 0.0f));
            mTrackLines[curveIndex + 3].push_back(glm::vec3(nextX, nextSampleNormalized.w, 0.0f));
         }

         ++trackIndex;
         curveIndex += 4;
      }
   }
}

void TrackVisualizer::deleteBuffers()
{
   glDeleteVertexArrays(1, &mReferenceLinesVAO);
   glDeleteBuffers(1, &mReferenceLinesVBO);

   glDeleteVertexArrays(1, &mEmptyLinesVAO);
   glDeleteBuffers(1, &mEmptyLinesVBO);

   if (mTrackLinesVAOs.size() > 0)
   {
      glDeleteVertexArrays(static_cast<GLsizei>(mTrackLinesVAOs.size()), &(mTrackLinesVAOs[0]));
   }

   if (mTrackLinesVBOs.size() > 0)
   {
      glDeleteBuffers(static_cast<GLsizei>(mTrackLinesVBOs.size()), &(mTrackLinesVBOs[0]));
   }
}
