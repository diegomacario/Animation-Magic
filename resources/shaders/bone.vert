in vec3 inPos;
in vec3 inCol;

uniform mat4 model;
uniform mat4 projectionView;

out vec3 col;

void main()
{
   col = inCol;
   gl_Position = projectionView * model * vec4(inPos, 1.0f);
}
