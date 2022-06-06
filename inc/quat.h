#ifndef QUAT_H
#define QUAT_H

#include <glm/glm.hpp>

#define QUAT_EPSILON 0.000001f

namespace Q
{
   struct quat
   {
   public:

      quat()
         : x(0.0f)
         , y(0.0f)
         , z(0.0f)
         , w(1.0f)
      {

      }

      quat(float _x, float _y, float _z, float _w)
         : x(_x)
         , y(_y)
         , z(_z)
         , w(_w)
      {

      }

      union
      {
         struct
         {
            float x;
            float y;
            float z;
            float w;
         };

         struct
         {
            glm::vec3 vector;
            float     scalar;
         };

         float v[4];
      };
   };

   quat      angleAxis(float angle, const glm::vec3& axis);
   float     getAngle(const quat& q);
   glm::vec3 getAxis(const quat& q);
   quat      fromTo(const glm::vec3& from, const glm::vec3& to);

   quat      operator+(const quat& a, const quat& b);
   quat      operator-(const quat& a, const quat& b);
   quat      operator-(const quat& q);
   quat      operator*(const quat& q, float f);
   quat      operator*(float f, const quat& q);
   quat      operator*(const quat& q, const quat& p);
   glm::vec3 operator*(const quat& q, const glm::vec3& v);
   quat      operator^(const quat& q, float exponent);
   bool      operator==(const quat& a, const quat& b);
   bool      operator!=(const quat& a, const quat& b);

   bool      sameOrientation(const quat& a, const quat& b);

   float     dot(const quat& a, const quat& b);

   float     squaredLength(const quat& q);
   float     length(const quat& q);

   void      normalize(quat& q);
   quat      normalized(const quat& q);

   quat      conjugate(const quat& q);
   quat      inverse(const quat& q);

   quat      mix(const quat& start, const quat& end, float t);
   quat      nlerp(const quat& start, const quat& end, float t);
   quat      slerp(const quat& start, const quat& end, float t);

   quat      lookRotation(const glm::vec3& direction, const glm::vec3& upReference);

   glm::mat4 quatToMat4(const quat& q);
   quat      mat4ToQuat(const glm::mat4& m);
}

glm::vec3 normalizeWithZeroLengthCheck(const glm::vec3& v);

#endif
