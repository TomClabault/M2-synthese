
#version 330

#ifdef VERTEX_SHADER
layout(location= 0) in vec3 position;
layout(location= 2) in vec3 normal;

out vec3 vertex_normal;
out vec4 frustum_position;

uniform mat4 mvpMatrix;
uniform mat4 mvMatrix;
uniform mat4 frustumMatrix;

void main( )
{
    gl_Position= mvpMatrix * vec4(position, 1);
    vertex_normal= mat3(mvMatrix) * normal;
    frustum_position= frustumMatrix * vec4(position, 1);
}
#endif

#ifdef FRAGMENT_SHADER
in vec3 vertex_normal;
in vec4 frustum_position;

out vec4 fragment_color;

uniform vec4 mesh_color;

void main( )
{
    fragment_color= vec4(1, 1, 0, 1);
    
    vec4 p= frustum_position;
    if(p.x < -p.w)  fragment_color= mesh_color;
    if(p.x >  p.w)  fragment_color= mesh_color;
    if(p.y < -p.w)  fragment_color= mesh_color;
    if(p.y >  p.w)  fragment_color= mesh_color;
    if(p.z < -p.w)  fragment_color= mesh_color;
    if(p.z >  p.w)  fragment_color= mesh_color;
    
    float cos_theta= normalize(vertex_normal).z;
    fragment_color= vec4(fragment_color.rgb * cos_theta, 1);
}
#endif
