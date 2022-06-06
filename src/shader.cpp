#include <glm/gtc/type_ptr.hpp>

#include <iostream>

#include "shader.h"

Shader::Shader(unsigned int shaderProgID,
               std::map<std::string, unsigned int>&& attributes,
               std::map<std::string, unsigned int>&& uniforms)
   : mShaderProgID(shaderProgID)
   , mAttributes(std::move(attributes))
   , mUniforms(std::move(uniforms))
{

}

Shader::~Shader()
{
   glDeleteProgram(mShaderProgID);
}

Shader::Shader(Shader&& rhs) noexcept
   : mShaderProgID(std::exchange(rhs.mShaderProgID, 0))
   , mAttributes(std::move(rhs.mAttributes))
   , mUniforms(std::move(rhs.mUniforms))
{

}

Shader& Shader::operator=(Shader&& rhs) noexcept
{
   mShaderProgID = std::exchange(rhs.mShaderProgID, 0);
   mAttributes   = std::move(rhs.mAttributes);
   mUniforms     = std::move(rhs.mUniforms);
   return *this;
}

void Shader::use(bool use) const
{
   glUseProgram(use ? mShaderProgID : 0);
}

unsigned int Shader::getID() const
{
   return mShaderProgID;
}

void Shader::setUniformBool(const std::string& name, bool value) const
{
   glUniform1i(getUniformLocation(name.c_str()), static_cast<int>(value));
}

void Shader::setUniformInt(const std::string& name, int value) const
{
   glUniform1i(getUniformLocation(name.c_str()), value);
}

void Shader::setUniformFloat(const std::string& name, float value) const
{
   glUniform1f(getUniformLocation(name.c_str()), value);
}

void Shader::setUniformVec2(const std::string& name, const glm::vec2 &value) const
{
   glUniform2fv(getUniformLocation(name.c_str()), 1, &value[0]);
}

void Shader::setUniformVec2(const std::string& name, float x, float y) const
{
   glUniform2f(getUniformLocation(name.c_str()), x, y);
}

void Shader::setUniformVec3(const std::string& name, const glm::vec3 &value) const
{
   glUniform3fv(getUniformLocation(name.c_str()), 1, &value[0]);
}

void Shader::setUniformVec3(const std::string& name, float x, float y, float z) const
{
   glUniform3f(getUniformLocation(name.c_str()), x, y, z);
}

void Shader::setUniformVec4(const std::string& name, const glm::vec4 &value) const
{
   glUniform4fv(getUniformLocation(name.c_str()), 1, &value[0]);
}

void Shader::setUniformVec4(const std::string& name, float x, float y, float z, float w) const
{
   glUniform4f(getUniformLocation(name.c_str()), x, y, z, w);
}

void Shader::setUniformMat2(const std::string& name, const glm::mat2& value) const
{
   glUniformMatrix2fv(getUniformLocation(name.c_str()), 1, GL_FALSE, &value[0][0]);
}

void Shader::setUniformMat3(const std::string& name, const glm::mat3& value) const
{
   glUniformMatrix3fv(getUniformLocation(name.c_str()), 1, GL_FALSE, &value[0][0]);
}

void Shader::setUniformMat4(const std::string& name, const glm::mat4& value) const
{
   glUniformMatrix4fv(getUniformLocation(name.c_str()), 1, GL_FALSE, &value[0][0]);
}

void Shader::setUniformMat4Array(const std::string& name, const std::vector<glm::mat4>& values) const
{
   glUniformMatrix4fv(getUniformLocation(name.c_str()), static_cast<GLsizei>(values.size()), GL_FALSE, glm::value_ptr(values[0]));
}

int Shader::getAttributeLocation(const std::string& attributeName) const
{
   std::map<std::string, unsigned int>::const_iterator it = mAttributes.find(attributeName);

   if (it == mAttributes.end())
   {
      std::cout << "Error - Shader::getAttributeLocation - The following attribute does not exist: " << attributeName << "\n";
      return -1;
   }

   return it->second;
}

int Shader::getUniformLocation(const std::string& uniformName) const
{
   std::map<std::string, unsigned int>::const_iterator it = mUniforms.find(uniformName);

   if (it == mUniforms.end())
   {
      std::cout << "Error - Shader::getUniformLocation - The following uniform does not exist: " << uniformName << "\n";
      return -1;
   }

   return it->second;
}
