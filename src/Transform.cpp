#include <glm/gtx/compatibility.hpp>

#include "Transform.h"

bool operator==(const Transform& a, const Transform& b)
{
   return (a.position == b.position) &&
          (a.rotation == b.rotation) &&
          (a.scale == b.scale);
}

bool operator!=(const Transform& a, const Transform& b)
{
   return !(a == b);
}

Transform combine(const Transform& parent, const Transform& child)
{
   Transform result;

   result.scale = parent.scale * child.scale;

   // NOTE: Reversed because q * p is implemented as p * q
   result.rotation = child.rotation * parent.rotation;

   // Bring the child's position into the parent's space before adding their positions
   // First scale, then rotate and finally translate
   result.position = parent.rotation * (parent.scale * child.position);
   result.position = parent.position + result.position;

   return result;
}

Transform inverse(const Transform& t)
{
   Transform result;

   result.scale.x = glm::abs(t.scale.x) < TRANSFORM_EPSILON ? 0.0f : (1.0f / t.scale.x);
   result.scale.y = glm::abs(t.scale.y) < TRANSFORM_EPSILON ? 0.0f : (1.0f / t.scale.y);
   result.scale.z = glm::abs(t.scale.z) < TRANSFORM_EPSILON ? 0.0f : (1.0f / t.scale.z);

   result.rotation = Q::inverse(t.rotation);

   // The inverse position needs to be scaled and rotated to bring it into the new space
   glm::vec3 inversePos = -t.position;
   result.position = result.rotation * (result.scale * inversePos);

   return result;
}

Transform mix(const Transform& a, const Transform& b, float t)
{
   // Quaternion neighborhood check
   Q::quat bRotation = b.rotation;
   if (Q::dot(a.rotation, b.rotation) < 0.0f)
   {
      bRotation = -bRotation;
   }

   return Transform(glm::lerp(a.position, b.position, t),
                    Q::nlerp(a.rotation, bRotation, t),
                    glm::lerp(a.scale, b.scale, t));
}

glm::mat4 transformToMat4(const Transform& t)
{
   // Calculate the rotation basis of the transform
   glm::vec3 right = t.rotation * glm::vec3(1, 0, 0);
   glm::vec3 up    = t.rotation * glm::vec3(0, 1, 0);
   glm::vec3 fwd   = t.rotation * glm::vec3(0, 0, 1);

   // Scale the rotation basis
   right *= t.scale.x;
   up    *= t.scale.y;
   fwd   *= t.scale.z;

   // Compose the transformation matrix
   return glm::mat4(right.x,      right.y,      right.z,      0,  // Scaled X basis
                    up.x,         up.y,         up.z,         0,  // Scaled Y basis
                    fwd.x,        fwd.y,        fwd.z,        0,  // Scaled Z basis
                    t.position.x, t.position.y, t.position.z, 1); // Position
}

Transform mat4ToTransform(const glm::mat4& m)
{
   Transform result;

   // Extract the position from the matrix
   result.position = glm::vec3(m[3][0], m[3][1], m[3][2]);

   // Extract the rotation from the matrix
   // We can do this even if the matrix contains scale information
   // by normalizing the rotation bases, which is what Q::mat4ToQuat does
   result.rotation = Q::mat4ToQuat(m);

   // This matrix contains the scale and rotation information
   glm::mat4 scaleAndRotMat(m[0][0], m[0][1], m[0][2], 0, // Scaled X basis
                            m[1][0], m[1][1], m[1][2], 0, // Scaled Y basis
                            m[2][0], m[2][1], m[2][2], 0, // Scaled Z basis
                            0,       0,       0,       1);

   // This matrix only contains the inverse of the rotation information
   glm::mat4 inverseRotMat = Q::quatToMat4(Q::inverse(result.rotation));

   // By multiplying the previous two matrices we end up with a matrix that contains scale and skew information
   glm::mat4 scaleAndSkewMat = scaleAndRotMat * inverseRotMat;

   // To extract the scale information from the scale and skew matrix, we will simply take its diagonal
   // This isn't perfect though, so the scale we are extracting here should be considered to be a "lossy" scale
   // It's possible to get an accurate scale using matrix decomposition, but that's an expensive operation
   result.scale = glm::vec3(scaleAndSkewMat[0][0], scaleAndSkewMat[1][1], scaleAndSkewMat[2][2]);

   return result;
}

glm::vec3 transformPoint(const Transform& t, const glm::vec3& p)
{
   // First scale, then rotate and finally translate
   return t.position + (t.rotation * (t.scale * p));
}

glm::vec3 transformVector(const Transform& t, const glm::vec3& v)
{
   // First scale, then rotate
   // We don't translate because vectors don't have a position
   // Only a magnitude and a direction
   return t.rotation * (t.scale * v);
}
