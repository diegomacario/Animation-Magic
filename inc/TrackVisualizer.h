#ifndef TRACK_VISUALIZER_H
#define TRACK_VISUALIZER_H

#include <memory>

#include "TransformTrack.h"
#include "window.h"
#include "shader.h"

class TrackVisualizer
{
public:

   TrackVisualizer();
   ~TrackVisualizer();

   void setTracks(std::vector<FastTransformTrack>& tracks);

   void update(float deltaTime, float playbackSpeed, const std::shared_ptr<Window>& window, bool fillEmptyTilesWithRepeatedGraphs, bool graphsAreVisible);

   void render(bool fillEmptyTilesWithRepeatedGraphs);

   int  getIndexOfSelectedGraph() const;

private:

   void initializeReferenceLines();

   void initializeTrackLines();

   void deleteBuffers();

   float                               mWidthOfGraphSpace;
   float                               mHeightOfGraphSpace;
   unsigned int                        mNumGraphs;
   unsigned int                        mNumCurves;
   unsigned int                        mNumTiles;
   unsigned int                        mNumEmptyRows;
   unsigned int                        mNumEmptyTilesInIncompleteRow;
   float                               mTileWidth;
   float                               mTileHeight;
   float                               mTileHorizontalOffset;
   float                               mTileVerticalOffset;
   float                               mGraphWidth;
   float                               mGraphHeight;

   unsigned int                        mReferenceLinesVAO;
   unsigned int                        mReferenceLinesVBO;

   unsigned int                        mEmptyLinesVAO[4];
   unsigned int                        mEmptyLinesVBO[4];

   std::vector<unsigned int>           mTrackLinesVAOs;
   std::vector<unsigned int>           mTrackLinesVBOs;

   std::shared_ptr<Shader>             mTrackShader;

   glm::mat4                           mProjectionViewMatrix;
   glm::mat4                           mInverseProjectionViewMatrix;

   std::vector<FastQuaternionTrack>    mTracks;
   std::vector<glm::vec4>              mMinSamples;
   std::vector<glm::vec4>              mInverseSampleRanges;
   std::vector<glm::vec3>              mReferenceLines;
   std::vector<std::vector<glm::vec3>> mEmptyLines;
   std::vector<std::vector<glm::vec3>> mTrackLines;

   glm::vec3                           mTrackLinesColorPalette[4];
   glm::vec3                           mSelectedTrackLinesColorPalette[4];

   bool                                mInitialized;

   std::vector<glm::vec2>              mGraphLowerLeftCorners;
   std::vector<glm::vec2>              mGraphUpperRightCorners;
   int                                 mIndexOfSelectedGraph;

   bool                                mRightMouseButtonWasPressed;
};

#endif
