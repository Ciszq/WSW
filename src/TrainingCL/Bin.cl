__kernel void Bin(__global int* Y, __global int* Bin, int size)
{
	// Find position in global arrays.
	int n = get_global_id(0);

	// Bound check.
	if (n < size)
	{
		if (Y[n] > 70)
			Bin[n]=1;
		else
			Bin[n]=0;
	}
}