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

using namespace std;

void randomizeArray(cl_int* data, size_t vectorSize, int rate)
{
	srand(rate * time(NULL));
	for (size_t i = 0; i < vectorSize; ++i)
	{
		data[i] = rand() % 10;
	}
}

void ExportToBmpFile(const int width, const int height, cl_int* RedMatrix, cl_int* GreenMatrix, cl_int* BlueMatrix, char* filename)
{
	auto fp = fopen(filename, "wb");
	(void)fprintf(fp, "P6\n%d %d\n255\n", width, height);
	for (auto j = 0; j < height; ++j)
	{
		for (auto i = 0; i < width; ++i)
		{
			static unsigned char color[3];
			color[0] = RedMatrix[j*width + i];
			color[1] = GreenMatrix[j*width + i];
			color[2] = BlueMatrix[j*width + i];
			(void)fwrite(color, 1, 3, fp);
		}
	}
	(void)fclose(fp);
}

cl_int initGraphicComponents(cl_int& error, cl_platform_id*& platformIds, cl_uint& deviceNumber, cl_device_id*& deviceIds)
{
	cl_uint platformNumber = 0;

	error = clGetPlatformIDs(0, NULL, &platformNumber);

	if (0 == platformNumber)
	{
		std::cout << "No OpenCL platforms found." << std::endl;
		return true;
	}

	// Get platform identifiers.
	platformIds = new cl_platform_id[platformNumber];

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

	auto platformIndex = 1;
	error = clGetDeviceIDs(platformIds[platformIndex], CL_DEVICE_TYPE_GPU, 0, NULL, &deviceNumber);

	if (0 == deviceNumber)
	{
		std::cout << "No OpenCL devices found on platform " << 1 << "." << std::endl;
	}

	// Get device identifiers.
	deviceIds = new cl_device_id[deviceNumber];

	error = clGetDeviceIDs(platformIds[platformIndex], CL_DEVICE_TYPE_GPU, deviceNumber, deviceIds, &deviceNumber);

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
	return error;
}

int main(int argc, char **argv)
{
	cl_int error;
	cl_platform_id* platformIds;
	cl_uint deviceNumber;
	cl_device_id* deviceIds;

	error = initGraphicComponents(error, platformIds, deviceNumber, deviceIds);

	// Define number of local workers
	size_t localWorkSize = 32;
	time_t tstart, tend;

	// Allocate and initialize host arrays
	const size_t hVectorSize = 1840;
	const size_t wVectorSize = 3264;
	const size_t matrixSize = wVectorSize * hVectorSize;
	unsigned int matrix_mem_size = sizeof(cl_int) * matrixSize;
	int matrix_size = hVectorSize*wVectorSize;
	int mem_matrix_size = sizeof(cl_int) * matrix_size;
	cl_int* FirstImageRed = (cl_int*)malloc(mem_matrix_size);
	cl_int* SecondImageRed = (cl_int*)malloc(mem_matrix_size);
	cl_int* FirstImageGreen = (cl_int*)malloc(mem_matrix_size);
	cl_int* SecondImageGreen = (cl_int*)malloc(mem_matrix_size);
	cl_int* FirstImageBlue = (cl_int*)malloc(mem_matrix_size);
	cl_int* SecondImageBue = (cl_int*)malloc(mem_matrix_size);
	cl_int* ResultImageRed = (cl_int*)malloc(mem_matrix_size);
	cl_int* ResultImageGreen = (cl_int*)malloc(mem_matrix_size);
	cl_int* ResultImageBlue = (cl_int*)malloc(mem_matrix_size);



	// Read image filename from the command line (or set it to "img/parrot.ppm" if option '-i' is not provided)
	const char* file_a = cimg_option("-i", cimg_imagepath "test.bmp", "Input image");
	const char* file_b = cimg_option("-i", cimg_imagepath "test1.bmp", "Input image");
	const CImg<unsigned char> image_a = CImg<>(file_a).normalize(0, 255);
	const CImg<unsigned char> image_b = CImg<>(file_b).normalize(0, 255);

	const int width = image_a.width();
	const int height = image_a.height();

	for (auto x = 0; x < height; x++)
	{
		for (auto y = 0; y < width; y++)
		{
			FirstImageRed[x*width + y] = image_a(y, x, 0);
			FirstImageGreen[x*width + y] = image_a(y, x, 1),
				FirstImageBlue[x*width + y] = image_a(y, x, 2);
			SecondImageRed[x*width + y] = image_b(y, x, 0);
			SecondImageGreen[x*width + y] = image_b(y, x, 1),
				SecondImageBue[x*width + y] = image_b(y, x, 2);
		}
	}

	ExportToBmpFile(width, height, FirstImageRed, FirstImageGreen, FirstImageBlue, "RGBbefore.ppm");

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
	cl_mem bufferE = clCreateBuffer(context, CL_MEM_WRITE_ONLY, matrix_mem_size, NULL, &error);
	cl_mem bufferF = clCreateBuffer(context, CL_MEM_WRITE_ONLY, matrix_mem_size, NULL, &error);

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
	error = clEnqueueWriteBuffer(commandQueue, bufferA, CL_FALSE, 0, matrix_mem_size, FirstImageRed, 0, NULL, NULL);
	error = clEnqueueWriteBuffer(commandQueue, bufferB, CL_FALSE, 0, matrix_mem_size, SecondImageRed, 0, NULL, NULL);

	// Launch kernel.
	error = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, &matrixSize, &localWorkSize, 0, NULL, NULL);

	// Read back results and check accumulated errors.
	error = clEnqueueReadBuffer(commandQueue, bufferC, CL_TRUE, 0, matrix_mem_size, ResultImageRed, 0, NULL, NULL);
	tend = time(0);

	// Asynchronous write of data to GPU device.
	tstart = time(0);
	error = clEnqueueWriteBuffer(commandQueue, bufferA, CL_FALSE, 0, matrix_mem_size, FirstImageGreen, 0, NULL, NULL);
	error = clEnqueueWriteBuffer(commandQueue, bufferB, CL_FALSE, 0, matrix_mem_size, SecondImageGreen, 0, NULL, NULL);

	// Launch kernel.
	error = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, &matrixSize, &localWorkSize, 0, NULL, NULL);

	// Read back results and check accumulated errors.
	error = clEnqueueReadBuffer(commandQueue, bufferC, CL_TRUE, 0, matrix_mem_size, ResultImageGreen, 0, NULL, NULL);
	tend = time(0);

	// Asynchronous write of data to GPU device.
	tstart = time(0);
	error = clEnqueueWriteBuffer(commandQueue, bufferA, CL_FALSE, 0, matrix_mem_size, FirstImageBlue, 0, NULL, NULL);
	error = clEnqueueWriteBuffer(commandQueue, bufferB, CL_FALSE, 0, matrix_mem_size, SecondImageBue, 0, NULL, NULL);

	// Launch kernel.
	error = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, &matrixSize, &localWorkSize, 0, NULL, NULL);

	// Read back results and check accumulated errors.
	error = clEnqueueReadBuffer(commandQueue, bufferC, CL_TRUE, 0, matrix_mem_size, ResultImageBlue, 0, NULL, NULL);
	tend = time(0);

	ExportToBmpFile(width, height, ResultImageRed, ResultImageGreen, ResultImageBlue, "RGB.ppm");

	//delete_table(&ptr_R_A, hVectorSize);
	free(FirstImageRed);
	free(SecondImageRed);
	free(FirstImageGreen);
	free(SecondImageGreen);
	free(FirstImageBlue);
	free(SecondImageBue);
	std::cout << "Po usuwaniu" << std::endl;
	cl_int Y[matrixSize];
	cl_int Pb[matrixSize];
	cl_int Pr[matrixSize];

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
	error = clSetKernelArg(kernel, 4, sizeof(cl_mem), (void*)&bufferE);
	error = clSetKernelArg(kernel, 5, sizeof(cl_mem), (void*)&bufferF);
	error = clSetKernelArg(kernel, 6, sizeof(cl_int), (void*)&matrixSize);

	// Asynchronous write of data to GPU device.
	tstart = time(0);
	error = clEnqueueWriteBuffer(commandQueue, bufferA, CL_FALSE, 0, matrix_mem_size, ResultImageRed, 0, NULL, NULL);
	error = clEnqueueWriteBuffer(commandQueue, bufferB, CL_FALSE, 0, matrix_mem_size, ResultImageGreen, 0, NULL, NULL);
	error = clEnqueueWriteBuffer(commandQueue, bufferD, CL_FALSE, 0, matrix_mem_size, ResultImageBlue, 0, NULL, NULL);

	// Launch kernel.
	error = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, &matrixSize, &localWorkSize, 0, NULL, NULL);

	// Read back results and check accumulated errors.
	error = clEnqueueReadBuffer(commandQueue, bufferC, CL_TRUE, 0, matrix_mem_size, Y, 0, NULL, NULL);
	error = clEnqueueReadBuffer(commandQueue, bufferE, CL_TRUE, 0, matrix_mem_size, Pb, 0, NULL, NULL);
	error = clEnqueueReadBuffer(commandQueue, bufferF, CL_TRUE, 0, matrix_mem_size, Pr, 0, NULL, NULL);
	tend = time(0);

	ExportToBmpFile(width, height, Y, Pb, Pr, "ycbcr.ppm");

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
	error = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&bufferB);
	error = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&bufferD);
	error = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void*)&bufferC);
	error = clSetKernelArg(kernel, 4, sizeof(cl_int), (void*)&matrixSize);

	// Asynchronous write of data to GPU device.
	tstart = time(0);
	error = clEnqueueWriteBuffer(commandQueue, bufferA, CL_FALSE, 0, matrix_mem_size, Y, 0, NULL, NULL);
	error = clEnqueueWriteBuffer(commandQueue, bufferB, CL_FALSE, 0, matrix_mem_size, Pb, 0, NULL, NULL);
	error = clEnqueueWriteBuffer(commandQueue, bufferD, CL_FALSE, 0, matrix_mem_size, Pr, 0, NULL, NULL);

	// Launch kernel.
	error = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, &matrixSize, &localWorkSize, 0, NULL, NULL);

	// Read back results and check accumulated errors.
	error = clEnqueueReadBuffer(commandQueue, bufferC, CL_TRUE, 0, matrix_mem_size, Bin, 0, NULL, NULL);
	tend = time(0);

	std::cout << "Bin:   " << Bin[50000] << std::endl;
	std::cout << "Time:    " << difftime(tend, tstart) << std::endl;

	// Cleanup and free memory.
	// Try to make bmp 
	ExportToBmpFile(width, height, Bin, Bin, Bin, "binary.ppm");
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

	// Press Enter, to quit application.
	std::cin.get();

	return 0;

}
