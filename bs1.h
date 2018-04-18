#include<windows.h>
double MSECompute(double* refImage,double* comImage,int weight,int height);
double PSNRCompute(double MSE);
double SSIMCompute(double* refImage,double* comImage,int weight,int height);
double f(double q);
int preprocessor(char *filename,int height,int weight,double* LBuff,double* aBuff,double* bBuff);
void Readrgb(FILE* pFile,BITMAPFILEHEADER & file_h,BITMAPINFOHEADER & info_h,unsigned char * rgbDataOut);
