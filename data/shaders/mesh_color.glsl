
#version 330

#ifdef VERTEX_SHADER
layout(location= 0) in vec3 position;

#ifdef USE_COLOR
    layout(location= 3) in vec3 color;
    out vec3 vertex_color;
#endif

uniform mat4 mvpMatrix;

void main( )
{
    gl_Position= mvpMatrix * vec4(position, 1);
#ifdef USE_COLOR
    vertex_color= color;
#endif
}
#endif

#ifdef FRAGMENT_SHADER

#ifdef USE_COLOR
    in vec3 vertex_color;
#endif

uniform vec4 mesh_color;

void main( )
{
    vec4 color= mesh_color;
#ifdef USE_COLOR
    color= vec4(vertex_color, 1);
#endif

    gl_FragColor= color;
}
#endif
