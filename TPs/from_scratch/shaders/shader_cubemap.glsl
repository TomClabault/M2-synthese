#version 330

#ifdef VERTEX_SHADER

void main()
{
	//We're going to draw the skybox on a triangle that spans the whole screen
	//The triangle's vertices coordinates are specified in screen space so (-1, -1) is the bottom left
	//of the screen, (3, -1) is the bottom right (but way more to the right than necessary to be sure that
	//the triangle overlaps all the pixels of the screen). Same logic with (-1, 3) which is top left 
	//(more top than necessary)
	vec2 triangle_vertices[3] = vec2[3](vec2(-1, -1), vec2(3, -1), vec2(-1, 3));

	//Because we expect gl_Position to be a vec4, we're adding z=1 and w=1.
	//w=1 is the classical homogenous coordinate and z=1 is the far plane so that
	//our skybox is always as far in the background as possible (otherwise object could be drawn
	//BEHIND the skybox which should be impossible because objects can't be behind the sky.... so that's
	//why we're choosing the far plane)
	gl_Position = vec4(triangle_vertices[gl_VertexID], 1, 1);
}
#endif

#ifdef FRAGMENT_SHADER

uniform mat4 u_inverse_matrix;
uniform vec3 u_camera_position;

uniform samplerCube u_cubemap;

void main()
{
	//Because we created a scren-wide triangle in the vertex shader, the fragment shader is going to have
	//to find the color of the skybox for this fragment
	//We're simply going to us the position of the fragment (in world space, not screen space hence the inverse transformation matrix)
	//as the direciton to sample the cubemap

	vec4 world_position = u_inverse_matrix * vec4(gl_FragCoord.xyz, 1);
	vec3 pixel = world_position.xyz / world_position.w;

	vec3 direction = normalize(pixel - u_camera_position);
	gl_FragColor = texture(u_cubemap, direction);
	//gl_FragColor = vec4(0.5, 0, 0, 1);
}
#endif