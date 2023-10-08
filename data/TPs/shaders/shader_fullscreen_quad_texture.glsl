#version 330

#ifdef VERTEX_SHADER

out vec2 vs_tex_coords;

void main()
{
	vec2 triangle_vertices[6] = vec2[6](vec2(-1, -1), vec2(1, -1), vec2(-1, 1), vec2(1, -1), vec2(1, 1), vec2(-1, 1));
	vec2 triangle_tex_coords[6] = vec2[6](vec2(0, 0), vec2(1, 0), vec2(0, 1), vec2(1, 0), vec2(1, 1), vec2(0, 1));

	//Because we expect gl_Position to be a vec4, we're adding z=0 and w=1.
	//w=1 is the classical homogenous coordinate and z=0 is the near plane so that
	//we render the texture in front of everything
        gl_Position = vec4(triangle_vertices[gl_VertexID], 1, 1);
	vs_tex_coords = triangle_tex_coords[gl_VertexID];
}
#endif

#ifdef FRAGMENT_SHADER

uniform sampler2D u_texture;

in vec2 vs_tex_coords;

void main()
{
    gl_FragColor = texture(u_texture, vs_tex_coords);
}
#endif
