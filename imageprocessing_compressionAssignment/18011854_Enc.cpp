#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <math.h>
#define QP 5

//단독으로 사용할 땐 main(), 프로젝트 내에서 사용할 땐 enc()로 설정해주세요.
int enc(int ratio)
{

	BITMAPFILEHEADER bmpFile;
	BITMAPINFOHEADER bmpInfo;

	FILE* inputFile = fopen("AICenterY.bmp", "rb");
	FILE* bitOut = fopen("bitstream.txt", "w");
	FILE* outputFile = fopen("result_img/reconEnc(ratio_512).bmp", "wb");

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


	int k = 0;
	unsigned char* inputImg = NULL, * outputImg = NULL, * outputBit = NULL;
	double mse = 0.0, psnr, A, B, C;
	int* Y1, * Y2, * Y3;
	double *Y_down, * Y_up,sum,* upsampling_kernel;

	inputImg = (unsigned char*)malloc(sizeof(unsigned char) * size);
	outputImg = (unsigned char*)malloc(sizeof(unsigned char) * size);
	outputBit = (unsigned char*)malloc(sizeof(unsigned char) * height2 * width2 * 3);

	upsampling_kernel = (double*)malloc(sizeof(double) * ratio * ratio);
	Y1 = (int*)malloc(sizeof(int) * width * height);
	Y_down = (double*)malloc(sizeof(double) * width2 * height2);
	Y_up = (double*)malloc(sizeof(double) * width * height);
	Y2 = (int*)malloc(sizeof(int) * width2 * height2);
	Y3 = (int*)malloc(sizeof(int) * (width2 + 1) * (height2 + 1));
	

	fread(inputImg, sizeof(unsigned char), size, inputFile);


	//원본 이미지 파일 호출
	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++) 
		{
			Y1[j * width + i] = inputImg[j * stride + i * 3];
		}
	}

	//Downsampling
	for (int j = 0; j < height2; j++)
	{
		for (int i = 0; i < width2; i++)
		{
			sum = 0.0;
			for (int y = 0; y < ratio; y++)
			{
				for (int x = 0; x < ratio; x++)
				{
					sum += Y1[(j * ratio + y) * height + i * ratio + x];
				}
			}
			Y_down[j * width2 + i] = (unsigned char)(sum / (ratio * ratio));
		}
	}

	
	//압축, quantization과 fixed-length code에서 나오는 에러를 고려하여 prediction
	for (int j = 0; j < height2; j++) 
	{
		k = 128;
		for (int i = 0; i < width2; i++)
		{
			Y2[j * width2 + i] = Y_down[j * width2 + i] - k;

			if (Y2[j * width2 + i] / QP < -3)k = (-4) * QP + k;
			else if (Y2[j * width2 + i] / QP > 2)k = 3 * QP + k;
			else k = (Y2[j * width2 + i] / QP) * QP + k;

			Y2[j * width2 + i] /= QP;
		}
	}

	//fixed-length code 적용
	for (int j = 0; j < height2; j++) 
	{
		for (int i = 0; i < width2; i++) 
		{
			if (Y2[j * width2 + i] < -3)
			{
				outputBit[j * width2 * 3 + i * 3 + 0] = 0 + '0';
				outputBit[j * width2 * 3 + i * 3 + 1] = 0 + '0';
				outputBit[j * width2 * 3 + i * 3 + 2] = 0 + '0';
			}
			else if (Y2[j * width2 + i] == -3)
			{
				outputBit[j * width2 * 3 + i * 3 + 0] = 0 + '0';
				outputBit[j * width2 * 3 + i * 3 + 1] = 0 + '0';
				outputBit[j * width2 * 3 + i * 3 + 2] = 1 + '0';
			}
			else if (Y2[j * width2 + i] == -2)
			{
				outputBit[j * width2 * 3 + i * 3 + 0] = 0 + '0';
				outputBit[j * width2 * 3 + i * 3 + 1] = 1 + '0';
				outputBit[j * width2 * 3 + i * 3 + 2] = 0 + '0';
			}
			else if (Y2[j * width2 + i] == -1)
			{
				outputBit[j * width2 * 3 + i * 3 + 0] = 0 + '0';
				outputBit[j * width2 * 3 + i * 3 + 1] = 1 + '0';
				outputBit[j * width2 * 3 + i * 3 + 2] = 1 + '0';
			}
			else if (Y2[j * width2 + i] == 0)
			{
				outputBit[j * width2 * 3 + i * 3 + 0] = 1 + '0';
				outputBit[j * width2 * 3 + i * 3 + 1] = 0 + '0';
				outputBit[j * width2 * 3 + i * 3 + 2] = 0 + '0';
			}
			else if (Y2[j * width2 + i] == 1)
			{
				outputBit[j * width2 * 3 + i * 3 + 0] = 1 + '0';
				outputBit[j * width2 * 3 + i * 3 + 1] = 0 + '0';
				outputBit[j * width2 * 3 + i * 3 + 2] = 1 + '0';
			}
			else if (Y2[j * width2 + i] == 2)
			{
				outputBit[j * width2 * 3 + i * 3 + 0] = 1 + '0';
				outputBit[j * width2 * 3 + i * 3 + 1] = 1 + '0';
				outputBit[j * width2 * 3 + i * 3 + 2] = 0 + '0';
			}
			else if (Y2[j * width2 + i] > 2)
			{
				outputBit[j * width2 * 3 + i * 3 + 0] = 1 + '0';
				outputBit[j * width2 * 3 + i * 3 + 1] = 1 + '0';
				outputBit[j * width2 * 3 + i * 3 + 2] = 1 + '0';
			}
		}
	}
	
	//bitstream 파일 저장
	fwrite(outputBit, sizeof(unsigned char), height2 * width2 * 3, bitOut);
	
	//encoder내에서도 fixed-length code 구현
	for (int j = 0; j < height2; j++) 
	{
		for (int i = 0; i < width2; i++) 
		{
			if (Y2[j * width2 + i] < -3)Y2[j * width2 + i] = -4;
			if (Y2[j * width2 + i] > 2)Y2[j * width2 + i] = 3;
		}
	}

	//inverse quantization 
	for (int j = 0; j < height2; j++) 
	{
		k = 128;
		for (int i = 0; i < width2; i++) 
		{
			Y3[j * width2 + i] = Y2[j * width2 + i] * QP;
			Y3[j * width2 + i] = Y3[j * width2 + i] + k;
			k = Y3[j * width2 + i];

			Y3[j * width2 + i] = Y3[j * width2 + i] > 255 ? 255 : (Y3[j * width2 + i] < 0 ? 0 : Y3[j * width2 + i]);
		}
	}

	//패딩
	for (int j = 0; j < height2 + 1; j++)
	{
		for (int i = 0; i < width2 + 1; i++)
		{
			if (i >= width2) Y3[j * width2 + i] = Y3[j * width2 + i - 1];
			if (j >= height2) Y3[j * width2 + i] = Y3[(j - 1) * width2 + i];
		}
	}

	//Upsampling
	for (int j = 0; j < height2; j++)
	{
		for (int i = 0; i < width2; i++)
		{
			A = Y3[j * width2 + i];
			B = Y3[j * width2 + i + 1];
			C = Y3[(j + 1) * width2 + i];
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

	//성능평가를 위해 psnr 계산
	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			Y_up[j * width + i] = Y_up[j * width + i] > 255 ? 255 : (Y_up[j * width + i] < 0 ? 0 : (int)Y_up[j * width + i]);
			mse += (double)((Y_up[j * width + i] - Y1[j * width + i]) * (Y_up[j * width + i] - Y1[j * width + i]));
			outputImg[j * stride + i * 3 + 2] = outputImg[j * stride + i * 3 + 1] = outputImg[j * stride + i * 3 + 0] = Y_up[j * width + i];
		}
	}
	mse /= (width * height);
	psnr = mse != 0.0 ? 10.0 * log10(255 * 255 / mse) : 99.99;
	printf("Encoder MSE = %.2lf\nEncoder PSNR = %.2lf\n", mse, psnr);

	//reconEnc 이미지 파일 저장
	fwrite(&bmpFile, sizeof(BITMAPFILEHEADER), 1, outputFile);
	fwrite(&bmpInfo, sizeof(BITMAPINFOHEADER), 1, outputFile);
	fwrite(outputImg, sizeof(unsigned char), size, outputFile);


	free(outputImg);
	free(inputImg);
	free(Y1);
	free(Y2);
	free(Y3);
	free(outputBit);
	fclose(inputFile);
	fclose(outputFile);
	fclose(bitOut);


	return 0;
}