#version 430

#ifdef COMPUTE_SHADER

layout(std430, binding = 0) buffer inputData
{
	int global_counter;
	int input_data[];
};

layout(std430, binding = 1) buffer bufferOutputData
{
	int output_data[];
};

uniform int u_operand;

layout(local_size_x = 256) in;
void main()
{
	const uint thread_id = gl_GlobalInvocationID.x;

	if (thread_id < input_data.length())
	{
		int value = input_data[thread_id] + u_operand;
		int counter = atomicAdd(global_counter, 1);

		if (value >= 100)
			output_data[counter] = value;
	}
}

#endif
