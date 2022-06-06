#pragma warning(disable : 26812)

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>

#include "GLTFLoader.h"
#include <iostream>
#include "Transform.h"
#include <algorithm>

namespace GLTFHelpers
{
   // A glTF file may contain an array of scenes and an array of nodes
   // Each scene may contain an array of indices of nodes
   // Each node may contain an array of indices of its children
   // The function below linearly searches for a specific node in the array of nodes and returns its index
   int GetNodeIndex(cgltf_node* target, cgltf_node* nodes, unsigned int numNodes)
   {
      if (target == nullptr)
      {
         return -1;
      }

      for (unsigned int nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex)
      {
         if (target == &nodes[nodeIndex])
         {
            return static_cast<int>(nodeIndex);
         }
      }

      return -1;
   }

   // A node may contain a local transform, which can be stored as:
   // - A column-major matrix array
   // - Separate position, rotation and scale arrays
   // The function below gets the local transform of a specific node
   Transform GetLocalTransformFromNode(const cgltf_node& node)
   {
      Transform localTransform;

      if (node.has_matrix)
      {
         localTransform = mat4ToTransform(glm::make_mat4(node.matrix));
      }

      if (node.has_translation)
      {
         localTransform.position = glm::vec3(node.translation[0], node.translation[1], node.translation[2]);
      }

      if (node.has_rotation)
      {
         localTransform.rotation = Q::quat(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]);
      }

      if (node.has_scale)
      {
         localTransform.scale = glm::vec3(node.scale[0], node.scale[1], node.scale[2]);
      }

      return localTransform;
   }

   // In a glTF file...
   // - A buffer contains data that is read from a file
   //   Properties: byteLength, uri
   // - A bufferView defines a segment of the buffer data
   //   Properties: buffer, byteOffset, byteLength, byteStride, target (optional OpenGL buffer target)
   // - An accessor defines how the data of a bufferView is interpreted
   //   Properties: bufferView, byteOffset, type (e.g. "VEC2"), componentType (e.g. GL_FLOAT), count (e.g. number of "VEC2"s in the bufferView), min, max
   // The function below reads the values of an accessor whose componentType must be GL_FLOAT
   // E.g. For an accessor with a type of "VEC2", a componentType of GL_FLOAT and a count of 32, componentCount should be equal to 2,
   //      which would result in 64 floats being read
   void GetFloatsFromAccessor(const cgltf_accessor& accessor, unsigned int componentCount, std::vector<float>& outValues)
   {
      outValues.resize(accessor.count * componentCount);

      for (cgltf_size i = 0; i < accessor.count; ++i)
      {
         cgltf_accessor_read_float(&accessor, i, &outValues[i * componentCount], componentCount);
      }
   }

   // A glTF file may contain an array of animations
   // Each animation consists of two elements:
   // - An array of channels
   // - An array of samplers
   // Each channel specifies:
   // - The target of the animation
   //   This usually refers to a node (by its index) and to a path, which is the name of the animated property
   //   (e.g. "translation", "rotation", "scale" or "weights")
   // - A sampler, which summarizes the animation data
   // Each sampler specifies:
   // - The index of the accessor that provides the input data,
   //   which are the times of the key frames of the animation
   // - The index of the accessor that provides the output data,
   //   which are the values for the animated property at the respective key frames
   // - The interpolation mode (e.g. "LINEAR", "STEP" or "CUBICSPLINE")
   // The function below creates a track from a channel
   template<typename T, int N>
   void GetTrackFromChannel(const cgltf_animation_channel& channel, Track<T, N>& outTrack)
   {
      // Get the sampler of the channel, which summarizes the animation data
      cgltf_animation_sampler& sampler = *channel.sampler;

      // Get the interpolation mode from the sampler
      bool interpolationModeIsCubic = false;
      Interpolation interpolation = Interpolation::Constant;
      if (channel.sampler->interpolation == cgltf_interpolation_type_linear)
      {
         interpolation = Interpolation::Linear;
      }
      else if (channel.sampler->interpolation == cgltf_interpolation_type_cubic_spline)
      {
         interpolationModeIsCubic = true;
         interpolation = Interpolation::Cubic;
      }

      // Store the interpolation mode in the track
      outTrack.SetInterpolation(interpolation);

      // Get the times of the key frames from the sampler
      std::vector<float> keyFrameTimes;
      GetFloatsFromAccessor(*sampler.input, 1, keyFrameTimes);

      // Get the animated values from the sampler
      // For a constant or linearly interpolated track,
      // these would be a sequence of vec3s if the track animates the position or scale,
      // or a sequence of quats (vec4s) if the track animates the rotation
      // For a cubically interpolated track, it would be the same as above,
      // except that input and output slopes would also be part of the sequence in this order:
      // | inSlope | value | outSlope | inSlope | value | outSlope |
      std::vector<float> keyFrameValues;
      GetFloatsFromAccessor(*sampler.output, N, keyFrameValues);

      unsigned int numFrames = (unsigned int)sampler.input->count;
      // For a constant or linearly interpolated track that animates the position or scale, numFloatsPerFrame = 3 floats (vec3)
      // For a constant or linearly interpolated track that animates the rotation,          numFloatsPerFrame = 4 floats (quat == vec4)
      // For a cubically interpolated track that animates the position or scale,            numFloatsPerFrame = 3 floats (vec3) * 3 values (input slope, value, output slope) = 9 floats
      // For a cubically interpolated track that animates the rotation,                     numFloatsPerFrame = 4 floats (quat == vec4) * 3 values (input slope, value, output slope) = 12 floats
      unsigned int numFloatsPerFrame = static_cast<unsigned int>(keyFrameValues.size() / keyFrameTimes.size());

      // Loop over all the frames and store them in the track one by one
      outTrack.SetNumberOfFrames(numFrames);
      for (unsigned int frameIndex = 0; frameIndex < numFrames; ++frameIndex)
      {
         // For a sequence of floats like the one below, which belongs to a cubically interpolated track:
         // | inSlope0 | value0 | outSlope0 | inSlope1 | value1 | outSlope1 | inSlope2 | value2 | outSlope2 |
         // ^                               ^                               ^                               ^
         // |-------------------------------|-------------------------------|-------------------------------|
         // In each iteration:
         // - The firstIndexOfCurrKeyFrameFloats variable points to the first float of the current key frame (inSlope0[0], inSlope1[0], inSlope2[0])
         // - The offsetIntoCurrKeyFrameFloats variable is used to read the floats of the current key frame
         // Note that this also works for the sequences of floats of constant or linearly interpolated tracks, like the one below:
         // | value0 | value1 | value2 |
         // ^        ^        ^        ^
         // |--------|--------|--------|
         int firstIndexOfCurrKeyFrameFloats = frameIndex * numFloatsPerFrame;
         int offsetIntoCurrKeyFrameFloats = 0;

         Frame<N>& frame = outTrack.GetFrame(frameIndex);

         // Store the time
         frame.mTime = keyFrameTimes[frameIndex];

         // Store the input slope if the track is cubically interpolated
         // Otherwise, set it to zero
         for (int component = 0; component < N; ++component)
         {
            frame.mInSlope[component] = interpolationModeIsCubic ? keyFrameValues[firstIndexOfCurrKeyFrameFloats + offsetIntoCurrKeyFrameFloats++] : 0.0f;
         }

         // Store the animated value
         for (int component = 0; component < N; ++component)
         {
            // The component variable loops over the components of the value (XYZ for position and scale or XYZW for rotation)
            // The offsetIntoCurrKeyFrameFloats variable loops over the same values in the sequence of floats
            frame.mValue[component] = keyFrameValues[firstIndexOfCurrKeyFrameFloats + offsetIntoCurrKeyFrameFloats++];
         }

         // Store the output slope if the track is cubically interpolated
         // Otherwise, set it to zero
         for (int component = 0; component < N; ++component)
         {
            frame.mOutSlope[component] = interpolationModeIsCubic ? keyFrameValues[firstIndexOfCurrKeyFrameFloats + offsetIntoCurrKeyFrameFloats++] : 0.0f;
         }
      }
   } 

   // A glTF file may contain an array of meshes
   // Each mesh may contain multiple mesh primitives, which refer to the geometry data that is required to render a mesh
   // Each mesh primitive consists of:
   // - A rendering mode (e.g. POINTS, LINES or TRIANGLES)
   // - A set of indices
   // - The attributes of the vertices
   // - The material that should be used for rendering
   // Each attribute is defined by mapping the attribute name (e.g. "POSITION", "NORMAL", etc.)
   // to the index of the accessor that contains the attribute data
   void StoreValuesOfAttributeInAnimatedMesh(cgltf_attribute& attribute, cgltf_skin* skin, cgltf_node* nodes, unsigned int nodeCount, AnimatedMesh& outMesh)
   {
      // Get the accessor of the attribute and its component count
      cgltf_accessor& accessor = *attribute.data;
      unsigned int componentCount = 0;
      if (accessor.type == cgltf_type_vec2)
      {
         componentCount = 2;
      }
      else if (accessor.type == cgltf_type_vec3)
      {
         componentCount = 3;
      }
      else if (accessor.type == cgltf_type_vec4)
      {
         componentCount = 4;
      }

      // Read the floats from the accessor
      std::vector<float> attributeFloats;
      GetFloatsFromAccessor(accessor, componentCount, attributeFloats);

      // Get the vectors that will store the attributes of the mesh that we are loading
      // In each call to this function we only fill one of these, since a cgltf_attribute only describes one attribute
      std::vector<glm::vec3>&  positions  = outMesh.GetPositions();
      std::vector<glm::vec3>&  normals    = outMesh.GetNormals();
      std::vector<glm::vec2>&  texCoords  = outMesh.GetTexCoords();
      std::vector<glm::ivec4>& influences = outMesh.GetInfluences();
      std::vector<glm::vec4>&  weights    = outMesh.GetWeights();

      // Loop over all the values of the accessor
      // Note that accessor.count is not equal to the number of floats in the accessor
      // It's equal to the number of attribute values (e.g, vec2s, vec3s, vec4s, etc.) in the accessor
      cgltf_attribute_type attributeType = attribute.type;
      unsigned int numAttributeValues = static_cast<unsigned int>(accessor.count);
      for (unsigned int attributeValueIndex = 0; attributeValueIndex < numAttributeValues; ++attributeValueIndex)
      {
         // Store the current attribute value in the correct vector of the mesh
         // TODO: This looks inefficient. I think it would be better to have separate for loops inside if-statements
         //       Since this function only loads one attribute type when it's called, it doesn't make sense to check
         //       the attribute type in each iteration of this loop
         int indexOfFirstFloatOfCurrAttribValue = attributeValueIndex * componentCount;
         switch (attributeType)
         {
         case cgltf_attribute_type_position:
            // Store a position vec3
            positions.push_back(glm::vec3(attributeFloats[indexOfFirstFloatOfCurrAttribValue + 0],
                                          attributeFloats[indexOfFirstFloatOfCurrAttribValue + 1],
                                          attributeFloats[indexOfFirstFloatOfCurrAttribValue + 2]));
            break;
         case cgltf_attribute_type_texcoord:
            // Store a texture coordinates vec2
            texCoords.push_back(glm::vec2(attributeFloats[indexOfFirstFloatOfCurrAttribValue + 0],
                                          attributeFloats[indexOfFirstFloatOfCurrAttribValue + 1]));
            break;
         case cgltf_attribute_type_weights:
            // Store a weights vec4
            weights.push_back(glm::vec4(attributeFloats[indexOfFirstFloatOfCurrAttribValue + 0],
                                        attributeFloats[indexOfFirstFloatOfCurrAttribValue + 1],
                                        attributeFloats[indexOfFirstFloatOfCurrAttribValue + 2],
                                        attributeFloats[indexOfFirstFloatOfCurrAttribValue + 3]));
            break;
         case cgltf_attribute_type_normal:
         {
            // Store a normal vec3
            glm::vec3 normal = glm::vec3(attributeFloats[indexOfFirstFloatOfCurrAttribValue + 0],
                                         attributeFloats[indexOfFirstFloatOfCurrAttribValue + 1],
                                         attributeFloats[indexOfFirstFloatOfCurrAttribValue + 2]);

            // TODO: Use a constant here and add an error message
            if (glm::length2(normal) < 0.000001f)
            {
               normal = glm::vec3(0, 1, 0);
            }

            normals.push_back(glm::normalize(normal));
         }
         break;
         case cgltf_attribute_type_joints:
         {
            // Store an influences ivec4
            // Remember that in this function we are processing a node that has a mesh and a skin,
            // so the indices we are storing below are indices into the array of nodes of the skin,
            // not indices into the array of nodes of the glTF file, so we need to convert them from being
            // skin-relative to being file-relative

            // Note that we read the ints below as floats using the GetFloatsFromAccessor function,
            // so we need to convert them into ints
            // Why do we add 0.5 before casting? Because if an index is stored as 1.999999999999943157,
            // it will be truncated into being equal to 1 instead of 2 by the cast
            glm::ivec4 jointsIndices(static_cast<int>(attributeFloats[indexOfFirstFloatOfCurrAttribValue + 0] + 0.5f),
                                     static_cast<int>(attributeFloats[indexOfFirstFloatOfCurrAttribValue + 1] + 0.5f),
                                     static_cast<int>(attributeFloats[indexOfFirstFloatOfCurrAttribValue + 2] + 0.5f),
                                     static_cast<int>(attributeFloats[indexOfFirstFloatOfCurrAttribValue + 3] + 0.5f));

            // TODO: Display error for negative indices
            // Convert the indices from being skin-relative to being file-relative
            // and store negative indices as zeroes
            jointsIndices.x = std::max(0, GetNodeIndex(skin->joints[jointsIndices.x], nodes, nodeCount));
            jointsIndices.y = std::max(0, GetNodeIndex(skin->joints[jointsIndices.y], nodes, nodeCount));
            jointsIndices.z = std::max(0, GetNodeIndex(skin->joints[jointsIndices.z], nodes, nodeCount));
            jointsIndices.w = std::max(0, GetNodeIndex(skin->joints[jointsIndices.w], nodes, nodeCount));

            influences.push_back(jointsIndices);
         }
         break;
         case cgltf_attribute_type_invalid:
         case cgltf_attribute_type_tangent:
         case cgltf_attribute_type_color:
            break;
         }
      }
   }

   // This function is identical to the one above, except that it's tailored for static meshes (i.e. meshes that are not animated)
   void StoreValuesOfAttributeInStaticMesh(cgltf_attribute& attribute, AnimatedMesh& outMesh)
   {
      // Get the accessor of the attribute and its component count
      cgltf_accessor& accessor = *attribute.data;
      unsigned int componentCount = 0;
      if (accessor.type == cgltf_type_vec2)
      {
         componentCount = 2;
      }
      else if (accessor.type == cgltf_type_vec3)
      {
         componentCount = 3;
      }
      else if (accessor.type == cgltf_type_vec4)
      {
         componentCount = 4;
      }

      // Read the floats from the accessor
      std::vector<float> attributeFloats;
      GetFloatsFromAccessor(accessor, componentCount, attributeFloats);

      // Get the vectors that will store the attributes of the mesh that we are loading
      // In each call to this function we only fill one of these, since a cgltf_attribute only describes one attribute
      std::vector<glm::vec3>&  positions  = outMesh.GetPositions();
      std::vector<glm::vec3>&  normals    = outMesh.GetNormals();
      std::vector<glm::vec2>&  texCoords  = outMesh.GetTexCoords();

      // Loop over all the values of the accessor
      // Note that accessor.count is not equal to the number of floats in the accessor
      // It's equal to the number of attribute values (e.g, vec2s, vec3s, vec4s, etc.) in the accessor
      cgltf_attribute_type attributeType = attribute.type;
      unsigned int numAttributeValues = static_cast<unsigned int>(accessor.count);
      for (unsigned int attributeValueIndex = 0; attributeValueIndex < numAttributeValues; ++attributeValueIndex)
      {
         // Store the current attribute value in the correct vector of the mesh
         // TODO: This looks inefficient. I think it would be better to have separate for loops inside if-statements
         //       Since this function only loads one attribute type when it's called, it doesn't make sense to check
         //       the attribute type in each iteration of this loop
         int indexOfFirstFloatOfCurrAttribValue = attributeValueIndex * componentCount;
         switch (attributeType)
         {
         case cgltf_attribute_type_position:
            // Store a position vec3
            positions.push_back(glm::vec3(attributeFloats[indexOfFirstFloatOfCurrAttribValue + 0],
                                          attributeFloats[indexOfFirstFloatOfCurrAttribValue + 1],
                                          attributeFloats[indexOfFirstFloatOfCurrAttribValue + 2]));
            break;
         case cgltf_attribute_type_texcoord:
            // Store a texture coordinates vec2
            texCoords.push_back(glm::vec2(attributeFloats[indexOfFirstFloatOfCurrAttribValue + 0],
                                          attributeFloats[indexOfFirstFloatOfCurrAttribValue + 1]));
            break;
         case cgltf_attribute_type_normal:
         {
            // Store a normal vec3
            glm::vec3 normal = glm::vec3(attributeFloats[indexOfFirstFloatOfCurrAttribValue + 0],
                                         attributeFloats[indexOfFirstFloatOfCurrAttribValue + 1],
                                         attributeFloats[indexOfFirstFloatOfCurrAttribValue + 2]);

            // TODO: Use a constant here and add an error message
            if (glm::length2(normal) < 0.000001f)
            {
               normal = glm::vec3(0, 1, 0);
            }

            normals.push_back(glm::normalize(normal));
         }
         break;
         case cgltf_attribute_type_invalid:
         case cgltf_attribute_type_tangent:
         case cgltf_attribute_type_color:
         case cgltf_attribute_type_joints:
         case cgltf_attribute_type_weights:
            break;
         }
      }
   }
}

cgltf_data* LoadGLTFFile(const char* path)
{
   // The cgltf_data struct contains all the information that's stored in a glTF file:
   // scenes, nodes, meshes, materials, skins, animations, cameras, textures, images, samplers, buffers, bufferViews and accessors
   cgltf_data* data = nullptr;

   // The cgltf_options struct can be used to control parts of the parsing process
   // Initializing it to zero tells cgltf that we want to use the default behavior
   cgltf_options options;
   memset(&options, 0, sizeof(cgltf_options));

   // Open the glTF file and parse the glTF data
   cgltf_result result = cgltf_parse_file(&options, path, &data);
   if (result != cgltf_result_success)
   {
      std::cout << "Could not parse the following glTF file: " << path << '\n';
      return nullptr;
   }

   // Open and read the buffer files referenced by the glTF file
   // Buffer files are binary files that contain geometry or animation data
   result = cgltf_load_buffers(&options, data, path);
   if (result != cgltf_result_success)
   {
      cgltf_free(data);
      std::cout << "Could not load the buffers of the following glTF file: " << path << '\n';
      return nullptr;
   }

   // Verify that the parsed glTF data is valid
   result = cgltf_validate(data);
   if (result != cgltf_result_success)
   {
      cgltf_free(data);
      std::cout << "The following glTF file is invalid: " << path << '\n';
      return nullptr;
   }

   return data;
}

void FreeGLTFFile(cgltf_data* data)
{
   if (data)
   {
      cgltf_free(data);
   }
   else
   {
      std::cout << "Tried to call cgltf_free on a null cgltf_data pointer\n";
   }
}

Pose LoadRestPose(cgltf_data* data)
{
   unsigned int numNodes = static_cast<unsigned int>(data->nodes_count);

   // We assume that the glTF file only contains one animated model and nothing else,
   // which means that each node represents a joint
   // Because of this, the number of nodes in the glTF file is equal to the number of joints in the Pose class
   Pose restPose(numNodes);

   // Loop over the array of nodes of the glTF file
   for (unsigned int nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex)
   {
      // Get the local transform of the current node
      Transform transform = GLTFHelpers::GetLocalTransformFromNode(data->nodes[nodeIndex]);

      // Store the local transform of the current node
      // Note how the array of joints of the Pose class has the same order as the array of nodes of the glTF file
      // So the joint indices or joint IDs of the Pose class are the same as the node indices of the glTF file
      restPose.SetLocalTransform(nodeIndex, transform);

      // Get the index of the current node's parent
      int parentIndex = GLTFHelpers::GetNodeIndex(data->nodes[nodeIndex].parent, data->nodes, numNodes);

      // Store the index of the current node's parent
      restPose.SetParent(nodeIndex, parentIndex);
   }

   return restPose;
}

std::vector<std::string> LoadJointNames(cgltf_data* data)
{
   unsigned int numNodes = static_cast<unsigned int>(data->nodes_count);
   std::vector<std::string> jointNames(numNodes, "Unnamed");

   // Loop over the array of nodes of the glTF file
   for (unsigned int nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex)
   {
      // If the current node has a name, store it
      if (data->nodes[nodeIndex].name)
      {
         jointNames[nodeIndex] = data->nodes[nodeIndex].name;
      }
   }

   return jointNames;
}

// A glTF file may contain an array of animations
// Each animation consists of two elements:
// - An array of channels
// - An array of samplers
// Each channel specifies:
// - The target of the animation
//   This usually refers to a node (by its index) and to a path, which is the name of the animated property
//   (e.g. "translation", "rotation", "scale" or "weights")
// - A sampler, which summarizes the animation data
std::vector<Clip> LoadClips(cgltf_data* data)
{
   unsigned int numNodes = static_cast<unsigned int>(data->nodes_count);
   unsigned int numClips = static_cast<unsigned int>(data->animations_count);

   std::vector<Clip> clips(numClips);

   // Loop over the array of animations of the glTF file
   for (unsigned int clipIndex = 0; clipIndex < numClips; ++clipIndex)
   {
      // Store the name of the current animation
      clips[clipIndex].SetName(data->animations[clipIndex].name);

      // Loop over the array of channels of the current animation
      unsigned int numChannels = static_cast<unsigned int>(data->animations[clipIndex].channels_count);
      for (unsigned int channelIndex = 0; channelIndex < numChannels; ++channelIndex)
      {
         // Get the current channel
         cgltf_animation_channel& channel = data->animations[clipIndex].channels[channelIndex];

         // Get the index of the node that the current channel targets
         int indexOfTargetNode = GLTFHelpers::GetNodeIndex(channel.target_node, data->nodes, numNodes);

         // Get a position, scale or rotation track from the channel depending on which of those values it animates
         // Note how we pass the index of the target node as a joint ID to the Clip class to get a TransformTrack
         // That's possible because all of our animation classes mirror the order of the nodes of the glTF file
         if (channel.target_path == cgltf_animation_path_type_translation)
         {
            VectorTrack& positionTrack = clips[clipIndex].GetTransformTrackOfJoint(indexOfTargetNode).GetPositionTrack();
            GLTFHelpers::GetTrackFromChannel<glm::vec3, 3>(channel, positionTrack);
         }
         else if (channel.target_path == cgltf_animation_path_type_scale)
         {
            VectorTrack& scaleTrack = clips[clipIndex].GetTransformTrackOfJoint(indexOfTargetNode).GetScaleTrack();
            GLTFHelpers::GetTrackFromChannel<glm::vec3, 3>(channel, scaleTrack);
         }
         else if (channel.target_path == cgltf_animation_path_type_rotation)
         {
            QuaternionTrack& rotationTrack = clips[clipIndex].GetTransformTrackOfJoint(indexOfTargetNode).GetRotationTrack();
            GLTFHelpers::GetTrackFromChannel<Q::quat, 4>(channel, rotationTrack);
         }
      }

      // Recalculate the duration of the current clip once all of its tracks have been loaded
      clips[clipIndex].RecalculateDuration();
   }

   return clips;
}

// A glTF file may contain an array of skins
// Each skin consists of two elements:
// - An array of joints, which are the indices of nodes that define the skeleton hierarchy
// - The inverseBindMatrices, which is a a reference to an accessor that contains one matrix for each joint
//   Note that these matrices are global, not local
// A node that refers to a mesh may also refer to a skin
Pose LoadBindPose(cgltf_data* data)
{
   // Initialize the global bind pose transforms with the global rest pose transforms
   // By doing this, we ensure that we have good default values if there are any skins that don't provide
   // inverse bind matrices for all of their joints
   Pose restPose = LoadRestPose(data);
   unsigned int numJointsOfPose = restPose.GetNumberOfJoints();
   std::vector<Transform> globalBindPoseTransforms(numJointsOfPose);
   for (unsigned int poseJointIndex = 0; poseJointIndex < numJointsOfPose; ++poseJointIndex)
   {
      globalBindPoseTransforms[poseJointIndex] = restPose.GetGlobalTransform(poseJointIndex);
   }

   // Loop over the array of skins of the glTF file
   unsigned int numSkins = static_cast<unsigned int>(data->skins_count);
   for (unsigned int skinIndex = 0; skinIndex < numSkins; ++skinIndex)
   {
      // Get the current skin
      cgltf_skin& skin = data->skins[skinIndex];

      // Get the global inverse bind matrices from the inverse bind matrices accessor of the current skin
      std::vector<float> globalInverseBindMatrices;
      GLTFHelpers::GetFloatsFromAccessor(*skin.inverse_bind_matrices, 16, globalInverseBindMatrices);

      // Loop over the array of joints of the current skin
      unsigned int numJointsOfSkin = (unsigned int)skin.joints_count;
      for (unsigned int skinJointIndex = 0; skinJointIndex < numJointsOfSkin; ++skinJointIndex)
      {
         // Get the global inverse bind matrix of the current joint
         glm::mat4 globalInverseBindMatrix = glm::make_mat4(&(globalInverseBindMatrices[skinJointIndex * 16]));

         // Invert the global inverse bind matrix to get the global bind matrix, then convert it into a transform
         Transform globalBindTransform = mat4ToTransform(glm::inverse(globalInverseBindMatrix));

         // Store the gloal bind transform at the index of the joint node it corresponds to
         cgltf_node* jointNode = skin.joints[skinJointIndex];
         int jointNodeIndex = GLTFHelpers::GetNodeIndex(jointNode, data->nodes, numJointsOfPose);
         globalBindPoseTransforms[jointNodeIndex] = globalBindTransform;
      }
   }

   // Convert the global bind pose into a local bind pose
   Pose bindPose = restPose;
   for (unsigned int poseJointIndex = 0; poseJointIndex < numJointsOfPose; ++poseJointIndex)
   {
      // Get the global bind transform of the current joint
      Transform transform = globalBindPoseTransforms[poseJointIndex];

      // If the current joint has a parent, bring the global bind transform into the space of that parent,
      // making it a local bind transform (i.e. a bind transform that's relative to the parent's bind transform)
      int parentIndex = bindPose.GetParent(poseJointIndex);
      if (parentIndex >= 0)
      {
         Transform parentTransform = globalBindPoseTransforms[parentIndex];

         /*
            Let's say our pose looks like this:

            (root) A ---- B        (root) 0 ---- 1
                          |                      |
                          |                      |
                          C ---- D               2 ---- 3

            Where A, B, C and D are the local transforms of the joints 0, 1, 2 and 3
            The global transform of joint 3 is composed like this:

               Global_3 = A * B * C * D

            The global transform of the parent of joint 3, which is joint 2, is composed like this:

               Global_2 = A * B * C

            If we only knew Global_3 and Global_2, how would we find the local transform of joint 3?
            It's simple. All we would need to do is combine the inverse of Global_2 with Global_3, which gives us:

               Local_3 = Inverse_Global_2 * Global_3 = (A * B * C)^-1 * (A * B * C * D) = D

            That's exactly what we do below
         */

         transform = combine(inverse(parentTransform), transform);
      }

      bindPose.SetLocalTransform(poseJointIndex, transform);
   }

   return bindPose;
}

Skeleton LoadSkeleton(cgltf_data* data)
{
   return Skeleton(LoadRestPose(data),
                   LoadBindPose(data),
                   LoadJointNames(data));
}

// A glTF file may contain an array of meshes
// Each mesh may contain multiple mesh primitives, which refer to the geometry data that is required to render a mesh
// Each mesh primitive consists of:
// - A rendering mode (e.g. POINTS, LINES or TRIANGLES)
// - A set of indices
// - The attributes of the vertices
// - The material that should be used for rendering
// This function loads the meshes of nodes that also refer to skins
// In other words, it loads animated meshes
std::vector<AnimatedMesh> LoadAnimatedMeshes(cgltf_data* data)
{
   std::vector<AnimatedMesh> animatedMeshes;

   // Loop over the array of nodes of the glTF file
   unsigned int numNodes = static_cast<unsigned int>(data->nodes_count);
   for (unsigned int nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex)
   {
      // This function only loads animated meshes, so the node must contain a mesh and a skin for us to process it
      cgltf_node* currNode = &data->nodes[nodeIndex];
      if (currNode->mesh == nullptr || currNode->skin == nullptr)
      {
         continue;
      }

      // Loop over the array of mesh primitives of the current node
      unsigned int numPrimitives = static_cast<unsigned int>(currNode->mesh->primitives_count);
      for (unsigned int primitiveIndex = 0; primitiveIndex < numPrimitives; ++primitiveIndex)
      {
         // Get the current mesh primitive
         cgltf_primitive* currPrimitive = &currNode->mesh->primitives[primitiveIndex];

         // Create an AnimatedMesh for the current mesh primitive
         animatedMeshes.push_back(AnimatedMesh());
         AnimatedMesh& currMesh = animatedMeshes[animatedMeshes.size() - 1];

         // Loop over the attributes of the current mesh primitive
         unsigned int numAttributes = static_cast<unsigned int>(currPrimitive->attributes_count);
         for (unsigned int attributeIndex = 0; attributeIndex < numAttributes; ++attributeIndex)
         {
            // Get the current attribute
            cgltf_attribute* attribute = &currPrimitive->attributes[attributeIndex];
            // Read the values of the current attribute and store them in the current mesh
            GLTFHelpers::StoreValuesOfAttributeInAnimatedMesh(*attribute, currNode->skin, data->nodes, numNodes, currMesh);
         }

         // If the current mesh primitive has a set of indices, store them too
         if (currPrimitive->indices != nullptr)
         {
            unsigned int indexCount = static_cast<unsigned int>(currPrimitive->indices->count);

            // Get the vector that will store the indices of the current mesh
            std::vector<unsigned int>& indices = currMesh.GetIndices();
            indices.resize(indexCount);

            // Loop over the indices of the current mesh primitive
            for (unsigned int i = 0; i < indexCount; ++i)
            {
               indices[i] = static_cast<unsigned int>(cgltf_accessor_read_index(currPrimitive->indices, i));
            }
         }

         // TODO: Perhaps we shouldn't do this here. The user should choose when this is done
         // Once we are done loading the current mesh, we load its VBOs with the data that we read
         currMesh.LoadBuffers();
      }
   }

   return animatedMeshes;
}

// This function is identical to the one above, except that it loads the meshes of nodes that don't refer to skins
// In other words, it loads static meshes
std::vector<AnimatedMesh> LoadStaticMeshes(cgltf_data* data)
{
   std::vector<AnimatedMesh> staticMeshes;

   // Loop over the array of nodes of the glTF file
   unsigned int numNodes = static_cast<unsigned int>(data->nodes_count);
   for (unsigned int nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex)
   {
      // This function only loads static meshes, so the node must contain a mesh for us to process it
      cgltf_node* currNode = &data->nodes[nodeIndex];
      if (currNode->mesh == nullptr)
      {
         continue;
      }

      // Loop over the array of mesh primitives of the current node
      unsigned int numPrimitives = static_cast<unsigned int>(currNode->mesh->primitives_count);
      for (unsigned int primitiveIndex = 0; primitiveIndex < numPrimitives; ++primitiveIndex)
      {
         // Get the current mesh primitive
         cgltf_primitive* currPrimitive = &currNode->mesh->primitives[primitiveIndex];

         // Create an AnimatedMesh for the current mesh primitive
         staticMeshes.push_back(AnimatedMesh());
         AnimatedMesh& currMesh = staticMeshes[staticMeshes.size() - 1];

         // Loop over the attributes of the current mesh primitive
         unsigned int numAttributes = static_cast<unsigned int>(currPrimitive->attributes_count);
         for (unsigned int attributeIndex = 0; attributeIndex < numAttributes; ++attributeIndex)
         {
            // Get the current attribute
            cgltf_attribute* attribute = &currPrimitive->attributes[attributeIndex];
            // Read the values of the current attribute and store them in the current mesh
            GLTFHelpers::StoreValuesOfAttributeInStaticMesh(*attribute, currMesh);
         }

         // If the current mesh primitive has a set of indices, store them too
         if (currPrimitive->indices != nullptr)
         {
            unsigned int indexCount = static_cast<unsigned int>(currPrimitive->indices->count);

            // Get the vector that will store the indices of the current mesh
            std::vector<unsigned int>& indices = currMesh.GetIndices();
            indices.resize(indexCount);

            // Loop over the indices of the current mesh primitive
            for (unsigned int i = 0; i < indexCount; ++i)
            {
               indices[i] = static_cast<unsigned int>(cgltf_accessor_read_index(currPrimitive->indices, i));
            }
         }

         // TODO: Perhaps we shouldn't do this here. The user should choose when this is done
         // Once we are done loading the current mesh, we load its VBOs with the data that we read
         currMesh.LoadBuffers();
      }
   }

   return staticMeshes;
}
