#version 430

#ifdef COMPUTE_SHADER

layout(std430, binding = 0) buffer inp
{
    int input_data[];
};

layout(std430, binding = 1) buffer outp
{
    int output_data[];
};

uniform int u_operand;

layout(local_size_x = 256) in;
void main()
{
    uint id = gl_GlobalInvocationID.x;

    // chaque thread transforme le sommet d'indice ID...
    if(id < input_data.length())
        output_data[id] = u_operand + input_data[id];
}

#endif

