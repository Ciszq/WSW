#include <iostream>
#include <fstream>
#include <string>
#include "CImg.h"
#include <CL/cl.h>

using namespace cimg_library;

#ifndef cimg_imagepath
#define cimg_imagepath "img/"
#endif

using namespace std;

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
		cout << "No OpenCL platforms found." << endl;
		return true;
	}

	// Get platform identifiers.
	platformIds = new cl_platform_id[platformNumber];

	error = clGetPlatformIDs(platformNumber, platformIds, NULL);

	// Get platform info.
	for (cl_uint i = 0; i < platformNumber; ++i)
	{
		char name[1024] = { '\0' };

		cout << "Platform:\t" << i << endl;

		error = clGetPlatformInfo(platformIds[i], CL_PLATFORM_NAME, 1024, &name, NULL);

		cout << "Name:\t\t" << name << endl;

		error = clGetPlatformInfo(platformIds[i], CL_PLATFORM_VENDOR, 1024, &name, NULL);

		cout << "Vendor:\t\t" << name << endl;

		cout << endl;
	}

	// Get device count.

	auto platformIndex = 1;
	error = clGetDeviceIDs(platformIds[platformIndex], CL_DEVICE_TYPE_GPU, 0, NULL, &deviceNumber);

	if (0 == deviceNumber)
	{
		cout << "No OpenCL devices found on platform " << 1 << "." << endl;
	}

	// Get device identifiers.
	deviceIds = new cl_device_id[deviceNumber];

	error = clGetDeviceIDs(platformIds[platformIndex], CL_DEVICE_TYPE_GPU, deviceNumber, deviceIds, &deviceNumber);

	// Get device info.
	for (cl_uint i = 0; i < deviceNumber; ++i)
	{
		char name[1024] = { '\0' };

		cout << "Device:\t\t" << i << endl;

		error = clGetDeviceInfo(deviceIds[i], CL_DEVICE_NAME, 1024, &name, NULL);

		cout << "Name:\t\t" << name << endl;

		error = clGetDeviceInfo(deviceIds[i], CL_DEVICE_VENDOR, 1024, &name, NULL);

		cout << "Vendor:\t\t" << name << endl;

		error = clGetDeviceInfo(deviceIds[i], CL_DEVICE_VERSION, 1024, &name, NULL);

		cout << "Version:\t" << name << endl;
	}

	cout << endl;
	return error;
}

int main(int argc, char **argv)
{
	cl_int error;
	cl_platform_id* platformIds;
	cl_uint deviceNumber;
	cl_device_id* deviceIds;

	error = initGraphicComponents(error, platformIds, deviceNumber, deviceIds);

	size_t localWorkSize = 32;

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

	cl_context context = clCreateContext(0, deviceNumber, deviceIds, NULL, NULL, NULL);

	if (NULL == context)
	{
		cout << "Failed to create OpenCL context." << endl;
	}

	cl_command_queue commandQueue = clCreateCommandQueue(context, deviceIds[0], 0, &error);

	cl_mem bufferA = clCreateBuffer(context, CL_MEM_READ_ONLY, matrix_mem_size, NULL, &error);
	cl_mem bufferB = clCreateBuffer(context, CL_MEM_READ_ONLY, matrix_mem_size, NULL, &error);
	cl_mem bufferC = clCreateBuffer(context, CL_MEM_WRITE_ONLY, matrix_mem_size, NULL, &error);
	cl_mem bufferD = clCreateBuffer(context, CL_MEM_READ_ONLY, matrix_mem_size, NULL, &error);

	ifstream file(".\\Add.cl", ifstream::in);
	string str;
	file.seekg(0, ios::end);
	size_t programSize = (size_t)file.tellg();
	str.reserve((unsigned int)file.tellg());
	file.seekg(0, ios::beg);
	str.assign(istreambuf_iterator<char>(file), istreambuf_iterator<char>());
	const char* source = str.c_str();

	cl_program program = clCreateProgramWithSource(context, 1, &source, &programSize, &error);
	error = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	cl_kernel kernel = clCreateKernel(program, "Add", &error);
	error = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&bufferA);
	error = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&bufferB);
	error = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&bufferC);
	error = clSetKernelArg(kernel, 3, sizeof(cl_int), (void*)&matrixSize);
	error = clEnqueueWriteBuffer(commandQueue, bufferA, CL_FALSE, 0, matrix_mem_size, FirstImageRed, 0, NULL, NULL);
	error = clEnqueueWriteBuffer(commandQueue, bufferB, CL_FALSE, 0, matrix_mem_size, SecondImageRed, 0, NULL, NULL);
	error = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, &matrixSize, &localWorkSize, 0, NULL, NULL);
	error = clEnqueueReadBuffer(commandQueue, bufferC, CL_TRUE, 0, matrix_mem_size, ResultImageRed, 0, NULL, NULL);
	error = clEnqueueWriteBuffer(commandQueue, bufferA, CL_FALSE, 0, matrix_mem_size, FirstImageGreen, 0, NULL, NULL);
	error = clEnqueueWriteBuffer(commandQueue, bufferB, CL_FALSE, 0, matrix_mem_size, SecondImageGreen, 0, NULL, NULL);
	error = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, &matrixSize, &localWorkSize, 0, NULL, NULL);
	error = clEnqueueReadBuffer(commandQueue, bufferC, CL_TRUE, 0, matrix_mem_size, ResultImageGreen, 0, NULL, NULL);
	error = clEnqueueWriteBuffer(commandQueue, bufferA, CL_FALSE, 0, matrix_mem_size, FirstImageBlue, 0, NULL, NULL);
	error = clEnqueueWriteBuffer(commandQueue, bufferB, CL_FALSE, 0, matrix_mem_size, SecondImageBue, 0, NULL, NULL);
	error = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, &matrixSize, &localWorkSize, 0, NULL, NULL);
	error = clEnqueueReadBuffer(commandQueue, bufferC, CL_TRUE, 0, matrix_mem_size, ResultImageBlue, 0, NULL, NULL);

	ExportToBmpFile(width, height, ResultImageRed, ResultImageGreen, ResultImageBlue, "RGB.ppm");

	free(FirstImageRed);
	free(SecondImageRed);
	free(FirstImageGreen);
	free(SecondImageGreen);
	free(FirstImageBlue);
	free(SecondImageBue);
	file.close();

	cl_int Bin[matrixSize];

	file.open(".\\YCbCr.cl", ifstream::in);
	file.seekg(0, ios::end);
	programSize = (size_t)file.tellg();
	str.reserve((unsigned int)file.tellg());
	file.seekg(0, ios::beg);
	str.assign(istreambuf_iterator<char>(file), istreambuf_iterator<char>());
	source = str.c_str();

	program = clCreateProgramWithSource(context, 1, &source, &programSize, &error);
	error = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	kernel = clCreateKernel(program, "YCbCr", &error);
	error = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&bufferA);
	error = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&bufferB);
	error = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&bufferD);
	error = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void*)&bufferC);
	error = clSetKernelArg(kernel, 4, sizeof(cl_int), (void*)&matrixSize);
	error = clEnqueueWriteBuffer(commandQueue, bufferA, CL_FALSE, 0, matrix_mem_size, ResultImageRed, 0, NULL, NULL);
	error = clEnqueueWriteBuffer(commandQueue, bufferB, CL_FALSE, 0, matrix_mem_size, ResultImageGreen, 0, NULL, NULL);
	error = clEnqueueWriteBuffer(commandQueue, bufferD, CL_FALSE, 0, matrix_mem_size, ResultImageBlue, 0, NULL, NULL);
	error = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, &matrixSize, &localWorkSize, 0, NULL, NULL);
	error = clEnqueueReadBuffer(commandQueue, bufferC, CL_TRUE, 0, matrix_mem_size, Bin, 0, NULL, NULL);

	auto p = find(Bin, Bin+ matrixSize - 1, 255);
	if (p != Bin + matrixSize)
		std::cout << "Element found in myints: " << *p << '\n';
	ExportToBmpFile(width, height, Bin, Bin, Bin, "binary.ppm");
	free(Bin);

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
	cin.get();

	return 0;
}
