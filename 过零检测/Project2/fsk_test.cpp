#include "fsk_demode.h"
//#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <assert.h>



int main(int argc, char const *argv[])
{

	fsk_demode_t fsk[8];
	for (int i = 0; i < 8; i++)
	{
		init_demode_fsk(&fsk[i]);
	}
	unsigned char res_number[8][10];
	unsigned char res_time[8][10];
	short src[8][128];
	int flag[8] = { 1,1,1,1,1,1,1,1 };//解调完成标志，为1正在解调，为0解调完成
	int count = 0;//计数，计复制到第几组了
	//fsk_demode_t fsk;
	//init_demode_fsk(&fsk);   //初始化结构体
	short samples_points[30000];
	int srcLen[8] = { 128,128,128,128,128,128,128,128 };
	char err = 0x00;
	int len_num[8];
	int len_time[8];
	int Readlen = 0;//读文档数据长度
	//每次需要放入128个新点，这里需要解决一下
	FILE *fpRead = fopen("D:\\ring_512.txt", "r");

	int aaa;
	while (fscanf(fpRead, "%hd ", &samples_points[Readlen]) != EOF)
	{
		Readlen++;
	}
	while (flag[0] == 1 || flag[1] == 1 || flag[2] == 1 || flag[3] == 1 || flag[4] == 1 || flag[5] == 1 || flag[6] == 1 || flag[7] == 1)
	{
		//if (count >= 70)
		//{
		//	break;
		//}

		for (int k = 0; k < 1; k++)
			memcpy(src[k], samples_points + (128 * count), sizeof(src[k]));
		for (int j = 0; j < 1; j++)
		{
			if (flag[j] == 0)
			{
				continue;
			}
			if (err != 0)
			{
				aaa = 6;
			}
			if (interfunc(src[j], srcLen[j], &fsk[j]) == 1)
			{
				flag[j] = 0;

				
			}
		}
		count++;

	}
	return 0;
}



