#include <iostream>
#include <fstream>
#include <string>
#include <time.h>
#include "CImg.h"
#include <CL/cl.h>

using namespace cimg_library;

#ifndef cimg_imagepath
#define cimg_imagepath "img/"
#endif

void randomizeArray(cl_int* data, size_t vectorSize, int rate)
{
	srand(rate * time(NULL));
	for (size_t i = 0; i < vectorSize; ++i)
	{
		data[i] = rand() % 10;
	}
}

int main(int argc, char **argv)
{
	cl_int error = CL_SUCCESS;

	// Get platform number.
	cl_uint platformNumber = 0;

	error = clGetPlatformIDs(0, NULL, &platformNumber);

	if (0 == platformNumber)
	{
		std::cout << "No OpenCL platforms found." << std::endl;

		return 0;
	}

	// Get platform identifiers.
	cl_platform_id* platformIds = new cl_platform_id[platformNumber];

	error = clGetPlatformIDs(platformNumber, platformIds, NULL);

	// Get platform info.
	for (cl_uint i = 0; i < platformNumber; ++i)
	{
		char name[1024] = { '\0' };

		std::cout << "Platform:\t" << i << std::endl;

		error = clGetPlatformInfo(platformIds[i], CL_PLATFORM_NAME, 1024, &name, NULL);

		std::cout << "Name:\t\t" << name << std::endl;

		error = clGetPlatformInfo(platformIds[i], CL_PLATFORM_VENDOR, 1024, &name, NULL);

		std::cout << "Vendor:\t\t" << name << std::endl;

		std::cout << std::endl;
	}

	// Get device count.
	cl_uint deviceNumber;

	error = clGetDeviceIDs(platformIds[2], CL_DEVICE_TYPE_GPU, 0, NULL, &deviceNumber);

	if (0 == deviceNumber)
	{
		std::cout << "No OpenCL devices found on platform " << 1 << "." << std::endl;
	}

	// Get device identifiers.
	cl_device_id* deviceIds = new cl_device_id[deviceNumber];

	error = clGetDeviceIDs(platformIds[2], CL_DEVICE_TYPE_GPU, deviceNumber, deviceIds, &deviceNumber);

	// Get device info.
	for (cl_uint i = 0; i < deviceNumber; ++i)
	{
		char name[1024] = { '\0' };

		std::cout << "Device:\t\t" << i << std::endl;

		error = clGetDeviceInfo(deviceIds[i], CL_DEVICE_NAME, 1024, &name, NULL);

		std::cout << "Name:\t\t" << name << std::endl;

		error = clGetDeviceInfo(deviceIds[i], CL_DEVICE_VENDOR, 1024, &name, NULL);

		std::cout << "Vendor:\t\t" << name << std::endl;

		error = clGetDeviceInfo(deviceIds[i], CL_DEVICE_VERSION, 1024, &name, NULL);

		std::cout << "Version:\t" << name << std::endl;
	}

	std::cout << std::endl;

	// Define number of local workers
	size_t localWorkSize = 32;
	time_t tstart, tend;

	// Allocate and initialize host arrays
	const size_t hVectorSize = 1920;
	const size_t wVectorSize = 1080;
	const size_t matrixSize = wVectorSize * hVectorSize;
	unsigned int matrix_mem_size = sizeof(cl_int) * matrixSize;
	int matrix_size = hVectorSize*wVectorSize;
	int mem_matrix_size = sizeof(cl_int) * matrix_size;
	cl_int* R_A = (cl_int*)malloc(mem_matrix_size);
	cl_int* R_B = (cl_int*)malloc(mem_matrix_size);
	cl_int* G_A = (cl_int*)malloc(mem_matrix_size);
	cl_int* G_B = (cl_int*)malloc(mem_matrix_size);
	cl_int* B_A = (cl_int*)malloc(mem_matrix_size);
	cl_int* B_B = (cl_int*)malloc(mem_matrix_size);
	cl_int* R_C = (cl_int*)malloc(mem_matrix_size);
	cl_int* G_C = (cl_int*)malloc(mem_matrix_size);
	cl_int* B_C = (cl_int*)malloc(mem_matrix_size);



	// Read image filename from the command line (or set it to "img/parrot.ppm" if option '-i' is not provided)
	const char* file_a = cimg_option("-i", cimg_imagepath "a.bmp", "Input image");
	const char* file_b = cimg_option("-i", cimg_imagepath "b.bmp", "Input image");

	// Load an image, transform it to a color image (if necessary) and blur it with the standard deviation sigma
	const CImg<unsigned char> image_a = CImg<>(file_a).normalize(0, 255);
	const CImg<unsigned char> image_b = CImg<>(file_b).normalize(0, 255);

	// Create two display windo-aw, one for the image, the other for the color profile.
	CImgDisplay main_disp(image_a, "Color image (Try to move mouse pointer over)", 0);

	// Enter event loop. This loop ends when one of the two display window is closed or
	// when the keys 'ESC' or 'Q' are pressed.

	const int width = image_a.width();
	const int height = image_a.height();
	//unsigned short val_red_a[1080][1920];
	//unsigned short val_green_a[1080][1920];
	//unsigned short val_blue_a[1080][1920];
	//unsigned short val_red_b[1080][1920];
	//unsigned short val_green_b[1080][1920];
	//unsigned short val_blue_b[1080][1920];

	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			R_A[x*height+y] = image_a(x, y, 0);
			G_A[x*height + y] = image_a(x, y, 1),
			B_A[x*height + y] = image_a(x, y, 2);
			R_B[x*height + y] = image_b(x, y, 0);
			G_B[x*height + y] = image_b(x, y, 1),
			B_B[x*height + y] = image_b(x, y, 2);
		}
	}
	std::cout << "Red A:     " << R_A[50000] << std::endl;
	std::cout << "Red B:     " << R_B[50000] << std::endl;
	std::cout << "Green A:   " << G_A[50000] << std::endl;
	std::cout << "Green B:   " << G_B[50000] << std::endl;
	std::cout << "Blue A:    " << B_A[50000] << std::endl;
	std::cout << "Blue B:    " << B_B[50000] << std::endl;
	//std::cout << "Green: " << val_green_a[100][100] << std::endl;
	//std::cout << "Blue:  " << val_blue_a[100][100] << std::endl;

	// Create the OpenCL context.
	cl_context context = clCreateContext(0, deviceNumber, deviceIds, NULL, NULL, NULL);

	if (NULL == context)
	{
		std::cout << "Failed to create OpenCL context." << std::endl;
	}

	// Create a command-queue
	cl_command_queue commandQueue = clCreateCommandQueue(context, deviceIds[0], 0, &error);

	// Allocate the OpenCL buffer memory objects for source and result on the device.
	cl_mem bufferA = clCreateBuffer(context, CL_MEM_READ_ONLY, matrix_mem_size, NULL, &error);
	cl_mem bufferB = clCreateBuffer(context, CL_MEM_READ_ONLY, matrix_mem_size, NULL, &error);
	cl_mem bufferC = clCreateBuffer(context, CL_MEM_WRITE_ONLY, matrix_mem_size, NULL, &error);
	cl_mem bufferD = clCreateBuffer(context, CL_MEM_READ_ONLY, matrix_mem_size, NULL, &error);

	// Read the OpenCL kernel in from source file.
	std::ifstream file(".\\Add.cl", std::ifstream::in);
	std::string str;

	file.seekg(0, std::ios::end);
	size_t programSize = (size_t)file.tellg();

	str.reserve((unsigned int)file.tellg());
	file.seekg(0, std::ios::beg);

	str.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
	const char* source = str.c_str();

	// Create the program.
	cl_program program = clCreateProgramWithSource(context, 1, &source, &programSize, &error);

	error = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);

	// Create the kernel.
	cl_kernel kernel = clCreateKernel(program, "Add", &error);

	// Set the Argument values.
	error = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&bufferA);
	error = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&bufferB);
	error = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&bufferC);
	error = clSetKernelArg(kernel, 3, sizeof(cl_int), (void*)&matrixSize);


	// Asynchronous write of data to GPU device.
	tstart = time(0);
	error = clEnqueueWriteBuffer(commandQueue, bufferA, CL_FALSE, 0, matrix_mem_size, R_A, 0, NULL, NULL);
	error = clEnqueueWriteBuffer(commandQueue, bufferB, CL_FALSE, 0, matrix_mem_size, R_B, 0, NULL, NULL);

	// Launch kernel.
	error = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, &matrixSize, &localWorkSize, 0, NULL, NULL);

	// Read back results and check accumulated errors.
	error = clEnqueueReadBuffer(commandQueue, bufferC, CL_TRUE, 0, matrix_mem_size, R_C, 0, NULL, NULL);
	tend = time(0);

	std::cout << "Red C:     " << R_C[50000] << std::endl;
	std::cout << "Time:    " << difftime(tend, tstart) << std::endl;

	// Asynchronous write of data to GPU device.
	tstart = time(0);
	error = clEnqueueWriteBuffer(commandQueue, bufferA, CL_FALSE, 0, matrix_mem_size, G_A, 0, NULL, NULL);
	error = clEnqueueWriteBuffer(commandQueue, bufferB, CL_FALSE, 0, matrix_mem_size, G_B, 0, NULL, NULL);

	// Launch kernel.
	error = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, &matrixSize, &localWorkSize, 0, NULL, NULL);

	// Read back results and check accumulated errors.
	error = clEnqueueReadBuffer(commandQueue, bufferC, CL_TRUE, 0, matrix_mem_size, G_C, 0, NULL, NULL);
	tend = time(0);

	std::cout << "Green C:   " << G_C[50000] << std::endl;
	std::cout << "Time:    " << difftime(tend, tstart) << std::endl;

	// Asynchronous write of data to GPU device.
	tstart = time(0);
	error = clEnqueueWriteBuffer(commandQueue, bufferA, CL_FALSE, 0, matrix_mem_size, B_A, 0, NULL, NULL);
	error = clEnqueueWriteBuffer(commandQueue, bufferB, CL_FALSE, 0, matrix_mem_size, B_B, 0, NULL, NULL);

	// Launch kernel.
	error = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, &matrixSize, &localWorkSize, 0, NULL, NULL);

	// Read back results and check accumulated errors.
	error = clEnqueueReadBuffer(commandQueue, bufferC, CL_TRUE, 0, matrix_mem_size, B_C, 0, NULL, NULL);
	tend = time(0);

	std::cout << "Blue C:   " << B_C[50000] << std::endl;
	std::cout << "Time:    " << difftime(tend, tstart) << std::endl;

	//cl_int (*ptr_R_A)[wVectorSize][hVectorSize] = &R_A;

	//delete_table(&ptr_R_A, hVectorSize);
	free(R_A);
	free(R_B);
	free(G_A);
	free(G_B);
	free(B_A);
	free(B_B);
	std::cout << "Po usuwaniu" << std::endl;
	cl_int* Y = (cl_int*)malloc(mem_matrix_size);

	// Read the OpenCL kernel in from source file.
	file.close();
	file.open(".\\YCbCr.cl", std::ifstream::in);

	file.seekg(0, std::ios::end);
	programSize = (size_t)file.tellg();

	str.reserve((unsigned int)file.tellg());
	file.seekg(0, std::ios::beg);

	str.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
	source = str.c_str();

	// Create the program.
	program = clCreateProgramWithSource(context, 1, &source, &programSize, &error);

	error = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);

	// Create the kernel.
	kernel = clCreateKernel(program, "YCbCr", &error);

	// Set the Argument values.
	error = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&bufferA);
	error = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&bufferB);
	error = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&bufferD);
	error = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void*)&bufferC);
	error = clSetKernelArg(kernel, 4, sizeof(cl_int), (void*)&matrixSize);

	// Asynchronous write of data to GPU device.
	tstart = time(0);
	error = clEnqueueWriteBuffer(commandQueue, bufferA, CL_FALSE, 0, matrix_mem_size, R_C, 0, NULL, NULL);
	error = clEnqueueWriteBuffer(commandQueue, bufferB, CL_FALSE, 0, matrix_mem_size, G_C, 0, NULL, NULL);
	error = clEnqueueWriteBuffer(commandQueue, bufferD, CL_FALSE, 0, matrix_mem_size, B_C, 0, NULL, NULL);

	// Launch kernel.
	error = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, &matrixSize, &localWorkSize, 0, NULL, NULL);

	// Read back results and check accumulated errors.
	error = clEnqueueReadBuffer(commandQueue, bufferC, CL_TRUE, 0, matrix_mem_size, Y, 0, NULL, NULL);
	tend = time(0);

	std::cout << "Y:   " << Y[50000] << std::endl;
	std::cout << "Time:    " << difftime(tend, tstart) << std::endl;

	//Miejsce na binaryzacje
	cl_int* Bin = (cl_int*)malloc(mem_matrix_size);
	// Read the OpenCL kernel in from source file.
	file.close();
	file.open(".\\Bin.cl", std::ifstream::in);

	file.seekg(0, std::ios::end);
	programSize = (size_t)file.tellg();

	str.reserve((unsigned int)file.tellg());
	file.seekg(0, std::ios::beg);

	str.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
	source = str.c_str();

	// Create the program.
	program = clCreateProgramWithSource(context, 1, &source, &programSize, &error);

	error = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);

	// Create the kernel.
	kernel = clCreateKernel(program, "Bin", &error);

	// Set the Argument values.
	error = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&bufferA);
	error = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&bufferC);
	error = clSetKernelArg(kernel, 2, sizeof(cl_int), (void*)&matrixSize);

	// Asynchronous write of data to GPU device.
	tstart = time(0);
	error = clEnqueueWriteBuffer(commandQueue, bufferA, CL_FALSE, 0, matrix_mem_size, Y, 0, NULL, NULL);

	FILE *f = fopen("outY.ppm", "wb");
	fprintf(f, "P6\n%i %i 255\n", width, height);
	for (int y = 0; y<height; y++)
		for (int x = 0; x<width; x++)
		{
			fputc(Y[y*width + x], f);   // 0 .. 255
			fputc(Y[y*width + x], f); // 0 .. 255
			fputc(Y[y*width + x], f);  // 0 .. 255
		}
	fclose(f);

	// Launch kernel.
	error = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, &matrixSize, &localWorkSize, 0, NULL, NULL);

	// Read back results and check accumulated errors.
	error = clEnqueueReadBuffer(commandQueue, bufferC, CL_TRUE, 0, matrix_mem_size, Bin, 0, NULL, NULL);
	tend = time(0);

	std::cout << "Bin:   " << Bin[50000] << std::endl;
	std::cout << "Time:    " << difftime(tend, tstart) << std::endl;

	// Cleanup and free memory.
	// Try to make bmp 
	FILE *fa = fopen("out.ppm", "wb");
	fprintf(fa, "P6\n%i %i 255\n", width, height);
	for (int y = 0; y<height; y++)
		for (int x = 0; x<width; x++)
		{
			fputc(Bin[y*width + x], fa);   // 0 .. 255
			fputc(Bin[y*width + x], fa); // 0 .. 255
			fputc(Bin[y*width + x], fa);  // 0 .. 255
		}
	fclose(fa);
	//Stop trying

	free(Y);

	clFlush(commandQueue);
	clFinish(commandQueue);
	clReleaseKernel(kernel);
	clReleaseProgram(program);
	clReleaseMemObject(bufferA);
	clReleaseMemObject(bufferB);
	clReleaseMemObject(bufferC);
	clReleaseMemObject(bufferD);
	clReleaseCommandQueue(commandQueue);
	clReleaseContext(context);

	delete[] platformIds;
	delete[] deviceIds;

	while (!main_disp.is_closed() && !main_disp.is_keyESC() && !main_disp.is_keyQ()) {
	}

	// Press Enter, to quit application.
	std::cin.get();

	return 0;

}