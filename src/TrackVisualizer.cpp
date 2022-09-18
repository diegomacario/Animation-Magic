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
   , mNumEmptyRows(0)
   , mNumEmptyTilesInIncompleteRow(0)
   , mTileWidth(0.0f)
   , mTileHeight(0.0f)
   , mTileHorizontalOffset(0.0f)
   , mTileVerticalOffset(0.0f)
   , mGraphWidth(0.0f)
   , mGraphHeight(0.0f)
   , mReferenceLinesVAO(0)
   , mReferenceLinesVBO(0)
   , mEmptyLinesVAO{0, 0, 0, 0}
   , mEmptyLinesVBO{0, 0, 0, 0}
   , mTrackLinesVAOs()
   , mTrackLinesVBOs()
   , mTracks()
   , mMinSamples()
   , mInverseSampleRanges()
   , mReferenceLines()
   , mEmptyLines()
   , mTrackLines()
   , mTrackLinesColorPalette{glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.65f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)}
   //, mSelectedTrackLinesColorPalette{glm::vec3(244.0f, 255.0f, 97.0f) / 255.0f, glm::vec3(168.0f, 255.0f, 62.0f) / 255.0f, glm::vec3(50.0f, 255.0f, 106.0f) / 255.0f, glm::vec3(50.0f, 255.0f, 106.0f) / 255.0f}
   , mSelectedTrackLinesColorPalette{glm::vec3(111, 231, 221) / 255.0f, glm::vec3(52, 144, 222) / 255.0f, glm::vec3(102, 57, 166) / 255.0f, glm::vec3(82, 18, 98) / 255.0f}
   , mInitialized(false)
   , mGraphLowerLeftCorners()
   , mGraphUpperRightCorners()
   , mIndexOfSelectedGraph(-1)
   , mRightMouseButtonWasPressed(false)
{
   mTrackShader = ResourceManager<Shader>().loadUnmanagedResource<ShaderLoader>("resources/shaders/graph.vert",
                                                                                "resources/shaders/graph.frag");

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

   mProjectionViewMatrix = projection * view;
   mInverseProjectionViewMatrix = glm::inverse(mProjectionViewMatrix);
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
      mMinSamples.clear();
      mInverseSampleRanges.clear();
      mReferenceLines.clear();
      mEmptyLines.clear();
      mTrackLines.clear();
      deleteBuffers();
      mReferenceLinesVAO = 0;
      mReferenceLinesVBO = 0;
      for (int i = 0; i < 4; ++i)
      {
         mEmptyLinesVAO[i] = 0;
         mEmptyLinesVBO[i] = 0;
      }
      mTrackLinesVAOs.clear();
      mTrackLinesVBOs.clear();
      mGraphLowerLeftCorners.clear();
      mGraphUpperRightCorners.clear();
      mIndexOfSelectedGraph = -1;
   }

   // Determine which tracks are valid and store their min samples and inverse sample ranges
   for (int trackIndex = 0; trackIndex < tracks.size(); ++trackIndex)
   {
      FastQuaternionTrack& rotationTrack = tracks[trackIndex].GetRotationTrack();

      float trackDuration = rotationTrack.GetEndTime() - rotationTrack.GetStartTime();

      glm::vec4 minSamples = glm::vec4(std::numeric_limits<float>::max());
      glm::vec4 maxSamples = glm::vec4(std::numeric_limits<float>::lowest());
      for (int sampleIndex = 0; sampleIndex < 600; ++sampleIndex)
      {
         float sampleIndexNormalized = static_cast<float>(sampleIndex) / 599.0f;

         Q::quat sample = rotationTrack.Sample(sampleIndexNormalized * trackDuration, false);

         if (sample.x < minSamples.x) minSamples.x = sample.x;
         if (sample.y < minSamples.y) minSamples.y = sample.y;
         if (sample.z < minSamples.z) minSamples.z = sample.z;
         if (sample.w < minSamples.w) minSamples.w = sample.w;
         if (sample.x > maxSamples.x) maxSamples.x = sample.x;
         if (sample.y > maxSamples.y) maxSamples.y = sample.y;
         if (sample.z > maxSamples.z) maxSamples.z = sample.z;
         if (sample.w > maxSamples.w) maxSamples.w = sample.w;
      }

      glm::vec4 inverseSampleRange;
      inverseSampleRange.x = 1.0f / (maxSamples.x - minSamples.x);
      inverseSampleRange.y = 1.0f / (maxSamples.y - minSamples.y);
      inverseSampleRange.z = 1.0f / (maxSamples.z - minSamples.z);
      inverseSampleRange.w = 1.0f / (maxSamples.w - minSamples.w);

      if (std::isinf(inverseSampleRange.x) && std::isinf(inverseSampleRange.y) && std::isinf(inverseSampleRange.z) && std::isinf(inverseSampleRange.w))
      {
         //std::cout << "Graph " << trackIndex << " is invisible!" << '\n';
      }
      else
      {
         if (std::isinf(inverseSampleRange.x)) inverseSampleRange.x = 1.0f;
         if (std::isinf(inverseSampleRange.y)) inverseSampleRange.y = 1.0f;
         if (std::isinf(inverseSampleRange.z)) inverseSampleRange.z = 1.0f;
         if (std::isinf(inverseSampleRange.w)) inverseSampleRange.w = 1.0f;

         mTracks.push_back(rotationTrack);
         mMinSamples.push_back(minSamples);
         mInverseSampleRanges.push_back(inverseSampleRange);
      }
   }

   mNumGraphs = static_cast<unsigned int>(mTracks.size());
   mNumCurves = mNumGraphs * 4;
   mNumTiles  = static_cast<unsigned int>(glm::sqrt(mNumGraphs));
   while (mNumGraphs > (mNumTiles * mNumTiles))
   {
      ++mNumTiles;
   }

   // If there are empty rows, we don't render them
   mNumEmptyRows = 0;
   int numEmptyTiles = (mNumTiles * mNumTiles) - mNumGraphs;
   while (numEmptyTiles > 0)
   {
      numEmptyTiles -= mNumTiles;
      if (numEmptyTiles >= 0)
      {
         ++mNumEmptyRows;
      }
   }

   // If there is an incomplete row, we fill the empty tiles in it with repeated graphs
   mNumEmptyTilesInIncompleteRow = (mNumTiles * mNumTiles) - mNumGraphs - (mNumEmptyRows * mNumTiles);
   mNumGraphs += mNumEmptyTilesInIncompleteRow;
   mNumCurves += (mNumEmptyTilesInIncompleteRow * 4);

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

   glGenVertexArrays(4, mEmptyLinesVAO);
   glGenBuffers(4, mEmptyLinesVBO);

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

   for (int i = 0; i < 4; ++i)
   {
      if (mEmptyLines[i].size() != 0)
      {
         glBindVertexArray(mEmptyLinesVAO[i]);
         glBindBuffer(GL_ARRAY_BUFFER, mEmptyLinesVBO[i]);
         glBufferData(GL_ARRAY_BUFFER, mEmptyLines[i].size() * sizeof(glm::vec3), &mEmptyLines[i][0], GL_STATIC_DRAW);
         glBindBuffer(GL_ARRAY_BUFFER, 0);
         glBindVertexArray(0);
      }
   }

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

   for (int i = 0; i < 4; ++i)
   {
      glBindVertexArray(mEmptyLinesVAO[i]);
      glBindBuffer(GL_ARRAY_BUFFER, mEmptyLinesVBO[i]);
      glEnableVertexAttribArray(posAttribLocation);
      glVertexAttribPointer(posAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindVertexArray(0);
   }

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

void TrackVisualizer::update(float deltaTime, float playbackSpeed, const std::shared_ptr<Window>& window, bool fillEmptyTilesWithRepeatedGraphs, bool graphsAreVisible)
{
   float speedFactor = (playbackSpeed == 0.0f) ? 0.0f : 1.0f + (4.0f * playbackSpeed);
   float xOffset = deltaTime * speedFactor;
   unsigned int trackIndex = 0;
   unsigned int curveIndex = 0;
   for (int j = 0; j < mNumTiles; ++j)
   {
      if (j > (mNumTiles - mNumEmptyRows - 1))
      {
         // Skip empty rows
         break;
      }

      for (int i = 0; i < mNumTiles; ++i)
      {
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

   if (!graphsAreVisible)
   {
      return;
   }

   bool rightMouseButtonIsPressed = window->isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT);
   if (!mRightMouseButtonWasPressed && rightMouseButtonIsPressed)
   {
      float  devicePixelRatio = window->getDevicePixelRatio();
      double cursorXPos       = window->getCursorXPos();
      double cursorYPos       = window->getCursorYPos();
   #ifndef __EMSCRIPTEN__
      cursorXPos *= devicePixelRatio;
      cursorYPos *= devicePixelRatio;
   #endif

      double x = ((2.0 * (cursorXPos - window->getLowerLeftCornerOfViewportXInPix())) / window->getWidthOfViewportInPix()) - 1.0f;
      double y = ((2.0 * (cursorYPos - window->getLowerLeftCornerOfViewportYInPix())) / window->getHeightOfViewportInPix()) - 1.0f;

      glm::vec4 screenPos(x, -y, -1.0f, 1.0f);
      glm::vec4 worldPos = mInverseProjectionViewMatrix * screenPos;

      int newIndexOfSelectedGraph = -1;
      for (int i = 0; i < mGraphLowerLeftCorners.size(); ++i)
      {
         if (worldPos.x >= mGraphLowerLeftCorners[i].x &&
             worldPos.x <= mGraphUpperRightCorners[i].x &&
             worldPos.y >= mGraphLowerLeftCorners[i].y &&
             worldPos.y <= mGraphUpperRightCorners[i].y)
         {
            newIndexOfSelectedGraph = i;
            break;
         }
      }

      if (newIndexOfSelectedGraph == mIndexOfSelectedGraph)
      {
         // Clear the selection when an already selected graph is clicked
         mIndexOfSelectedGraph = -1;
      }
      else
      {
         mIndexOfSelectedGraph = newIndexOfSelectedGraph;
      }

      mRightMouseButtonWasPressed = true;
   }
   else if (!rightMouseButtonIsPressed)
   {
      mRightMouseButtonWasPressed = false;
   }

   // If a repeated graph is selected and the "fill empty tiles with repeated graphs" option is unchecked,
   // clear the selection
   if ((mIndexOfSelectedGraph >= static_cast<int>(mNumGraphs - mNumEmptyTilesInIncompleteRow)) &&
       !fillEmptyTilesWithRepeatedGraphs)
   {
      mIndexOfSelectedGraph = -1;
   }
}

void TrackVisualizer::render(bool fillEmptyTilesWithRepeatedGraphs)
{
   mTrackShader->use(true);

   mTrackShader->setUniformMat4("projectionView", mProjectionViewMatrix);

   mTrackShader->setUniformVec3("color", glm::vec3(1.0f, 1.0f, 1.0f));
   glBindVertexArray(mReferenceLinesVAO);
   glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(mReferenceLines.size()));
   glBindVertexArray(0);

   if (!fillEmptyTilesWithRepeatedGraphs)
   {
      for (int i = 0; i < 4; ++i)
      {
         mTrackShader->setUniformVec3("color", mTrackLinesColorPalette[3 - (i % 4)]);
         glBindVertexArray(mEmptyLinesVAO[i]);
         glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(mEmptyLines[i].size()));
         glBindVertexArray(0);
      }
   }

   int numCurvesToRender = fillEmptyTilesWithRepeatedGraphs ? mNumCurves : mNumCurves - (mNumEmptyTilesInIncompleteRow * 4);
   int indexOfFirstSelectedCurve = mIndexOfSelectedGraph * 4;
   for (int i = 0; i < numCurvesToRender; ++i)
   {
      if ((i >= indexOfFirstSelectedCurve) && (i < (indexOfFirstSelectedCurve + 4)))
      {
         mTrackShader->setUniformVec3("color", mSelectedTrackLinesColorPalette[i % 4]);
      }
      else
      {
         mTrackShader->setUniformVec3("color", mTrackLinesColorPalette[i % 4]);
      }

      glBindVertexArray(mTrackLinesVAOs[i]);
      glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(mTrackLines[i].size()));
      glBindVertexArray(0);
   }

   mTrackShader->use(false);
}

int TrackVisualizer::getIndexOfSelectedGraph() const
{
   if (mIndexOfSelectedGraph == -1)
   {
      return mIndexOfSelectedGraph;
   }

   return (mIndexOfSelectedGraph % (mNumGraphs - mNumEmptyTilesInIncompleteRow));
}

void TrackVisualizer::initializeReferenceLines()
{
   mEmptyLines.resize(4);
   unsigned int trackIndex = 0;
   unsigned int emptyLineIndex = 0;
   for (int j = 0; j < mNumTiles; ++j)
   {
      if (j > (mNumTiles - mNumEmptyRows - 1))
      {
         // Skip empty rows
         break;
      }

      float emptyRowOffset = (mNumEmptyRows * mTileHeight * 0.5f);
      float yPosOfOriginOfGraph = (mTileHeight * j) + (mTileVerticalOffset / 2.0f) + emptyRowOffset;

      for (int i = 0; i < mNumTiles; ++i)
      {
         float xPosOfOriginOfGraph = (mTileWidth * i) + (mTileHorizontalOffset / 2.0f);

         if (trackIndex >= (mNumGraphs - mNumEmptyTilesInIncompleteRow))
         {
            // Horizontal line
            mEmptyLines[emptyLineIndex].push_back(glm::vec3(xPosOfOriginOfGraph, yPosOfOriginOfGraph + mGraphHeight * 0.5f, 4.0f));
            mEmptyLines[emptyLineIndex].push_back(glm::vec3(xPosOfOriginOfGraph + mGraphWidth, yPosOfOriginOfGraph + mGraphHeight * 0.5f, 4.0f));
            emptyLineIndex = (emptyLineIndex + 1) % 4;
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

         // The corners are used to determine which graph the mouse is hovering over
         mGraphLowerLeftCorners.push_back(glm::vec2(xPosOfOriginOfGraph, yPosOfOriginOfGraph));
         mGraphUpperRightCorners.push_back(glm::vec2(xPosOfOriginOfGraph + mGraphWidth, yPosOfOriginOfGraph + mGraphHeight));

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
      if (j > (mNumTiles - mNumEmptyRows - 1))
      {
         // Skip empty rows
         break;
      }

      float emptyRowOffset = (mNumEmptyRows * mTileHeight * 0.5f);
      float yPosOfOriginOfGraph = (mTileHeight * j) + (mTileVerticalOffset / 2.0f) + emptyRowOffset;

      for (int i = 0; i < mNumTiles; ++i)
      {
         // We wrap the track index to initialize the repeated graphs that fill the incomplete row
         unsigned int wrappedTrackIndex = trackIndex % (mNumGraphs - mNumEmptyTilesInIncompleteRow);

         float xPosOfOriginOfGraph = (mTileWidth * i) + (mTileHorizontalOffset / 2.0f);

         float trackDuration = mTracks[wrappedTrackIndex].GetEndTime() - mTracks[wrappedTrackIndex].GetStartTime();

         for (int sampleIndex = 1; sampleIndex < 600; ++sampleIndex)
         {
            float currSampleIndexNormalized = static_cast<float>(sampleIndex - 1) / 599.0f;
            float nextSampleIndexNormalized = static_cast<float>(sampleIndex) / 599.0f;

            float currX = xPosOfOriginOfGraph + (currSampleIndexNormalized * mGraphWidth);
            float nextX = xPosOfOriginOfGraph + (nextSampleIndexNormalized * mGraphWidth);

            Q::quat currSampleQuat = mTracks[wrappedTrackIndex].Sample(currSampleIndexNormalized * trackDuration, false);
            Q::quat nextSampleQuat = mTracks[wrappedTrackIndex].Sample(nextSampleIndexNormalized * trackDuration, false);

            glm::vec4 currSample = glm::vec4(currSampleQuat.x, currSampleQuat.y, currSampleQuat.z, currSampleQuat.w);
            glm::vec4 nextSample = glm::vec4(nextSampleQuat.x, nextSampleQuat.y, nextSampleQuat.z, nextSampleQuat.w);

            glm::vec4 currSampleNormalized, nextSampleNormalized;
            for (int k = 0; k < 4; ++k)
            {
               currSampleNormalized[k] = yPosOfOriginOfGraph + (((currSample[k] - mMinSamples[wrappedTrackIndex][k]) * mInverseSampleRanges[wrappedTrackIndex][k]) * mGraphHeight);
               nextSampleNormalized[k] = yPosOfOriginOfGraph + (((nextSample[k] - mMinSamples[wrappedTrackIndex][k]) * mInverseSampleRanges[wrappedTrackIndex][k]) * mGraphHeight);
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

   glDeleteVertexArrays(4, mEmptyLinesVAO);
   glDeleteBuffers(4, mEmptyLinesVBO);

   if (mTrackLinesVAOs.size() > 0)
   {
      glDeleteVertexArrays(static_cast<GLsizei>(mTrackLinesVAOs.size()), &(mTrackLinesVAOs[0]));
   }

   if (mTrackLinesVBOs.size() > 0)
   {
      glDeleteBuffers(static_cast<GLsizei>(mTrackLinesVBOs.size()), &(mTrackLinesVBOs[0]));
   }
}
