#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

#include "AnimatedMesh.h"
#include "Transform.h"

AnimatedMesh::AnimatedMesh()
{
   glGenVertexArrays(1, &mVAO);
   glGenBuffers(5, &mVBOs[0]);
   glGenBuffers(1, &mEBO);
}

AnimatedMesh::~AnimatedMesh()
{
   glDeleteVertexArrays(1, &mVAO);
   glDeleteBuffers(5, &mVBOs[0]);
   glDeleteBuffers(1, &mEBO);
}

AnimatedMesh::AnimatedMesh(AnimatedMesh&& rhs) noexcept
   : mPositions(std::move(rhs.mPositions))
   , mNormals(std::move(rhs.mNormals))
   , mTexCoords(std::move(rhs.mTexCoords))
   , mWeights(std::move(rhs.mWeights))
   , mInfluences(std::move(rhs.mInfluences))
   , mIndices(std::move(rhs.mIndices))
   , mNumIndices(std::exchange(rhs.mNumIndices, 0))
   , mVAO(std::exchange(rhs.mVAO, 0))
   , mVBOs(std::exchange(rhs.mVBOs, std::array<unsigned int, 5>()))
   , mEBO(std::exchange(rhs.mEBO, 0))
{

}

AnimatedMesh& AnimatedMesh::operator=(AnimatedMesh&& rhs) noexcept
{
   mPositions  = std::move(rhs.mPositions);
   mNormals    = std::move(rhs.mNormals);
   mTexCoords  = std::move(rhs.mTexCoords);
   mWeights    = std::move(rhs.mWeights);
   mInfluences = std::move(rhs.mInfluences);
   mIndices    = std::move(rhs.mIndices);
   mNumIndices = std::exchange(rhs.mNumIndices, 0);
   mVAO        = std::exchange(rhs.mVAO, 0);
   mVBOs       = std::exchange(rhs.mVBOs, std::array<unsigned int, 5>());
   mEBO        = std::exchange(rhs.mEBO, 0);
   return *this;
}

// TODO: Experiment with GL_STATIC_DRAW, GL_STREAM_DRAW and GL_DYNAMIC_DRAW to see which is faster
void AnimatedMesh::LoadBuffers()
{
   glBindVertexArray(mVAO);

   // Load the mesh's data into the buffers

   // Positions
   glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBOTypes::positions]);
   glBufferData(GL_ARRAY_BUFFER, mPositions.size() * sizeof(glm::vec3), &mPositions[0], GL_STATIC_DRAW);
   // Normals
   glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBOTypes::normals]);
   glBufferData(GL_ARRAY_BUFFER, mNormals.size() * sizeof(glm::vec3), &mNormals[0], GL_STATIC_DRAW);
   // Texture coordinates
   glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBOTypes::texCoords]);
   glBufferData(GL_ARRAY_BUFFER, mTexCoords.size() * sizeof(glm::vec2), &mTexCoords[0], GL_STATIC_DRAW);
   // TODO: The checks below are necessary because this class currently represents animated and static meshes. It must be split
   // Weights
   if (mWeights.size() != 0)
   {
      glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBOTypes::weights]);
      glBufferData(GL_ARRAY_BUFFER, mWeights.size() * sizeof(glm::vec4), &mWeights[0], GL_STATIC_DRAW);
   }
   // Influences
   if (mInfluences.size() != 0)
   {
      glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBOTypes::influences]);
      glBufferData(GL_ARRAY_BUFFER, mInfluences.size() * sizeof(glm::ivec4), &mInfluences[0], GL_STATIC_DRAW);
   }

   glBindBuffer(GL_ARRAY_BUFFER, 0);

   // Indices
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndices.size() * sizeof(unsigned int), &mIndices[0], GL_STATIC_DRAW);

   // Unbind the VAO first, then the EBO
   glBindVertexArray(0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void AnimatedMesh::ConfigureVAO(int posAttribLocation,
                                int normalAttribLocation,
                                int texCoordsAttribLocation,
                                int weightsAttribLocation,
                                int influencesAttribLocation)
{
   glBindVertexArray(mVAO);

   // Set the vertex attribute pointers
   BindFloatAttribute(posAttribLocation,       mVBOs[VBOTypes::positions], 3);
   BindFloatAttribute(normalAttribLocation,    mVBOs[VBOTypes::normals], 3);
   BindFloatAttribute(texCoordsAttribLocation, mVBOs[VBOTypes::texCoords], 2);
   BindFloatAttribute(weightsAttribLocation,   mVBOs[VBOTypes::weights], 4);
   BindIntAttribute(influencesAttribLocation,  mVBOs[VBOTypes::influences], 4);

   glBindVertexArray(0);
}

void AnimatedMesh::UnconfigureVAO(int posAttribLocation,
                                  int normalAttribLocation,
                                  int texCoordsAttribLocation,
                                  int weightsAttribLocation,
                                  int influencesAttribLocation)
{
   glBindVertexArray(mVAO);

   // Unset the vertex attribute pointers
   UnbindAttribute(posAttribLocation,        mVBOs[VBOTypes::positions]);
   UnbindAttribute(normalAttribLocation,     mVBOs[VBOTypes::normals]);
   UnbindAttribute(texCoordsAttribLocation,  mVBOs[VBOTypes::texCoords]);
   UnbindAttribute(weightsAttribLocation,    mVBOs[VBOTypes::weights]);
   UnbindAttribute(influencesAttribLocation, mVBOs[VBOTypes::influences]);

   glBindVertexArray(0);
}

void AnimatedMesh::BindFloatAttribute(int attribLocation, unsigned int VBO, int numComponents)
{
   if (attribLocation >= 0)
   {
      glBindBuffer(GL_ARRAY_BUFFER, VBO);
      glEnableVertexAttribArray(attribLocation);
      glVertexAttribPointer(attribLocation, numComponents, GL_FLOAT, GL_FALSE, 0, (void*)0);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
   }
}

void AnimatedMesh::BindIntAttribute(int attribLocation, unsigned int VBO, int numComponents)
{
   if (attribLocation >= 0)
   {
      glBindBuffer(GL_ARRAY_BUFFER, VBO);
      glEnableVertexAttribArray(attribLocation);
      glVertexAttribIPointer(attribLocation, numComponents, GL_INT, 0, (void*)0);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
   }
}

void AnimatedMesh::UnbindAttribute(int attribLocation, unsigned int VBO)
{
   if (attribLocation >= 0)
   {
      glBindBuffer(GL_ARRAY_BUFFER, VBO);
      glDisableVertexAttribArray(attribLocation);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
   }
}

// TODO: GL_TRIANGLES shouldn't be hardcoded here
//       Can we load that from the GLTF file?
void AnimatedMesh::Render()
{
   glBindVertexArray(mVAO);

   if (mIndices.size() > 0)
   {
      // TODO: Use mNumIndices here
      glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(mIndices.size()), GL_UNSIGNED_INT, 0);
   }
   else
   {
      glDrawArrays(GL_TRIANGLES, 0, static_cast<unsigned int>(mPositions.size()));
   }

   glBindVertexArray(0);
}

// TODO: GL_TRIANGLES shouldn't be hardcoded here
//       Can we load that from the GLTF file?
void AnimatedMesh::RenderInstanced(unsigned int numInstances)
{
   glBindVertexArray(mVAO);

   if (mIndices.size() > 0)
   {
      // TODO: Use mNumIndices here
      glDrawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(mIndices.size()), GL_UNSIGNED_INT, 0, numInstances);
   }
   else
   {
      glDrawArraysInstanced(GL_TRIANGLES, 0, static_cast<unsigned int>(mPositions.size()), numInstances);
   }

   glBindVertexArray(0);
}

/*
   Deforming a mesh to match an animated pose is called "skinning"
   To skin a mesh, the mesh must be "rigged", which means that each of its vertices must have:

   - One or more joints that affect it
   - Weights that determine the influence of the joints that affect it (note that all the weights must add up to 1.0)

   Skinning a mesh in which each vertex is only affected by one joint is called "rigid skinning"
   Skinning a mesh in which each vertex is affected by more than one joint is called "smooth skinning" or "linear blend skinning"

   For a mesh that isn't animated, the vertex transformation pipeline looks like this:

   - All the vertices are initially in model space
   - Model matrix      * Model space vertex = World space vertex
   - View matrix       * World space vertex = View space vertex
   - Projection matrix * View space vertex  = Vertex in NDC

   For an animated mesh in which each vertex is only affected by one joint, the vertex transformation pipeline looks like this:

   - All the vertices are initially in model space in the bind pose
   - Global inverse bind pose transform of joint that affects vertex * Model space vertex = Skin space vertex  -> New step!
   - Global animated pose transform of joint that affects vertex     * Skin space vertex  = Model space vertex -> New step!
   - Model matrix                                                    * Model space vertex = World space vertex
   - View matrix                                                     * World space vertex = View space vertex
   - Projection matrix                                               * View space vertex  = Vertex in NDC

   The diagrams below illustrate the two new steps:

          *   ------> The global bind pose transform of this joint places the forearm here
          |   |
   *--*--*|*--*--*
          |
          *
         / \
        /   \

          *
          |
   *--*--*|*--*  *
          |
          *
         / \
        / | \
          ----------> The global inverse bind pose transform of the same joint places the forearm in skin space

          *     *
          |    / ---> The global animated pose transform of the same joint places the forearm in this new position
   *--*--*|*--*
          |
          *
         / \
        /   \

   As you can see, the skin space is just a neutral intermediate step that allows us to move from one animated pose to the next

   For a vertex that is affected by more than one joint, we simply perform the steps described above independently with each joint,
   and then we use the weights of the vertex to blend the resulting skinned vertices into a final vertex
*/

void AnimatedMesh::SkinMeshOnTheCPUUsingMatrices(Skeleton& skeleton, Pose& animatedPose)
{
   // If the mesh doesn't have any vertices we can't skin it
   unsigned int numVertices = static_cast<unsigned int>(mPositions.size());
   if (numVertices == 0)
   {
      return;
   }

   // Resize the containers that will store the skinned positions and normals
   mSkinnedPositions.resize(numVertices);
   mSkinnedNormals.resize(numVertices);

   // Get the palettes of the inverse bind pose and the animated pose
   // Remember that a palette contains the global transform matrices of each joint
   const std::vector<glm::mat4>& invBindPosePalette = skeleton.GetInvBindPose();
   animatedPose.GetMatrixPalette(mAnimatedPosePalette);

   // Loop over the vertices of the mesh
   for (unsigned int vertexIndex = 0; vertexIndex < numVertices; ++vertexIndex)
   {
      // Get the influences and weights of the current vertex
      // The influences are the IDs of the joints that affect it
      const glm::ivec4& influencesOfCurrVertex = mInfluences[vertexIndex];
      const glm::vec4&  weightsOfCurrVertex    = mWeights[vertexIndex];

      // Calculate the skin matrix of each influencing joint independently
      // A skin matrix takes a vertex from the bind pose to skin space and then to the animated pose
      glm::mat4 skinMatrix0 = (mAnimatedPosePalette[influencesOfCurrVertex.x] * invBindPosePalette[influencesOfCurrVertex.x]);
      glm::mat4 skinMatrix1 = (mAnimatedPosePalette[influencesOfCurrVertex.y] * invBindPosePalette[influencesOfCurrVertex.y]);
      glm::mat4 skinMatrix2 = (mAnimatedPosePalette[influencesOfCurrVertex.z] * invBindPosePalette[influencesOfCurrVertex.z]);
      glm::mat4 skinMatrix3 = (mAnimatedPosePalette[influencesOfCurrVertex.w] * invBindPosePalette[influencesOfCurrVertex.w]);

      // Combine the skin matrices together using the weights
      glm::mat4 skinMatrix = (skinMatrix0 * weightsOfCurrVertex.x) +
                             (skinMatrix1 * weightsOfCurrVertex.y) +
                             (skinMatrix2 * weightsOfCurrVertex.z) +
                             (skinMatrix3 * weightsOfCurrVertex.w);

      // Calculate the skinned position and normal of the current vertex
      // Remember that the position is a point while the normal is a vector
      mSkinnedPositions[vertexIndex] = skinMatrix * glm::vec4(mPositions[vertexIndex], 1.0f);
      mSkinnedNormals[vertexIndex]   = skinMatrix * glm::vec4(mNormals[vertexIndex], 0.0f);
   }

   // TODO: Should I bind the VAO here?

   // Load the skinned positions and normals into the buffers

   // Positions
   glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBOTypes::positions]);
   glBufferSubData(GL_ARRAY_BUFFER, 0, mSkinnedPositions.size() * sizeof(glm::vec3), &mSkinnedPositions[0]);
   // Normals
   glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBOTypes::normals]);
   glBufferSubData(GL_ARRAY_BUFFER, 0, mSkinnedNormals.size() * sizeof(glm::vec3), &mSkinnedNormals[0]);

   glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void AnimatedMesh::SkinMeshOnTheCPUUsingTransforms(Skeleton& skeleton, Pose& animatedPose)
{
   // If the mesh doesn't have any vertices we can't skin it
   unsigned int numVertices = static_cast<unsigned int>(mPositions.size());
   if (numVertices == 0)
   {
      return;
   }

   // Resize the containers that will store the skinned positions and normals
   mSkinnedPositions.resize(numVertices);
   mSkinnedNormals.resize(numVertices);

   // Get the bind pose
   const Pose& bindPose = skeleton.GetBindPose();

   // Loop over the vertices of the mesh
   for (unsigned int vertexIndex = 0; vertexIndex < numVertices; ++vertexIndex)
   {
      // Get the influences and weights of the current vertex
      // The influences are the IDs of the joints that affect it
      const glm::ivec4& influencesOfCurrVertex = mInfluences[vertexIndex];
      const glm::vec4&  weightsOfCurrVertex    = mWeights[vertexIndex];

      // Calculate the skin transform of each influencing joint independently and use it to calculate the skinned positions and normals
      Transform skinTransform0   = combine(animatedPose.GetGlobalTransform(influencesOfCurrVertex.x), inverse(bindPose.GetGlobalTransform(influencesOfCurrVertex.x)));
      glm::vec3 skinnedPosition0 = transformPoint(skinTransform0, mPositions[vertexIndex]);
      glm::vec3 skinnedNormal0   = transformVector(skinTransform0, mNormals[vertexIndex]);

      Transform skinTransform1   = combine(animatedPose.GetGlobalTransform(influencesOfCurrVertex.y), inverse(bindPose.GetGlobalTransform(influencesOfCurrVertex.y)));
      glm::vec3 skinnedPosition1 = transformPoint(skinTransform1, mPositions[vertexIndex]);
      glm::vec3 skinnedNormal1   = transformVector(skinTransform1, mNormals[vertexIndex]);

      Transform skinTransform2   = combine(animatedPose.GetGlobalTransform(influencesOfCurrVertex.z), inverse(bindPose.GetGlobalTransform(influencesOfCurrVertex.z)));
      glm::vec3 skinnedPosition2 = transformPoint(skinTransform2, mPositions[vertexIndex]);
      glm::vec3 skinnedNormal2   = transformVector(skinTransform2, mNormals[vertexIndex]);

      Transform skinTransform3   = combine(animatedPose.GetGlobalTransform(influencesOfCurrVertex.w), inverse(bindPose.GetGlobalTransform(influencesOfCurrVertex.w)));
      glm::vec3 skinnedPosition3 = transformPoint(skinTransform3, mPositions[vertexIndex]);
      glm::vec3 skinnedNormal3   = transformVector(skinTransform3, mNormals[vertexIndex]);

      // Combine the skinned positions and normals using the weights to obtain the final skinned position and normal
      mSkinnedPositions[vertexIndex] = (skinnedPosition0 * weightsOfCurrVertex.x) +
                                       (skinnedPosition1 * weightsOfCurrVertex.y) +
                                       (skinnedPosition2 * weightsOfCurrVertex.z) +
                                       (skinnedPosition3 * weightsOfCurrVertex.w);
      mSkinnedNormals[vertexIndex] = (skinnedNormal0 * weightsOfCurrVertex.x) +
                                     (skinnedNormal1 * weightsOfCurrVertex.y) +
                                     (skinnedNormal2 * weightsOfCurrVertex.z) +
                                     (skinnedNormal3 * weightsOfCurrVertex.w);
   }

   // TODO: Should I bind the VAO here?

   // Load the skinned positions and normals into the buffers

   // Positions
   glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBOTypes::positions]);
   glBufferSubData(GL_ARRAY_BUFFER, 0, mSkinnedPositions.size() * sizeof(glm::vec3), &mSkinnedPositions[0]);
   // Normals
   glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBOTypes::normals]);
   glBufferSubData(GL_ARRAY_BUFFER, 0, mSkinnedNormals.size() * sizeof(glm::vec3), &mSkinnedNormals[0]);

   glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void AnimatedMesh::SkinMeshOnTheCPU(std::vector<glm::mat4>& skinMatrices)
{
   // If the mesh doesn't have any vertices we can't skin it
   unsigned int numVertices = static_cast<unsigned int>(mPositions.size());
   if (numVertices == 0)
   {
      return;
   }

   // Resize the containers that will store the skinned positions and normals
   mSkinnedPositions.resize(numVertices);
   mSkinnedNormals.resize(numVertices);

   // Loop over the vertices of the mesh
   for (unsigned int vertexIndex = 0; vertexIndex < numVertices; ++vertexIndex)
   {
      // Get the influences and weights of the current vertex
      // The influences are the IDs of the joints that affect it
      glm::ivec4& influencesOfCurrVertex = mInfluences[vertexIndex];
      glm::vec4&  weightsOfCurrVertex    = mWeights[vertexIndex];

      // Calculate the skinned positions
      glm::vec3 skinnedPosition0 = skinMatrices[influencesOfCurrVertex.x] * glm::vec4(mPositions[vertexIndex], 1.0f);
      glm::vec3 skinnedPosition1 = skinMatrices[influencesOfCurrVertex.y] * glm::vec4(mPositions[vertexIndex], 1.0f);
      glm::vec3 skinnedPosition2 = skinMatrices[influencesOfCurrVertex.z] * glm::vec4(mPositions[vertexIndex], 1.0f);
      glm::vec3 skinnedPosition3 = skinMatrices[influencesOfCurrVertex.w] * glm::vec4(mPositions[vertexIndex], 1.0f);
      // Combine the skinned positions using the weights to obtain the final skinned position
      mSkinnedPositions[vertexIndex] = (skinnedPosition0 * weightsOfCurrVertex.x) +
                                       (skinnedPosition1 * weightsOfCurrVertex.y) +
                                       (skinnedPosition2 * weightsOfCurrVertex.z) +
                                       (skinnedPosition3 * weightsOfCurrVertex.w);

      // Calculate the skinned normals
      glm::vec3 skinnedNormal0 = skinMatrices[influencesOfCurrVertex.x] * glm::vec4(mNormals[vertexIndex], 0.0f);
      glm::vec3 skinnedNormal1 = skinMatrices[influencesOfCurrVertex.y] * glm::vec4(mNormals[vertexIndex], 0.0f);
      glm::vec3 skinnedNormal2 = skinMatrices[influencesOfCurrVertex.z] * glm::vec4(mNormals[vertexIndex], 0.0f);
      glm::vec3 skinnedNormal3 = skinMatrices[influencesOfCurrVertex.w] * glm::vec4(mNormals[vertexIndex], 0.0f);
      // Combine the skinned normals using the weights to obtain the final skinned normal
      mSkinnedNormals[vertexIndex] = (skinnedNormal0 * weightsOfCurrVertex.x) +
                                     (skinnedNormal1 * weightsOfCurrVertex.y) +
                                     (skinnedNormal2 * weightsOfCurrVertex.z) +
                                     (skinnedNormal3 * weightsOfCurrVertex.w);
   }

   // TODO: Should I bind the VAO here?

   // Load the skinned positions and normals into the buffers

   // Positions
   glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBOTypes::positions]);
   glBufferSubData(GL_ARRAY_BUFFER, 0, mSkinnedPositions.size() * sizeof(glm::vec3), &mSkinnedPositions[0]);
   // Normals
   glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBOTypes::normals]);
   glBufferSubData(GL_ARRAY_BUFFER, 0, mSkinnedNormals.size() * sizeof(glm::vec3), &mSkinnedNormals[0]);

   glBindBuffer(GL_ARRAY_BUFFER, 0);
}
