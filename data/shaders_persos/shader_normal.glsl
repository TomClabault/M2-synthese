#version 330




#ifdef VERTEX_SHADER
layout(location = 0) in vec3 position;
layout(location = 2) in vec3 normal;

uniform mat4 mvpMatrix;

out vertex_to_shader_interface {
    vec3 position;
    vec3 normal;
} vs_fs_interface ;

void main( )
{
    gl_Position = mvpMatrix * vec4(position, 1);

    vs_fs_interface.position = position;
    vs_fs_interface.normal = normal;
}
#endif




 
#ifdef FRAGMENT_SHADER

uniform vec4 meshColor;
uniform vec3 light_position;

in vertex_to_shader_interface {
    vec3 position;
    vec3 normal;
} vs_fs_interface ;

void main( )
{
    //Pastel normals
    //gl_FragColor = vec4((vs_fs_interface.normal + 1) / 2, 1);

    //Diffuse shading
    gl_FragColor = meshColor * dot(vs_fs_interface.normal, normalize(light_position - vs_fs_interface.position));

    //Diffuse toon shading
    const int paliers = 8;
    float cos_value = dot(vs_fs_interface.normal, normalize(light_position - vs_fs_interface.position));
    float cos_value_palier;
    for (int i = 0; i < paliers; i++)
    {
        float palier_value = 1.0f / float(i + 1);
        if (cos_value < palier_value)
            cos_value_palier = palier_value;
    }

    gl_FragColor = meshColor * cos_value_palier;
}
#endif
