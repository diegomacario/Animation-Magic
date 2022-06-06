in vec3 position;

uniform mat4 projectionView;

void main()
{
   gl_Position = projectionView * vec4(position, 1.0);
}
