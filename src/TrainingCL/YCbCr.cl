__kernel void YCbCr(__global int* R, __global int* G, __global int* B, __global int* Bin, int size)
{
	// Find position in global arrays.
	int n = get_global_id(0);
	float 
		a11 = 0.299,
		a12 = 0.587,
		a13 = 0.114,
		a21 = -0.168736,
		a22 = 0.331264,
		a23 = 0.5,
		a31 = 0.5,
		a32 = 0.418688,
		a33 = 0.081312;
	uint Y,Pb,Pr;

	// Bound check.
	if (n < size)
	{
		Y = a11 * R[n] + a12 * G[n] + a13 * B[n];
		Pb = a21 * R[n] + a22 * G[n] + a23 * B[n];
		Pr = a31 * R[n] + a32 * G[n] + a33 * B[n];
		if (Y > 100 || Pb > 100 || Pr > 100)
			Bin[n]=255;
		else
			Bin[n]=0;
	}
}