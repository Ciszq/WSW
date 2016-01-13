__kernel void Bin(__global int* Y, __global int* Pb, __global int* Pr, __global int* Bin, int size)
{
	// Find position in global arrays.
	int n = get_global_id(0);

	// Bound check.
	if (n < size)
	{
		if (Y[n] > 70 || Pb[n] > 70 || Pr[n] > 70)
			Bin[n]=255;
		else
			Bin[n]=0;
	}
}