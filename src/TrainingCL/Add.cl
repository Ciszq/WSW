__kernel void Add(__global int* a, __global int* b, __global int* c, int size)
{
	// Find position in global arrays.
	int n = get_global_id(0);

	// Bound check.
	if (n < size)
	{
		c[n] = a[n] - b[n];
	}
}
