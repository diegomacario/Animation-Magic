#ifndef GLTF_LOADER_H
#define GLTF_LOADER_H

#include "cgltf/cgltf.h"
#include "Pose.h"
#include "Skeleton.h"
#include "AnimatedMesh.h"
#include "Clip.h"
#include <vector>
#include <string>

cgltf_data*               LoadGLTFFile(const char* path);
void                      FreeGLTFFile(cgltf_data* handle);

Pose                      LoadRestPose(cgltf_data* data);
std::vector<std::string>  LoadJointNames(cgltf_data* data);
std::vector<Clip>         LoadClips(cgltf_data* data);
Pose                      LoadBindPose(cgltf_data* data);
Skeleton                  LoadSkeleton(cgltf_data* data);
std::vector<AnimatedMesh> LoadAnimatedMeshes(cgltf_data* data);
std::vector<AnimatedMesh> LoadStaticMeshes(cgltf_data* data);

#endif
