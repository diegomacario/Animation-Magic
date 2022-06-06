#ifndef SHADER_LOADER_H
#define SHADER_LOADER_H

#include <memory>
#include <map>

#include "shader.h"

class ShaderLoader
{
public:

   ShaderLoader() = default;
   ~ShaderLoader() = default;

   ShaderLoader(const ShaderLoader&) = default;
   ShaderLoader& operator=(const ShaderLoader&) = default;

   ShaderLoader(ShaderLoader&&) = default;
   ShaderLoader& operator=(ShaderLoader&&) = default;

   std::shared_ptr<Shader> loadResource(const std::string& vShaderFilePath,
                                        const std::string& fShaderFilePath) const;

#ifndef __EMSCRIPTEN__
   std::shared_ptr<Shader> loadResource(const std::string& vShaderFilePath,
                                        const std::string& fShaderFilePath,
                                        const std::string& gShaderFilePath) const;
#endif

private:

   bool                    readShaderFile(const std::string& shaderFilePath, std::string& outShaderCode) const;
   void                    addVersionToShaderCode(std::string& ioShaderCode, GLenum shaderType) const;

   unsigned int            createAndCompileShader(const std::string& shaderCode, GLenum shaderType) const;
   unsigned int            createAndLinkShaderProgram(unsigned int vShaderID, unsigned int fShaderID) const;
#ifndef __EMSCRIPTEN__
   unsigned int            createAndLinkShaderProgram(unsigned int vShaderID, unsigned int fShaderID, unsigned int gShaderID) const;
#endif

   bool                    shaderCompilationSucceeded(unsigned int shaderID) const;
   bool                    shaderProgramLinkingSucceeded(unsigned int shaderProgID) const;

   void                    logShaderCompilationErrors(unsigned int shaderID, GLenum shaderType, const std::string& shaderFilePath) const;
   void                    logShaderProgramLinkingErrors(unsigned int shaderProgID) const;

   void                    readAttributes(unsigned int shaderProgID, std::map<std::string, unsigned int>& outAttributes) const;
   void                    readUniforms(unsigned int shaderProgID, std::map<std::string, unsigned int>& outUniforms) const;
};

#endif
