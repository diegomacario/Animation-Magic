in vec3 inPos;
in vec3 inNormal;

#define MAX_NUMBER_OF_SKIN_MATRICES 49
uniform mat4 modelMatrices[MAX_NUMBER_OF_SKIN_MATRICES];
uniform mat4 projectionView;
uniform int  indexOfGlowingJoint;

out vec3 norm;
out vec3 fragPos;
out vec3 col;

void main()
{
   fragPos = vec3(modelMatrices[gl_InstanceID] * vec4(inPos, 1.0f));
   norm    = normalize(vec3(modelMatrices[gl_InstanceID] * vec4(inNormal, 0.0f)));

   if (indexOfGlowingJoint == gl_InstanceID)
   {
      col = vec3(0.0f, 0.25f, 0.25f);
   }
   else
   {
      col = vec3(0.0f, 0.35f, 0.0f);
   }

   gl_Position = projectionView * vec4(fragPos, 1.0);
}
