__kernel void YCbCr(__global int* R, __global int* G, __global int* B, __global int* Y, int size)
{
	// Find position in global arrays.
	int n = get_global_id(0);
	float 
		a11 = 0.299,
		a12 = 0.587,
		a13 = 0.114;

	// Bound check.
	if (n < size)
	{
		Y[n] = a11 * R[n] + a12 * G[n] + a13 * B[n];
	}
}