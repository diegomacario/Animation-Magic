#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "quat.h"

#define TRANSFORM_EPSILON 0.000001f

struct Transform
{
public:

   Transform()
      : position(glm::vec3(0.0f))
      , rotation(Q::quat()) // Identity quaternion (w = 1.0f)
      , scale(glm::vec3(1.0f))
   {

   }

   Transform(const glm::vec3& p, const Q::quat& r, const glm::vec3& s)
      : position(p)
      , rotation(r)
      , scale(s)
   {

   }

   glm::vec3 position;
   Q::quat   rotation;
   glm::vec3 scale;
};

// TODO: Update all operators in this project to use rhs/lhs notation
bool      operator==(const Transform& a, const Transform& b);
bool      operator!=(const Transform& a, const Transform& b);

Transform combine(const Transform& parent, const Transform& child);

Transform inverse(const Transform& t);

Transform mix(const Transform& a, const Transform& b, float t);

glm::mat4 transformToMat4(const Transform& t);
Transform mat4ToTransform(const glm::mat4& m);
glm::vec3 transformPoint(const Transform& t, const glm::vec3& p);
glm::vec3 transformVector(const Transform& t, const glm::vec3& v);

#endif
