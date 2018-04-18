#include<stdio.h>
#include<math.h>
#include <string.h>
#include"bs1.h"
#define MAX 255
#define c1 (0.01*255)*(0.01*255)
#define c2 0.03*255*0.03*255
#define c3 c2/2
//D65时参考值
#define y0 100
#define x0 94.045
#define z0 108.892
//滤波增益
double q1Gan[9]={1.093028,-0.019388,-0.009737,-0.019388,-0.015444,-0.007732,-0.009737,-0.007732,-0.03871};
double q2Gan[9]={0.915619,0.039480,0.000091,0.039480,0.005215,0.000012,0.000091,0.000012,0};
double q3Gan[9]={0.941837,0.028019,0.000012,0.028019,0.002102,0.00001,0.000012,0.000001,0};
bool MakePalette(FILE * pFile,BITMAPFILEHEADER &file_h,BITMAPINFOHEADER & info_h,RGBQUAD *pRGB_out)
{
    if ((file_h.bfOffBits - sizeof(BITMAPFILEHEADER) - info_h.biSize) == sizeof(RGBQUAD)*pow(2.0,info_h.biBitCount))//如果文件头和文件信息头的长度加上调色板的长度等于文件开始到实际数据的偏移量，说明有调色板存在
    {
        fseek(pFile,sizeof(BITMAPFILEHEADER)+info_h.biSize,0);//文件指针定位到调色板位置
        fread(pRGB_out,sizeof(RGBQUAD),(unsigned int)pow(2.0,info_h.biBitCount),pFile);
        return true;
    }else
        return false;
}
//读取rgb数据，bmp中为倒叙的，但是因为是质量评价没有更改
void Readrgb(FILE* pFile,BITMAPFILEHEADER & file_h,BITMAPINFOHEADER & info_h,unsigned char * rgbDataOut)
{
    unsigned char* data=(unsigned char*)malloc(info_h.biHeight*info_h.biWidth*info_h.biBitCount/8);
	FILE *out322=fopen("D:\\360安全浏览器下载\\MyProjects\\text\\temprgbdata.rgb","wb");
	if(out322==NULL) //如果失败了
	{
		printf("没有找到文件");
		exit(1); //中止程序
	}
    fseek(pFile,file_h.bfOffBits,0);
    fread(data,1,info_h.biHeight*info_h.biWidth*info_h.biBitCount/8,pFile);
    int i;
    switch(info_h.biBitCount)
    {
    case 32://最高位为alpha通道，不用保存
        for(i=0;i<info_h.biHeight*info_h.biWidth*info_h.biBitCount/8;i +=4)
        {
            *rgbDataOut = data[i];
            *(rgbDataOut+1) = data[i+1];
            *(rgbDataOut+2) = data[i+2];
            rgbDataOut +=3;
        }
        break;
    case 24://24bit
        memcpy(rgbDataOut,data,info_h.biHeight*info_h.biWidth*3);
		fwrite(rgbDataOut,1,info_h.biHeight*info_h.biWidth*3,out322);
        break;
    case 16://16bit 555
        if(info_h.biCompression == BI_RGB)
        {
            for (i = 0;i < info_h.biHeight*info_h.biWidth*info_h.biBitCount/8;i +=2)
            {
                *rgbDataOut = (data[i]&0x1F)<<3;
                *(rgbDataOut + 1) = ((data[i]&0xE0)>>2) + ((data[i+1]&0x03)<<6);
                *(rgbDataOut + 2) = (data[i+1]&0x7C)<<1;
                rgbDataOut +=3;
            }
        }
        break;
    default://1-8bit
        RGBQUAD *pRGB = (RGBQUAD *)malloc(sizeof(RGBQUAD)*(int)pow(2.0,info_h.biBitCount));
        if(!(MakePalette(pFile,file_h,info_h,pRGB)))
        {
            printf("No palette\n");
        }
        int index,mask;
        for(i=0;i<info_h.biHeight*info_h.biWidth*info_h.biBitCount/8;i++)
        {
            //printf("%4d%",data[i]);
            int shiftCnt=1;
            switch(info_h.biBitCount)
            {
            case 1:
                mask=0x80; //1000 00000
                break;
            case 2:
                mask=0xc0; //1100 0000
                break;
            case 4:
                mask=0xf0; //1111 0000
                break;
            default:
                mask=0xff; //1111 1111
                break;
            }
            while(mask)
            {
                unsigned char index=(mask==0xff?data[i]:((data[i] & mask)>>(8-shiftCnt*info_h.biBitCount)));
                *(rgbDataOut)=pRGB[index].rgbBlue;
                *(rgbDataOut+1)=pRGB[index].rgbGreen;
                *(rgbDataOut+2)=pRGB[index].rgbRed;
                shiftCnt++;
                rgbDataOut+=3;
                if(info_h.biBitCount== 8) 
                    mask=0;
                else mask>>=info_h.biBitCount;
            }
        }
        if(pRGB) free(pRGB);
        //break;
    }
    if(data) free(data);
	fclose(out322);
}
//计算mse
double MSECompute(double* refImage,double* comImage,int weight,int height)
{
	int i,j;
	double sum=0;
	for(i=0;i<height;i++){
		for(j=0;j<weight;j++){
			sum=sqrt((refImage[i*weight+j]-comImage[i*weight+j])*(refImage[i*weight+j]-comImage[i*weight+j]))+sum;
		}
	}
	return sum/(weight*height);
}
//计算psnr
double PSNRCompute(double MSE)
{
	return 10*log10(MAX*MAX/(MSE+1));
}
//改进，提高运行速度
double SSIMCompute(double* refImage,double* comImage,int weight,int height)
{
	int i,j;
	float ur,uc,fr,fc,frc;
	double l,c,s;
	ur=uc=fr=fc=frc=0;
	//均值
	for(i=0;i<height;i++){
		for(j=0;j<weight;j++){
			ur=refImage[i*weight+j]/(double)(weight*height)+ur;
			uc=comImage[i*weight+j]/(double)(weight*height)+uc;
		}
	}
	//方差
	for(i=0;i<height;i++){
		for(j=0;j<weight;j++){
			fr=(refImage[i*weight+j]-ur)*(refImage[i*weight+j]-ur)/(weight*height)+fr;
			fc=(comImage[i*weight+j]-uc)*(comImage[i*weight+j]-uc)/(weight*height)+fc;
			//协方差
			frc=(refImage[i*weight+j]-ur)*(comImage[i*weight+j]-uc)/(weight*height-1)+frc;
		}
	}
	fr=sqrt(fr); 
	fc=sqrt(fc);
	l=(2*ur*uc+c1)/(ur*ur+uc*uc+c1);
	c=(2*fr*fc+c2)/(fr*fr+fc*fc+c2);
	s=(frc+c2)/(fr*fc+c2);
	//结构相似度
	return l*s*c;
}
double f(double q)
{
	if(q>0.0088856)
		return pow(q,(double)1/3);
	else
		return 7.7879*q+16/(double)116;
}
//预处理，获取lab数据
int preprocessor(char *filename,int height,int weight,double* LBuff,double* aBuff,double* bBuff){
	FILE* fp=fopen(filename,"rb");
	if(fp==NULL) //如果失败了
	{
		printf("没有找到文件");
		exit(1); //中止程序
	}
	int i,j;
	unsigned char *rgbBuffer,*rBuffer,*bBuffer,*gBuffer,*temprgb;
	float *lBuffer,*mBuffer,*sBuffer;
	float *q1Buffer,*q2Buffer,*q3Buffer;
	float *xBuffer,*yBuffer,*zBuffer;
	float temp1,temp2,temp3;
	//读取文件头
	BITMAPFILEHEADER file_h;
	BITMAPINFOHEADER info_h;
	fseek(fp,0,SEEK_SET); 
	fread(&file_h,1,sizeof(file_h),fp);
	fread(&info_h,1,sizeof(info_h),fp);
	fseek(fp,0,SEEK_SET);  //光标移到文件开始
    //开辟rgb内存
	rgbBuffer=new unsigned char[height*weight*3];
	rBuffer=new unsigned char[height*weight];
	bBuffer=new unsigned char[height*weight];
	gBuffer=new unsigned char[height*weight];
	//red rgb data from bmp file
	Readrgb(fp,file_h,info_h,rgbBuffer);
    //读取rgb数据
	temprgb=rgbBuffer;
	for (i = 0; i <height; i++){
        for (j = 0; j<weight; j++){
			bBuffer[i*weight+j]=*temprgb;
			gBuffer[i*weight+j]=*(temprgb+1);
			rBuffer[i*weight+j]=*(temprgb+2);
			temprgb=temprgb+3;
		}
	}
	//lms空间转换
	lBuffer=new float[height*weight];
	mBuffer=new float[height*weight];
	sBuffer=new float[height*weight];
	for (i = 0; i <height; i++){
        for (j = 0; j<weight; j++){
			lBuffer[i*weight+j]=0.0209*rBuffer[i*weight+j]+0.0760*gBuffer[i*weight+j]+0.0112*bBuffer[i*weight+j];
			mBuffer[i*weight+j]=0.0079*rBuffer[i*weight+j]+0.0764*gBuffer[i*weight+j]+0.0163*bBuffer[i*weight+j];
			sBuffer[i*weight+j]=0.0009*rBuffer[i*weight+j]+0.0080*gBuffer[i*weight+j]+0.0766*bBuffer[i*weight+j];
		}
	}
	//释放rgb内存
	delete[] rgbBuffer;
	delete[] rBuffer;
	delete[] bBuffer;
	delete[] gBuffer;
	//色彩分离
	q1Buffer=new float[height*weight];
	q2Buffer=new float[height*weight];
	q3Buffer=new float[height*weight];
	for (i = 0; i <height; i++){
        for (j = 0; j<weight; j++){
			q1Buffer[i*weight+j]=0.9900*lBuffer[i*weight+j]-0.0160*mBuffer[i*weight+j]-0.0940*sBuffer[i*weight+j];
			q2Buffer[i*weight+j]=-0.6690*lBuffer[i*weight+j]+0.7420*mBuffer[i*weight+j]-0.0270*sBuffer[i*weight+j];
			q3Buffer[i*weight+j]=-0.2120*lBuffer[i*weight+j]-0.3540*mBuffer[i*weight+j]+0.9110*sBuffer[i*weight+j];
		}
	}
	//释放lms内存
	delete[] lBuffer;
	delete[] mBuffer;
	delete[] sBuffer;
	//空间滤波
	//q1
	for (i = 0; i <height; i++){
		if(i=0){
			for(j=0;j<weight;j++){
				if(j=0){//第一行第一列
					//前一行 
					temp1=q1Buffer[i*weight+j]*q1Gan[0]+q1Buffer[i*weight+j]*q1Gan[1]+q1Buffer[i*weight+j+1]*q1Gan[2];
					//当前行
					temp2=q1Buffer[i*weight+j]*q1Gan[3]+q1Buffer[i*weight+j]*q1Gan[4]+q1Buffer[i*weight+(j+1)]*q1Gan[5];
					//下一行
					temp3=q1Buffer[(i+1)*weight+j]*q1Gan[6]+q1Buffer[(i+1)*weight+j]*q1Gan[7]+q1Buffer[(i+1)*weight+(j+1)]*q1Gan[8];
					q1Buffer[i*weight+j]=temp1+temp2+temp3;
				}
				else if(j=weight-1){//第一行最后一列
					//line one 
					temp1=q1Buffer[i*weight+j-1]*q1Gan[0]+q1Buffer[i*weight+j]*q1Gan[1]+q1Buffer[i*weight+j]*q1Gan[2];
					//line two
					temp2=q1Buffer[i*weight+j-1]*q1Gan[3]+q1Buffer[i*weight+j]*q1Gan[4]+q1Buffer[i*weight+j]*q1Gan[5];
					//line three
					temp3=q1Buffer[(i+1)*weight+j-1]*q1Gan[6]+q1Buffer[(i+1)*weight+j]*q1Gan[7]+q1Buffer[(i+1)*weight+j]*q1Gan[8];
					q1Buffer[i*weight+j]=temp1+temp2+temp3;
				}
				else{
					//line one 
					temp1=q1Buffer[(i-1)*weight+j-1]*q1Gan[0]+q1Buffer[(i-1)*weight+j]*q1Gan[1]+q1Buffer[(i-1)*weight+(j+1)]*q1Gan[2];
					//line two
					temp2=q1Buffer[i*weight+j-1]*q1Gan[3]+q1Buffer[i*weight+j]*q1Gan[4]+q1Buffer[i*weight+(j+1)]*q1Gan[5];
					//line three
					temp3=q1Buffer[(i+1)*weight+j-1]*q1Gan[6]+q1Buffer[(i+1)*weight+j]*q1Gan[7]+q1Buffer[(i+1)*weight+(j+1)]*q1Gan[8];
					q1Buffer[i*weight+j]=temp1+temp2+temp3;
				}
			}
		}
		else if(i=height-1){
			for(j=0;j<weight;j++){
				if(j=0){//最后一行第一列
					//line one 
					temp1=q1Buffer[(i-1)*weight+j]*q1Gan[0]+q1Buffer[(i-1)*weight+j]*q1Gan[1]+q1Buffer[(i-1)*weight+j+1]*q1Gan[2];
					//line two
					temp2=q1Buffer[i*weight+j]*q1Gan[3]+q1Buffer[i*weight+j]*q1Gan[4]+q1Buffer[i*weight+(j+1)]*q1Gan[5];
					//line three
					temp3=q1Buffer[i*weight+j]*q1Gan[6]+q1Buffer[i*weight+j]*q1Gan[7]+q1Buffer[i*weight+(j+1)]*q1Gan[8];
					q1Buffer[i*weight+j]=temp1+temp2+temp3;
				}
				else if(j=weight-1){//最后一行最后一列
					//line one 
					temp1=q1Buffer[(i-1)*weight+j-1]*q1Gan[0]+q1Buffer[(i-1)*weight+j]*q1Gan[1]+q1Buffer[(i-1)*weight+j]*q1Gan[2];
					//line two
					temp2=q1Buffer[i*weight+j-1]*q1Gan[3]+q1Buffer[i*weight+j]*q1Gan[4]+q1Buffer[i*weight+j]*q1Gan[5];
					//line three
					temp3=q1Buffer[i*weight+j-1]*q1Gan[6]+q1Buffer[i*weight+j]*q1Gan[7]+q1Buffer[i*weight+j]*q1Gan[8];
					q1Buffer[i*weight+j]=temp1+temp2+temp3;
				}
				else{
					//line one 
					temp1=q1Buffer[(i-1)*weight+j-1]*q1Gan[0]+q1Buffer[(i-1)*weight+j]*q1Gan[1]+q1Buffer[(i-1)*weight+(j+1)]*q1Gan[2];
					//line two
					temp2=q1Buffer[i*weight+j-1]*q1Gan[3]+q1Buffer[i*weight+j]*q1Gan[4]+q1Buffer[i*weight+(j+1)]*q1Gan[5];
					//line three
					temp3=q1Buffer[(i+1)*weight+j-1]*q1Gan[6]+q1Buffer[(i+1)*weight+j]*q1Gan[7]+q1Buffer[(i+1)*weight+(j+1)]*q1Gan[8];
					q1Buffer[i*weight+j]=temp1+temp2+temp3;
				}
			}
		}
		else{
			for(j=0;j<weight;j++){
				
					//line one 
					temp1=q1Buffer[(i-1)*weight+j-1]*q1Gan[0]+q1Buffer[(i-1)*weight+j]*q1Gan[1]+q1Buffer[(i-1)*weight+(j+1)]*q1Gan[2];
					//line two
					temp2=q1Buffer[i*weight+j-1]*q1Gan[3]+q1Buffer[i*weight+j]*q1Gan[4]+q1Buffer[i*weight+(j+1)]*q1Gan[5];
					//line three
					temp3=q1Buffer[(i+1)*weight+j-1]*q1Gan[6]+q1Buffer[(i+1)*weight+j]*q1Gan[7]+q1Buffer[(i+1)*weight+(j+1)]*q1Gan[8];
					q1Buffer[i*weight+j]=temp1+temp2+temp3;
			}
		}
	}
	//q2
	for (i = 0; i <height; i++){
		if(i=0){
			for(j=0;j<weight;j++){
				if(j=0){//第一行第一列
					//前一行 
					temp1=q2Buffer[i*weight+j]*q2Gan[0]+q2Buffer[i*weight+j]*q2Gan[1]+q2Buffer[i*weight+j+1]*q2Gan[2];
					//当前行
					temp2=q2Buffer[i*weight+j]*q2Gan[3]+q2Buffer[i*weight+j]*q2Gan[4]+q2Buffer[i*weight+(j+1)]*q2Gan[5];
					//下一行
					temp3=q2Buffer[(i+1)*weight+j]*q2Gan[6]+q2Buffer[(i+1)*weight+j]*q2Gan[7]+q2Buffer[(i+1)*weight+(j+1)]*q2Gan[8];
					q2Buffer[i*weight+j]=temp1+temp2+temp3;
				}
				else if(j=weight-1){//第一行最后一列
					//line one 
					temp1=q2Buffer[i*weight+j-1]*q2Gan[0]+q2Buffer[i*weight+j]*q2Gan[1]+q2Buffer[i*weight+j]*q2Gan[2];
					//line two
					temp2=q2Buffer[i*weight+j-1]*q2Gan[3]+q2Buffer[i*weight+j]*q2Gan[4]+q2Buffer[i*weight+j]*q2Gan[5];
					//line three
					temp3=q2Buffer[(i+1)*weight+j-1]*q2Gan[6]+q2Buffer[(i+1)*weight+j]*q2Gan[7]+q2Buffer[(i+1)*weight+j]*q2Gan[8];
					q2Buffer[i*weight+j]=temp1+temp2+temp3;
				}
				else{
					//line one 
					temp1=q2Buffer[(i-1)*weight+j-1]*q2Gan[0]+q2Buffer[(i-1)*weight+j]*q2Gan[1]+q2Buffer[(i-1)*weight+(j+1)]*q2Gan[2];
					//line two
					temp2=q2Buffer[i*weight+j-1]*q2Gan[3]+q2Buffer[i*weight+j]*q2Gan[4]+q2Buffer[i*weight+(j+1)]*q2Gan[5];
					//line three
					temp3=q2Buffer[(i+1)*weight+j-1]*q2Gan[6]+q2Buffer[(i+1)*weight+j]*q2Gan[7]+q2Buffer[(i+1)*weight+(j+1)]*q2Gan[8];
					q2Buffer[i*weight+j]=temp1+temp2+temp3;
				}
			}
		}
		else if(i=height-1){
			for(j=0;j<weight;j++){
				if(j=0){//最后一行第一列
					//line one 
					temp1=q2Buffer[(i-1)*weight+j]*q2Gan[0]+q1Buffer[(i-1)*weight+j]*q2Gan[1]+q2Buffer[(i-1)*weight+j+1]*q2Gan[2];
					//line two
					temp2=q2Buffer[i*weight+j]*q2Gan[3]+q2Buffer[i*weight+j]*q2Gan[4]+q2Buffer[i*weight+(j+1)]*q2Gan[5];
					//line three
					temp3=q2Buffer[i*weight+j]*q2Gan[6]+q2Buffer[i*weight+j]*q2Gan[7]+q2Buffer[i*weight+(j+1)]*q2Gan[8];
					q2Buffer[i*weight+j]=temp1+temp2+temp3;
				}
				else if(j=weight-1){//最后一行最后一列
					//line one 
					temp1=q2Buffer[(i-1)*weight+j-1]*q2Gan[0]+q2Buffer[(i-1)*weight+j]*q2Gan[1]+q2Buffer[(i-1)*weight+j]*q2Gan[2];
					//line two
					temp2=q2Buffer[i*weight+j-1]*q2Gan[3]+q2Buffer[i*weight+j]*q2Gan[4]+q2Buffer[i*weight+j]*q2Gan[5];
					//line three
					temp3=q2Buffer[i*weight+j-1]*q2Gan[6]+q2Buffer[i*weight+j]*q2Gan[7]+q2Buffer[i*weight+j]*q2Gan[8];
					q2Buffer[i*weight+j]=temp1+temp2+temp3;
				}
				else{
					//line one 
					temp1=q2Buffer[(i-1)*weight+j-1]*q2Gan[0]+q2Buffer[(i-1)*weight+j]*q2Gan[1]+q2Buffer[(i-1)*weight+(j+1)]*q2Gan[2];
					//line two
					temp2=q2Buffer[i*weight+j-1]*q2Gan[3]+q2Buffer[i*weight+j]*q2Gan[4]+q2Buffer[i*weight+(j+1)]*q2Gan[5];
					//line three
					temp3=q2Buffer[(i+1)*weight+j-1]*q2Gan[6]+q2Buffer[(i+1)*weight+j]*q2Gan[7]+q2Buffer[(i+1)*weight+(j+1)]*q2Gan[8];
					q2Buffer[i*weight+j]=temp1+temp2+temp3;
				}
			}
		}
		else{
			for(j=0;j<weight;j++){
				
					//line one 
					temp1=q2Buffer[(i-1)*weight+j-1]*q2Gan[0]+q2Buffer[(i-1)*weight+j]*q2Gan[1]+q2Buffer[(i-1)*weight+(j+1)]*q2Gan[2];
					//line two
					temp2=q2Buffer[i*weight+j-1]*q2Gan[3]+q2Buffer[i*weight+j]*q2Gan[4]+q2Buffer[i*weight+(j+1)]*q2Gan[5];
					//line three
					temp3=q2Buffer[(i+1)*weight+j-1]*q2Gan[6]+q2Buffer[(i+1)*weight+j]*q2Gan[7]+q2Buffer[(i+1)*weight+(j+1)]*q2Gan[8];
					q2Buffer[i*weight+j]=temp1+temp2+temp3;
			}
		}
	}
	//q3
	for (i = 0; i <height; i++){
		if(i=0){
			for(j=0;j<weight;j++){
				if(j=0){//第一行第一列
					//前一行 
					temp1=q3Buffer[i*weight+j]*q3Gan[0]+q3Buffer[i*weight+j]*q3Gan[1]+q3Buffer[i*weight+j+1]*q3Gan[2];
					//当前行
					temp2=q3Buffer[i*weight+j]*q3Gan[3]+q3Buffer[i*weight+j]*q3Gan[4]+q3Buffer[i*weight+(j+1)]*q3Gan[5];
					//下一行
					temp3=q3Buffer[(i+1)*weight+j]*q3Gan[6]+q3Buffer[(i+1)*weight+j]*q3Gan[7]+q3Buffer[(i+1)*weight+(j+1)]*q3Gan[8];
					q3Buffer[i*weight+j]=temp1+temp2+temp3;
				}
				else if(j=weight-1){//第一行最后一列
					//line one 
					temp1=q3Buffer[i*weight+j-1]*q3Gan[0]+q3Buffer[i*weight+j]*q3Gan[1]+q3Buffer[i*weight+j]*q3Gan[2];
					//line two
					temp2=q3Buffer[i*weight+j-1]*q3Gan[3]+q3Buffer[i*weight+j]*q3Gan[4]+q3Buffer[i*weight+j]*q3Gan[5];
					//line three
					temp3=q3Buffer[(i+1)*weight+j-1]*q3Gan[6]+q3Buffer[(i+1)*weight+j]*q3Gan[7]+q3Buffer[(i+1)*weight+j]*q3Gan[8];
					q3Buffer[i*weight+j]=temp1+temp2+temp3;
				}
				else{
					//line one 
					temp1=q3Buffer[(i-1)*weight+j-1]*q3Gan[0]+q3Buffer[(i-1)*weight+j]*q3Gan[1]+q3Buffer[(i-1)*weight+(j+1)]*q3Gan[2];
					//line two
					temp2=q3Buffer[i*weight+j-1]*q3Gan[3]+q3Buffer[i*weight+j]*q3Gan[4]+q3Buffer[i*weight+(j+1)]*q3Gan[5];
					//line three
					temp3=q3Buffer[(i+1)*weight+j-1]*q3Gan[6]+q3Buffer[(i+1)*weight+j]*q3Gan[7]+q3Buffer[(i+1)*weight+(j+1)]*q3Gan[8];
					q3Buffer[i*weight+j]=temp1+temp2+temp3;
				}
			}
		}
		else if(i=height-1){
			for(j=0;j<weight;j++){
				if(j=0){//最后一行第一列
					//line one 
					temp1=q3Buffer[(i-1)*weight+j]*q3Gan[0]+q3Buffer[(i-1)*weight+j]*q3Gan[1]+q3Buffer[(i-1)*weight+j+1]*q3Gan[2];
					//line two
					temp2=q3Buffer[i*weight+j]*q3Gan[3]+q3Buffer[i*weight+j]*q3Gan[4]+q3Buffer[i*weight+(j+1)]*q3Gan[5];
					//line three
					temp3=q3Buffer[i*weight+j]*q3Gan[6]+q3Buffer[i*weight+j]*q3Gan[7]+q3Buffer[i*weight+(j+1)]*q3Gan[8];
					q3Buffer[i*weight+j]=temp1+temp2+temp3;
				}
				else if(j=weight-1){//最后一行最后一列
					//line one 
					temp1=q3Buffer[(i-1)*weight+j-1]*q3Gan[0]+q3Buffer[(i-1)*weight+j]*q3Gan[1]+q3Buffer[(i-1)*weight+j]*q3Gan[2];
					//line two
					temp2=q3Buffer[i*weight+j-1]*q3Gan[3]+q3Buffer[i*weight+j]*q3Gan[4]+q3Buffer[i*weight+j]*q3Gan[5];
					//line three
					temp3=q3Buffer[i*weight+j-1]*q3Gan[6]+q3Buffer[i*weight+j]*q3Gan[7]+q3Buffer[i*weight+j]*q3Gan[8];
					q3Buffer[i*weight+j]=temp1+temp2+temp3;
				}
				else{
					//line one 
					temp1=q3Buffer[(i-1)*weight+j-1]*q3Gan[0]+q3Buffer[(i-1)*weight+j]*q3Gan[1]+q3Buffer[(i-1)*weight+(j+1)]*q3Gan[2];
					//line two
					temp2=q3Buffer[i*weight+j-1]*q3Gan[3]+q3Buffer[i*weight+j]*q3Gan[4]+q3Buffer[i*weight+(j+1)]*q3Gan[5];
					//line three
					temp3=q3Buffer[(i+1)*weight+j-1]*q3Gan[6]+q3Buffer[(i+1)*weight+j]*q3Gan[7]+q3Buffer[(i+1)*weight+(j+1)]*q3Gan[8];
					q3Buffer[i*weight+j]=temp1+temp2+temp3;
				}
			}
		}
		else{
			for(j=0;j<weight;j++){
				
					//line one 
					temp1=q3Buffer[(i-1)*weight+j-1]*q3Gan[0]+q3Buffer[(i-1)*weight+j]*q3Gan[1]+q3Buffer[(i-1)*weight+(j+1)]*q3Gan[2];
					//line two
					temp2=q3Buffer[i*weight+j-1]*q3Gan[3]+q3Buffer[i*weight+j]*q3Gan[4]+q3Buffer[i*weight+(j+1)]*q3Gan[5];
					//line three
					temp3=q3Buffer[(i+1)*weight+j-1]*q3Gan[6]+q3Buffer[(i+1)*weight+j]*q3Gan[7]+q3Buffer[(i+1)*weight+(j+1)]*q3Gan[8];
					q3Buffer[i*weight+j]=temp1+temp2+temp3;
			}
		}
	}
    //transform to xyz space
	xBuffer=new float[weight*height];
	yBuffer=new float[weight*height];
	zBuffer=new float[weight*height];
	for (i = 0; i <height; i++){
        for (j = 0; j<weight; j++){
			xBuffer[i*weight+j]=0.6204*q1Buffer[i*weight+j]-1.8704*q2Buffer[i*weight+j]-0.1553*q3Buffer[i*weight+j];
			yBuffer[i*weight+j]=1.3661*q1Buffer[i*weight+j]+0.9316*q2Buffer[i*weight+j]+0.4339*q3Buffer[i*weight+j];
			zBuffer[i*weight+j]=1.5013*q1Buffer[i*weight+j]+1.4176*q2Buffer[i*weight+j]+2.5331*q3Buffer[i*weight+j];
		}
	}
	delete[] q1Buffer;
	delete[] q2Buffer;
	delete[] q3Buffer;
	//compute lab
	int suml=0,suma=0,sumb=0;
	for (i = 0; i <height; i++){
        for (j = 0; j<weight; j++){
			//printf("%d--%d--y=%f--x=%f--z=%f\n",i,j,yBuffer[i*weight+j],xBuffer[i*weight+j],zBuffer[i*weight+j]);
			if((yBuffer[i*weight+j]/y0)>0.008856){ 
				LBuff[i*weight+j]=116*pow(yBuffer[i*weight+j]/y0,(double)1/3)-16;
			}
			else{
				LBuff[i*weight+j]=903.3*pow(yBuffer[i*weight+j]/y0,(double)1/3);
			}
			aBuff[i*weight+j]=500*(f(xBuffer[i*weight+j]/x0)-f(yBuffer[i*weight+j]/y0));
			bBuff[i*weight+j]=200*(f(yBuffer[i*weight+j]/y0)-f(zBuffer[i*weight+j]/z0));
		}
	}
	//free xyz space
	delete[] xBuffer;
	delete[] yBuffer;
	delete[] zBuffer;
	fclose(fp);
    return 0;
}
