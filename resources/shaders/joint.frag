in vec3 fragPos;
in vec3 norm;

struct PointLight
{
   vec3  worldPos;
   vec3  color;
   float constantAtt;
   float linearAtt;
   float quadraticAtt;
};

#define MAX_NUMBER_OF_POINT_LIGHTS 4
uniform PointLight pointLights[MAX_NUMBER_OF_POINT_LIGHTS];
uniform int numPointLightsInScene;

uniform vec3 cameraPos;

out vec4 fragColor;

void main()
{
   vec3 viewDir = normalize(cameraPos - fragPos);

   vec3 color = vec3(0.0);
   for(int i = 0; i < numPointLightsInScene; i++)
   {
      // Attenuation
      float distance    = length(pointLights[i].worldPos - fragPos);
      float attenuation = 1.0 / (pointLights[i].constantAtt + (pointLights[i].linearAtt * distance) + (pointLights[i].quadraticAtt * distance * distance));

      // Diffuse
      vec3  lightDir    = normalize(pointLights[i].worldPos - fragPos);
      vec3  diff        = max(dot(lightDir, norm), 0.0) * pointLights[i].color * attenuation;
      vec3  diffuse     = (diff * vec3(0.0f, 1.0f, 0.0f));
   
      color += diffuse + vec3(0.0f, 0.25f, 0.0f);
   }

   fragColor = vec4(color, 1.0);
}
