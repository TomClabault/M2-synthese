#version 430

#ifdef COMPUTE_SHADER

layout(std430, binding=0) buffer inp {
    int dIn[];
};

layout(std430, binding=1) buffer  outp {
    int dOut[];
};

uniform int modifier;

layout(local_size_x = 256) in;
void main( )
{
    uint id = gl_GlobalInvocationID.x;
    if(id < dIn.length())
    {
        dOut[id] = dIn[id] + modifier;
    }
}

#endif
