#include <utility>

#include "texture.h"

Texture::Texture(unsigned int texID)
   : mTexID(texID)
{

}

Texture::~Texture()
{
   glDeleteTextures(1, &mTexID);
}

Texture::Texture(Texture&& rhs) noexcept
   : mTexID(std::exchange(rhs.mTexID, 0))
{

}

Texture& Texture::operator=(Texture&& rhs) noexcept
{
   mTexID = std::exchange(rhs.mTexID, 0);
   return *this;
}

void Texture::bind(unsigned int textureUnit, int uniformLocation) const
{
   // Activate the proper texture unit before binding the texture
   glActiveTexture(GL_TEXTURE0 + textureUnit);
   // Bind the texture
   glBindTexture(GL_TEXTURE_2D, mTexID);
   // Tell the appropriate sampler2D uniform in what texture unit to look for the texture data
   glUniform1i(uniformLocation, textureUnit);
}

void Texture::unbind(unsigned int textureUnit) const
{
   // Activate the proper texture unit before unbinding the texture
   glActiveTexture(GL_TEXTURE0 + textureUnit);
   // Unbind the texture
   glBindTexture(GL_TEXTURE_2D, 0);
   // Disactivate the texture unit
   glActiveTexture(GL_TEXTURE0);
}
