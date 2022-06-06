#ifndef SHADER_H
#define SHADER_H

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif
#include <glm/glm.hpp>

#include <string>
#include <map>
#include <vector>

class Shader
{
public:

   Shader(unsigned int shaderProgID,
          std::map<std::string, unsigned int>&& attributes,
          std::map<std::string, unsigned int>&& uniforms);
   ~Shader();

   Shader(const Shader&) = delete;
   Shader& operator=(const Shader&) = delete;

   Shader(Shader&& rhs) noexcept;
   Shader& operator=(Shader&& rhs) noexcept;

   void         use(bool use) const;

   unsigned int getID() const;

   void         setUniformBool(const std::string& name, bool value) const;
   void         setUniformInt(const std::string& name, int value) const;
   void         setUniformFloat(const std::string& name, float value) const;

   void         setUniformVec2(const std::string& name, const glm::vec2& value) const;
   void         setUniformVec2(const std::string& name, float x, float y) const;
   void         setUniformVec3(const std::string& name, const glm::vec3& value) const;
   void         setUniformVec3(const std::string& name, float x, float y, float z) const;
   void         setUniformVec4(const std::string& name, const glm::vec4& value) const;
   void         setUniformVec4(const std::string& name, float x, float y, float z, float w) const;

   void         setUniformMat2(const std::string& name, const glm::mat2& value) const;
   void         setUniformMat3(const std::string& name, const glm::mat3& value) const;
   void         setUniformMat4(const std::string& name, const glm::mat4& value) const;
   void         setUniformMat4Array(const std::string& name, const std::vector<glm::mat4>& values) const;

   int          getAttributeLocation(const std::string& attributeName) const;
   int          getUniformLocation(const std::string& uniformName) const;

private:

   unsigned int                        mShaderProgID;
   std::map<std::string, unsigned int> mAttributes;
   std::map<std::string, unsigned int> mUniforms;
};

#endif
