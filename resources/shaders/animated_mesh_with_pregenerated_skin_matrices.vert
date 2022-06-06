in vec3  position;
in vec3  normal;
in vec2  texCoord;
in vec4  weights;
in ivec4 joints;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform mat4 animated[120];

out vec3 norm;
out vec3 fragPos;
out vec2 uv;

void main()
{
   mat4 skin = (animated[joints.x] * weights.x) +
               (animated[joints.y] * weights.y) +
               (animated[joints.z] * weights.z) +
               (animated[joints.w] * weights.w);

   gl_Position = projection * view * model * skin * vec4(position, 1.0f);

   fragPos = vec3(model * skin * vec4(position, 1.0f));
   norm    = vec3(model * skin * vec4(normal, 0.0f));
   uv      = texCoord;
}
