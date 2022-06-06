#ifndef TRACK_VISUALIZER_H
#define TRACK_VISUALIZER_H

#include <memory>

#include "TransformTrack.h"
#include "shader.h"

class TrackVisualizer
{
public:

   TrackVisualizer();
   ~TrackVisualizer();

   void setTracks(std::vector<FastTransformTrack>& tracks);

   void update(float deltaTime, float playbackSpeed);

   void render();

private:

   void initializeReferenceLines();

   void initializeTrackLines();

   void deleteBuffers();

   float                               mWidthOfGraphSpace;
   float                               mHeightOfGraphSpace;
   unsigned int                        mNumGraphs;
   unsigned int                        mNumCurves;
   unsigned int                        mNumTiles;
   float                               mTileWidth;
   float                               mTileHeight;
   float                               mTileHorizontalOffset;
   float                               mTileVerticalOffset;
   float                               mGraphWidth;
   float                               mGraphHeight;

   unsigned int                        mReferenceLinesVAO;
   unsigned int                        mReferenceLinesVBO;

   unsigned int                        mEmptyLinesVAO;
   unsigned int                        mEmptyLinesVBO;

   std::vector<unsigned int>           mTrackLinesVAOs;
   std::vector<unsigned int>           mTrackLinesVBOs;

   std::shared_ptr<Shader>             mTrackShader;

   std::vector<FastQuaternionTrack>    mTracks;
   std::vector<glm::vec3>              mReferenceLines;
   std::vector<glm::vec3>              mEmptyLines;
   std::vector<std::vector<glm::vec3>> mTrackLines;

   glm::vec3                           mTrackLinesColorPalette[4];

   bool                                mInitialized;
};

#endif
