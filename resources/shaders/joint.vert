in vec3 inPos;
in vec3 inNormal;

uniform mat4 modelMatrices[120];
uniform mat4 projectionView;

out vec3 norm;
out vec3 fragPos;

void main()
{
   fragPos = vec3(modelMatrices[gl_InstanceID] * vec4(inPos, 1.0f));
   norm    = vec3(normalize(modelMatrices[gl_InstanceID] * vec4(inNormal, 0.0f)));

   gl_Position = projectionView * vec4(fragPos, 1.0);
}
