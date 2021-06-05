
#include "fsk_demode.h"


/*初始化*/
void init_demode_fsk(fsk_demode_t *fsk)
{
	memset(fsk, 0, sizeof(*fsk));//内存清0
}

/*检查是否有到数组末尾*/
unsigned int check_sum_CircleBuffer(unsigned int i)
{
	return ((i + 1) == CIRCLEBUFFER_SIZE) ? 0 : (i + 1);
}

/*从环形数组里读取数据*/
short Read_CircleBuffer_Data(CircleBuffer_t *CircleBuffer)
{
	unsigned int Pos = 0;

	if (CircleBuffer->LeftSize > 0)
	{
		Pos = CircleBuffer->ReadIndex;
		CircleBuffer->ReadIndex = check_sum_CircleBuffer(CircleBuffer->ReadIndex);
		CircleBuffer->LeftSize--;

		return CircleBuffer->CircleBuffer[Pos];
	}
	return 0.0;
}

/*往环形数组内写入数据*/
void  Write_CircleBuffer_Data(CircleBuffer_t *CircleBuffer, short Data)
{
	if (CircleBuffer->LeftSize < CIRCLEBUFFER_SIZE)
	{
		*(CircleBuffer->CircleBuffer + CircleBuffer->WriteIndex) = Data;
		CircleBuffer->WriteIndex = check_sum_CircleBuffer(CircleBuffer->WriteIndex);
		CircleBuffer->LeftSize++;
	}
}

/*解调超时检测*/
//int TimeOut_Check(fsk_demode_t *fsk)
//{
//	if (fsk->Var.timeout_cnt == TIME_OUT)
//	{
//		fsk->Var.timeout_cnt = 0;
//		return 1;
//	}
//	else
//	{
//		fsk->Var.timeout_cnt++;
//		return 0;
//	}
//}

/*数据预处理，3倍插值*/
void data_3inter(fsk_demode_t *fsk, short *src, int srcLen)
{
	for (int i = 0; i < srcLen; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			Write_CircleBuffer_Data(&fsk->ScrBuffer, src[i]);
		}
	}
}


/*响铃检测，响铃部分的相关值也会超过门限，所以通过做积分的方法*/
/*对输入数据求能量，响铃过后会有很长一段几乎无能量的空白信号，通过*/
/*检测到这部分信号，可以判断出响铃部分过去了*/
void ring_detect(fsk_demode_t *fsk)
{
	//积分完一次
	if (fsk->Ring.ring_int_cnt == RING_INT_NUM - 1)
	{
		//第一次响铃后的空白时刻
		if (fsk->Ring.ring_cnt == 0)
		{
			if ((fsk->Ring.ring_int_value <= RING_INT_POW) && (fsk->Ring.ring_int_value >= -RING_INT_POW))
			{
				fsk->Ring.ring_int_value_cnt++;
			}
			else
			{
				fsk->Ring.ring_int_value_cnt = 0;
			}
			//连续3个值都很小，则证明是响铃结束后的那段无信号数据
			if (fsk->Ring.ring_int_value_cnt >= 3)
			{
				fsk->Ring.ring_cnt = 1;
				fsk->Ring.ring_int_value_cnt = 0;
			}
		}

		//第二次响铃
		if (fsk->Ring.ring_cnt == 1)
		{
			if ((fsk->Ring.ring_int_value > RING_INT_POW) || (fsk->Ring.ring_int_value < -RING_INT_POW))
			{
				fsk->Ring.ring_int_value_cnt++;
			}
			else
			{
				fsk->Ring.ring_int_value_cnt = 0;
			}
			//连续3个值都很大，则证明有能量
			if (fsk->Ring.ring_int_value_cnt >= 3)
			{
				fsk->Ring.ring_cnt = 2;
				fsk->Ring.ring_int_value_cnt = 0;
			}
		}

		//第二次响铃后的空白时刻
		if (fsk->Ring.ring_cnt == 2)
		{
			if ((fsk->Ring.ring_int_value <= RING_INT_POW) && (fsk->Ring.ring_int_value >= -RING_INT_POW))
			{
				fsk->Ring.ring_int_value_cnt++;
			}
			else
			{
				fsk->Ring.ring_int_value_cnt = 0;
			}
			//连续3个值都很小，则证明是响铃结束后的那段无信号数据
			if (fsk->Ring.ring_int_value_cnt >= 3)
			{
				fsk->Ring.ring_cnt = 3;
				fsk->Ring.ring_int_value_cnt = 0;
			}
		}

		//第三次检测信号
		if (fsk->Read_Cnt == 134)
		{
			fsk->Read_Cnt = 134;
		}

		if (fsk->Ring.ring_cnt == 3)
		{
			if ((fsk->Ring.ring_int_value > RING_INT_POW) || (fsk->Ring.ring_int_value < -RING_INT_POW))
			{
				fsk->Ring.ring_int_value_cnt++;
			}
			else
			{
				fsk->Ring.ring_int_value_cnt = 0;
			}
			//连续3个值都很大，则证明有能量
			if (fsk->Ring.ring_int_value_cnt >= 3)
			{
				fsk->Ring.ring_cnt = 0;
				fsk->Ring.ring_int_value_cnt = 0;
				fsk->fsk_state = 1;
				fsk->DSP.threshold = 80;//初始化门限值
			}
		}

		fsk->Ring.ring_int_cnt = 0;
		fsk->Ring.ring_int_value = 0;
	}
	else
	{
		fsk->Ring.ring_int_cnt++;
		fsk->Ring.ring_int_value = fsk->Ring.ring_int_value + ((float)(Read_CircleBuffer_Data(&fsk->ScrBuffer)) / 98301);
	}
}

/*数据处理，放大限幅->>微分->>整流->>脉宽调制->>低通滤波*/
int data_process(fsk_demode_t *fsk)
{
	int diff_value = 0;
	int diff_abs_value = 0;
	float temp = 0.0;

	//放大限幅
	fsk->DSP.diff_cur_value = (Read_CircleBuffer_Data(&fsk->ScrBuffer) > 0) ? 100 : -100;
	//微分
	
	diff_value = fsk->DSP.diff_cur_value - fsk->DSP.diff_pre_value;
	//整流
	diff_abs_value = abs(diff_value);
	//脉宽调制
	if (diff_abs_value != 0)
	{
		fsk->DSP.pulse_value = diff_abs_value;
		fsk->DSP.pulse_cnt = 3;
	}
	if (fsk->DSP.pulse_cnt == 0)//没有脉冲，将整流后数据写入缓存
	{
		Write_CircleBuffer_Data(&fsk->DSP.DSPBuffer, diff_abs_value);
	}
	else//有脉冲，将脉冲加宽到3个采样点
	{
		Write_CircleBuffer_Data(&fsk->DSP.DSPBuffer, fsk->DSP.pulse_value);
		fsk->DSP.pulse_cnt--;
	}

	//低通滤波
	if (fsk->DSP.DSPBuffer.LeftSize >= 31)
	{
		for (int i = 0; i < 31; i++)
		{
			temp = temp + (float)(Read_CircleBuffer_Data(&fsk->DSP.DSPBuffer)) * fir_coe[i];
		}
		//回退30个值
		if (fsk->DSP.DSPBuffer.ReadIndex < 30)
		{
			fsk->DSP.DSPBuffer.ReadIndex = CIRCLEBUFFER_SIZE + fsk->DSP.DSPBuffer.ReadIndex - 30;
		}
		else
		{
			fsk->DSP.DSPBuffer.ReadIndex = fsk->DSP.DSPBuffer.ReadIndex - 30;
		}
		
		fsk->DSP.DSPBuffer.LeftSize = fsk->DSP.DSPBuffer.LeftSize + 30;
	}

	fsk->DSP.diff_pre_value = fsk->DSP.diff_cur_value;

	return ((temp - fsk->DSP.threshold) > 0) ? 1 : 0;
}

/*修正过零检测门限，利用负反馈实时修正门限*/
void threshold_cur(fsk_demode_t *fsk)
{
	int data = 0;
	float temp = 0.0;

	
	data = data_process(fsk);
	//每100个点判别一次门限，判别50次
	if ((fsk->DSP.threshold_cnt++%100)==0)
	{
		if (fsk->DSP.threshold_cnt/100 < 10)
		{//前10次修正±1
			temp = 1;
		}
		else if (fsk->DSP.threshold_cnt/100 < 40)
		{//中间30次修正±0.1
			temp = 0.1;
		}
		else if (fsk->DSP.threshold_cnt/100 < 50)
		{//后10次修正±0.01
			temp = 0.01;
		}
		else
		{//然后去检测180个1去
			fsk->Message.message_state = 1;
		}
		//判别10个数是否相等
		if (fsk->DSP.threshold_d<50)
		{
			fsk->DSP.threshold = fsk->DSP.threshold - temp;
		}
		else if (fsk->DSP.threshold_d > 50)
		{
			fsk->DSP.threshold = fsk->DSP.threshold + temp;
		}
		//printf("%3.6f\n",fsk->DSP.threshold);
		fsk->DSP.threshold_d = 0;
	}
	else
	{
		fsk->DSP.threshold_d = fsk->DSP.threshold_d + data;
	}
}

/*180个标志位开始检测，有若干个全0bit时，为连续的1*/
void flag_detect(fsk_demode_t *fsk)
{
	int data = 0;

	data = data_process(fsk);
	if (fsk->Var.flag_cnt == 200)//连续的10个bit为1
	{
		fsk->Var.flag_cnt = 0;
		fsk->Message.message_state = 2;
	}
	else if (data == 0)
	{
		fsk->Var.flag_cnt++;
	}
	else
	{
		fsk->Var.flag_cnt = 0;
	}
}

/*消息字检测，一段时间的标志位后，检测到1个1bit，即检测到0的开始部分了*/
void word_detect(fsk_demode_t *fsk)
{
	int data = 0;

	data = data_process(fsk);

	if (data == 1)
	{
		fsk->Var.word_buf[fsk->Var.word_cnt++] = data;
		//连续的10个1才能判断为是消息字开始了
		if (fsk->Var.word_cnt > 10)
		{
			fsk->Message.message_state = 3;
		}
		else
		{
			fsk->Message.message_state = 2;
		}
	}
	else
	{
		fsk->Var.word_cnt = 0;
		fsk->Message.message_state = 2;
	}
}

/*消息字解调*/
void decode_word(fsk_demode_t *fsk)
{
	unsigned char buf[10] = {0};
	unsigned char temp = 0;
	unsigned char odd = 0;

	fsk->Var.word_buf[fsk->Var.word_cnt++] = data_process(fsk);

	//接受完整的一个10bit消息字后
	if (fsk->Var.word_cnt == 200)
	{
		//10个bit判决01
		for (int i=0; i < 200; i++)
		{
			temp = temp + fsk->Var.word_buf[i];
			//每20次判决一次0、1
			if ((i+1)%20 == 0)
			{
				buf[i/20] = (temp > 10) ? 0 : 1;
				temp = 0;
			}
		}
		fsk->Var.buffer = 0;
		for (int i=0; i < 7; i++)
		{
			//串转并
			fsk->Var.buffer = fsk->Var.buffer + (buf[i + 1] << i);

			//奇偶校验
			odd = odd + buf[i + 1];
		}
		fsk->Var.buf[fsk->Var.buf_pos++] = fsk->Var.buffer;
		fsk->Var.check_sum = fsk->Var.check_sum + fsk->Var.buffer;

		//单个消息字检查错误类型
		if ((buf[0]==1)||(buf[9]==0))	fsk->Status.error = 2;
		
		//检查消息字长度是否错误
		if ((fsk->Var.buf_pos == 2) && (buf[1] > 28))	fsk->Status.error = 3;

		//检查奇偶校验
		if ((fsk->Var.buf_pos > 2) && (odd == buf[8]))
		{
			fsk->Status.error = 1;
		}
		//判断解调完所有消息字了吗
		if (fsk->Var.buf_pos == (fsk->Var.buf[1] + 3))
		{
			//检查校验和是否错误
			if (fsk->Var.check_sum != 0)	fsk->Status.error = 4;

			fsk->Var.buf_pos = 0;
			fsk->Message.message_state = 4;
		}
		else
		{
			fsk->Message.message_state = 2;
		}
		fsk->Var.word_cnt = 0;
	}

}

/*提取数据*/
void get_message(fsk_demode_t *fsk)
{
	fsk->Message.msg_type = fsk->Var.buf[0];
	fsk->Message.msg_len = fsk->Var.buf[1];
	for (int i = 0; i < 8; i++)
	{
		fsk->Message.msg_time[i] = fsk->Var.buf[i+2] - 48;
	}
	for (int i = 0; i < (fsk->Message.msg_len-3); i++)
	{
		fsk->Message.msg_tel[i] = fsk->Var.buf[i + 10] - 48;
	}
	fsk->Message.msg_check = fsk->Var.buf[fsk->Message.msg_len + 2];

	//解调结束
	fsk->fsk_state = 2;

}

/*消息解调*/
void message_process(fsk_demode_t *fsk)
{

	switch (fsk->Message.message_state)
	{
	case 0:
		//300个01修正过零检测门限
		threshold_cur(fsk);

		break;
	case 1:
		//检测180个1
		flag_detect(fsk);

		break;
	case 2:
		//检测消息字
		word_detect(fsk);

		break;
	case 3:
		//解调消息字
		decode_word(fsk);

		break;
	case 4:
		//解调完成,提取数据
		get_message(fsk);

		break;
	default:
		break;
	}

}

int defsk_signal(fsk_demode_t *fsk)
{
	int temp;
	//解调状态机
	switch (fsk->fsk_state)
	{
	case 0:
		//响铃检测
		ring_detect(fsk);

		break;
	case 1:
		//消息解调
		message_process(fsk);
		//temp = data_process(fsk);
		//printf("%d\n",temp);

		break;
	case 2:
		//解调结束,打印解调结果
		fsk->Status.demode_ok = 1;
		printf("defsk finished!\n");
		printf("message_type=%d\n", fsk->Message.msg_type);
		printf("message_len=%d\n", fsk->Message.msg_len);
		printf("message_time=");
		for (int i = 0; i < 8; i++)
		{
			printf(" %d", fsk->Message.msg_time[i]);
		}
		printf("\nmessage_num=");
		for (int i = 0; i < 3; i++)
		{
			printf(" %d", fsk->Message.msg_tel[i]);
		}
		printf("\n错误代码%d\n", fsk->Status.error);

		return 0;

		break;
	default:
		break;
	}
	//循环数组非空判断
	return (fsk->ScrBuffer.LeftSize == 0) ? 0 : 1;

}



/*接口函数*/
int interfunc(short *src, int srcLen, fsk_demode_t *fsk)
{

	fsk->Read_Cnt++;
	if (fsk->Read_Cnt == 119)
	{
		fsk->Read_Cnt = 119;
	}

	//3倍插值->20个点代表1bit
	data_3inter(fsk, src, srcLen);

	//循环求解将环形数组内的数据解调完毕
	while (defsk_signal(fsk));

	//如果解调完成
	if (fsk->Status.demode_ok == 1)
	{
		return 1;					//解调结束
	}
	return 0;						//正在解调
}
