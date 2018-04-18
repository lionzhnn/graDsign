#include <stdio.h>
#include <malloc.h>
#include <math.h>
#include "bs1.h"
#define y0 100
#define x0 94.4221
#define z0 82.5221
int main(int argc,char *argv[])
{
	double *LrBuff,*arBuff,*brBuff;
	double *LcBuff,*acBuff,*bcBuff;
	double lMSE,lPSNR,lSSIM;//l
	double aMSE,aPSNR,aSSIM;//a
	double bMSE,bPSNR,bSSIM;//b
	int weight,height; //文件大小
	//获取文件大小
	//argv[1] ---参考图像 argv[2] ---测试图像
	FILE *inputr=fopen(argv[1],"rb");
	if(inputr==NULL) //如果失败了
	{
		printf("没有找到文件\n");
		exit(1); //中止程序
	}
	fseek(inputr,0,0);
	BITMAPFILEHEADER file_h;
	BITMAPINFOHEADER info_h;
	fread(&file_h,1,sizeof(file_h),inputr);
	fread(&info_h,1,sizeof(info_h),inputr);
	weight=info_h.biWidth;
	height=info_h.biHeight;
	fclose(inputr);
	//开辟lab空间
	LrBuff=new double[weight*height];
	arBuff=new double[weight*height];
	brBuff=new double[weight*height];
	LcBuff=new double[weight*height];
	acBuff=new double[weight*height];
	bcBuff=new double[weight*height];
	//预处理
	preprocessor(argv[1],height,weight,LrBuff,arBuff,brBuff);
	preprocessor(argv[2],height,weight,LcBuff,acBuff,bcBuff);
	//MSE计算
	lMSE=MSECompute(LrBuff,LcBuff,weight,height);
	aMSE=MSECompute(arBuff,acBuff,weight,height);
	bMSE=MSECompute(brBuff,bcBuff,weight,height);
	//PSNR计算
	lPSNR=PSNRCompute(lMSE);
	aPSNR=PSNRCompute(aMSE);
	bPSNR=PSNRCompute(bMSE);
	//SSIM计算
	lSSIM=SSIMCompute(LrBuff,LcBuff,weight,height);
	aSSIM=SSIMCompute(arBuff,acBuff,weight,height);
	bSSIM=SSIMCompute(brBuff,bcBuff,weight,height);
	printf("MSE=%f\n",(lMSE+aMSE+bMSE)/3); //权重相等
	printf("PSNR=%f\n",(lPSNR+aPSNR+bPSNR)/3);
	printf("SSIM=%f\n",(lSSIM+aSSIM+bSSIM)/3);
	delete[] LrBuff;
	delete[] arBuff;
	delete[] brBuff;
	delete[] LcBuff;
	delete[] acBuff;
	delete[] bcBuff;
	return 0;
}
