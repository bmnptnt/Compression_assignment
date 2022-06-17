#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#define QP 5

//단독으로 사용할 땐 main(), 프로젝트 내에서 사용할 땐 dec()로 설정해주세요.
int dec(int ratio) {
	BITMAPFILEHEADER bmpFile;
	BITMAPINFOHEADER bmpInfo;

	FILE* bitIn = fopen("bitstream.txt", "r");
	FILE* inputFile = fopen("GateY.bmp", "rb");
	FILE* outputFile = fopen("result_img/reconDec(ratio_512).bmp", "wb");

	fread(&bmpFile, sizeof(BITMAPFILEHEADER), 1, inputFile);
	fread(&bmpInfo, sizeof(BITMAPINFOHEADER), 1, inputFile);

	int width = bmpInfo.biWidth;
	int height = bmpInfo.biHeight;
	int size = bmpInfo.biSizeImage;
	int bitCnt = bmpInfo.biBitCount;
	int stride = (((bitCnt / 8) * width) + 3) / 4 * 4;

	int width2 = bmpInfo.biWidth / ratio;
	int height2 = bmpInfo.biHeight / ratio;
	int stride2 = (((bitCnt / 8) * width2) + 3) / 4 * 4;
	int size2 = stride2 * height2;

	unsigned char* outputImg = NULL, * inputBit = NULL;
	double A, B, C;
	double* Y_down, * Y_up, sum, * upsampling_kernel;
	int bit = 0,k=0;
	int* Y1,* Y2;


	upsampling_kernel = (double*)malloc(sizeof(double) * ratio * ratio);
	outputImg = (unsigned char*)malloc(sizeof(unsigned char) * size);
	inputBit = (unsigned char*)malloc(sizeof(unsigned char) * height2 * width2 * 3);
	Y1 = (int*)malloc(sizeof(int) * width2 * height2);
	Y2 = (int*)malloc(sizeof(int) * (width2 + 1) * (height2 + 1));
	Y_up = (double*)malloc(sizeof(double) * width * height);


	//bitstream 파일 호출
	fread(inputBit, sizeof(unsigned char), height2 * width2 * 3, bitIn);

	//fix-length code 해석
	for (int j = 0; j < height2; j++) {
		for (int i = 0; i < width2; i++) {
			bit = 0;
			bit = (inputBit[j * width2 * 3 + i * 3 + 0] - '0') * 4 + (inputBit[j * width2 * 3 + i * 3 + 1] - '0') * 2 + inputBit[j * width2 * 3 + i * 3 + 2] - '0';
			if (bit == 0)Y1[j * width2 + i] = -4;
			else if (bit == 1)Y1[j * width2 + i] = -3;
			else if (bit == 2)Y1[j * width2 + i] = -2;
			else if (bit == 3)Y1[j * width2 + i] = -1;
			else if (bit == 4)Y1[j * width2 + i] = 0;
			else if (bit == 5)Y1[j * width2 + i] = 1;
			else if (bit == 6)Y1[j * width2 + i] = 2;
			else if (bit == 7)Y1[j * width2 + i] = 3;
		}
	}

	//복원
	for (int j = 0; j < height2; j++) {
		k = 128;
		for (int i = 0; i < width2; i++) {
			Y2[j * width2 + i] = Y1[j * width2 + i] * QP;
			Y2[j * width2 + i] = Y2[j * width2 + i] + k;
			k = Y2[j * width2 + i];

			Y2[j * width2 + i] = Y2[j * width2 + i] > 255 ? 255 : (Y2[j * width2 + i] < 0 ? 0 : Y2[j * width2 + i]);

		}
	}

	//패딩
	for (int j = 0; j < height2 + 1; j++)
	{
		for (int i = 0; i < width2 + 1; i++)
		{
			if (i >= width2) Y2[j * width2 + i] = Y2[j * width2 + i - 1];
			if (j >= height2) Y2[j * width2 + i] = Y2[(j - 1) * width2 + i];
		}
	}

	//Upsampling
	for (int j = 0; j < height2; j++)
	{
		for (int i = 0; i < width2; i++)
		{
			A = Y2[j * width2 + i];
			B = Y2[j * width2 + i + 1];
			C = Y2[(j + 1) * width2 + i];
			for (int y = 0; y < ratio; y++)
			{
				for (int x = 0; x < ratio; x++)
				{
					upsampling_kernel[y * ratio + x] = (2 * (ratio - x) * (ratio - y) * A + x * (2 * ratio - y) * B + y * (2 * ratio - x) * C) / (2 * ratio * ratio);
				}
			}
			for (int y = 0; y < ratio; y++)
			{
				for (int x = 0; x < ratio; x++)
				{
					Y_up[(j * ratio + y) * height + i * ratio + x] = upsampling_kernel[y * ratio + x];
				}
			}
		}
	}

	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			Y_up[j * width + i] = Y_up[j * width + i] > 255 ? 255 : (Y_up[j * width + i] < 0 ? 0 : (int)Y_up[j * width + i]);
			outputImg[j * stride + i * 3 + 2] = outputImg[j * stride + i * 3 + 1] = outputImg[j * stride + i * 3 + 0] = Y_up[j * width + i];
		}
	}

	fwrite(&bmpFile, sizeof(BITMAPFILEHEADER), 1, outputFile);
	fwrite(&bmpInfo, sizeof(BITMAPINFOHEADER), 1, outputFile);
	fwrite(outputImg, sizeof(unsigned char), size, outputFile);

	free(outputImg);
	free(Y1);
	free(Y2);
	free(inputBit);
	fclose(inputFile);
	fclose(outputFile);
	fclose(bitIn);

	return 0;
}