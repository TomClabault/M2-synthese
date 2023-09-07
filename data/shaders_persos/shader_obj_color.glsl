#version 330

#ifdef VERTEX_SHADER

layout(location = 0) in vec3 position;
layout(location = 2) in vec3 normal;
layout(location = 4) in uint material_index;

uniform mat4 mvpMatrix;

flat out uint fs_material_index;
out vec3 fs_position;
out vec3 fs_normal;

void main()
{
    gl_Position = mvpMatrix * vec4(position, 1);

    fs_material_index = material_index;
    fs_position = position;
    fs_normal = normal;
}
#endif


#ifdef FRAGMENT_SHADER

flat in uint fs_material_index;

in vec3 fs_position;
in vec3 fs_normal;

uniform vec3 light_position;
uniform vec4 robotMaterials[3];

void main()
{
    gl_FragColor = robotMaterials[fs_material_index] * dot(fs_normal, normalize(light_position - fs_position));
}
#endif
