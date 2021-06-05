
#include "fsk_demode.h"


/*��ʼ��*/
void init_demode_fsk(fsk_demode_t *fsk)
{
	memset(fsk, 0, sizeof(*fsk));//�ڴ���0
}

/*����Ƿ��е�����ĩβ*/
unsigned int check_sum_CircleBuffer(unsigned int i)
{
	return ((i + 1) == CIRCLEBUFFER_SIZE) ? 0 : (i + 1);
}

/*�ӻ����������ȡ����*/
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

/*������������д������*/
void  Write_CircleBuffer_Data(CircleBuffer_t *CircleBuffer, short Data)
{
	if (CircleBuffer->LeftSize < CIRCLEBUFFER_SIZE)
	{
		*(CircleBuffer->CircleBuffer + CircleBuffer->WriteIndex) = Data;
		CircleBuffer->WriteIndex = check_sum_CircleBuffer(CircleBuffer->WriteIndex);
		CircleBuffer->LeftSize++;
	}
}

/*�����ʱ���*/
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

/*����Ԥ����3����ֵ*/
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


/*�����⣬���岿�ֵ����ֵҲ�ᳬ�����ޣ�����ͨ�������ֵķ���*/
/*���������������������������кܳ�һ�μ����������Ŀհ��źţ�ͨ��*/
/*��⵽�ⲿ���źţ������жϳ����岿�ֹ�ȥ��*/
void ring_detect(fsk_demode_t *fsk)
{
	//������һ��
	if (fsk->Ring.ring_int_cnt == RING_INT_NUM - 1)
	{
		//��һ�������Ŀհ�ʱ��
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
			//����3��ֵ����С����֤���������������Ƕ����ź�����
			if (fsk->Ring.ring_int_value_cnt >= 3)
			{
				fsk->Ring.ring_cnt = 1;
				fsk->Ring.ring_int_value_cnt = 0;
			}
		}

		//�ڶ�������
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
			//����3��ֵ���ܴ���֤��������
			if (fsk->Ring.ring_int_value_cnt >= 3)
			{
				fsk->Ring.ring_cnt = 2;
				fsk->Ring.ring_int_value_cnt = 0;
			}
		}

		//�ڶ��������Ŀհ�ʱ��
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
			//����3��ֵ����С����֤���������������Ƕ����ź�����
			if (fsk->Ring.ring_int_value_cnt >= 3)
			{
				fsk->Ring.ring_cnt = 3;
				fsk->Ring.ring_int_value_cnt = 0;
			}
		}

		//�����μ���ź�
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
			//����3��ֵ���ܴ���֤��������
			if (fsk->Ring.ring_int_value_cnt >= 3)
			{
				fsk->Ring.ring_cnt = 0;
				fsk->Ring.ring_int_value_cnt = 0;
				fsk->fsk_state = 1;
				fsk->DSP.threshold = 80;//��ʼ������ֵ
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

/*���ݴ����Ŵ��޷�->>΢��->>����->>�������->>��ͨ�˲�*/
int data_process(fsk_demode_t *fsk)
{
	int diff_value = 0;
	int diff_abs_value = 0;
	float temp = 0.0;

	//�Ŵ��޷�
	fsk->DSP.diff_cur_value = (Read_CircleBuffer_Data(&fsk->ScrBuffer) > 0) ? 100 : -100;
	//΢��
	
	diff_value = fsk->DSP.diff_cur_value - fsk->DSP.diff_pre_value;
	//����
	diff_abs_value = abs(diff_value);
	//�������
	if (diff_abs_value != 0)
	{
		fsk->DSP.pulse_value = diff_abs_value;
		fsk->DSP.pulse_cnt = 3;
	}
	if (fsk->DSP.pulse_cnt == 0)//û�����壬������������д�뻺��
	{
		Write_CircleBuffer_Data(&fsk->DSP.DSPBuffer, diff_abs_value);
	}
	else//�����壬������ӿ�3��������
	{
		Write_CircleBuffer_Data(&fsk->DSP.DSPBuffer, fsk->DSP.pulse_value);
		fsk->DSP.pulse_cnt--;
	}

	//��ͨ�˲�
	if (fsk->DSP.DSPBuffer.LeftSize >= 31)
	{
		for (int i = 0; i < 31; i++)
		{
			temp = temp + (float)(Read_CircleBuffer_Data(&fsk->DSP.DSPBuffer)) * fir_coe[i];
		}
		//����30��ֵ
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

/*�������������ޣ����ø�����ʵʱ��������*/
void threshold_cur(fsk_demode_t *fsk)
{
	int data = 0;
	float temp = 0.0;

	
	data = data_process(fsk);
	//ÿ100�����б�һ�����ޣ��б�50��
	if ((fsk->DSP.threshold_cnt++%100)==0)
	{
		if (fsk->DSP.threshold_cnt/100 < 10)
		{//ǰ10��������1
			temp = 1;
		}
		else if (fsk->DSP.threshold_cnt/100 < 40)
		{//�м�30��������0.1
			temp = 0.1;
		}
		else if (fsk->DSP.threshold_cnt/100 < 50)
		{//��10��������0.01
			temp = 0.01;
		}
		else
		{//Ȼ��ȥ���180��1ȥ
			fsk->Message.message_state = 1;
		}
		//�б�10�����Ƿ����
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

/*180����־λ��ʼ��⣬�����ɸ�ȫ0bitʱ��Ϊ������1*/
void flag_detect(fsk_demode_t *fsk)
{
	int data = 0;

	data = data_process(fsk);
	if (fsk->Var.flag_cnt == 200)//������10��bitΪ1
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

/*��Ϣ�ּ�⣬һ��ʱ��ı�־λ�󣬼�⵽1��1bit������⵽0�Ŀ�ʼ������*/
void word_detect(fsk_demode_t *fsk)
{
	int data = 0;

	data = data_process(fsk);

	if (data == 1)
	{
		fsk->Var.word_buf[fsk->Var.word_cnt++] = data;
		//������10��1�����ж�Ϊ����Ϣ�ֿ�ʼ��
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

/*��Ϣ�ֽ��*/
void decode_word(fsk_demode_t *fsk)
{
	unsigned char buf[10] = {0};
	unsigned char temp = 0;
	unsigned char odd = 0;

	fsk->Var.word_buf[fsk->Var.word_cnt++] = data_process(fsk);

	//����������һ��10bit��Ϣ�ֺ�
	if (fsk->Var.word_cnt == 200)
	{
		//10��bit�о�01
		for (int i=0; i < 200; i++)
		{
			temp = temp + fsk->Var.word_buf[i];
			//ÿ20���о�һ��0��1
			if ((i+1)%20 == 0)
			{
				buf[i/20] = (temp > 10) ? 0 : 1;
				temp = 0;
			}
		}
		fsk->Var.buffer = 0;
		for (int i=0; i < 7; i++)
		{
			//��ת��
			fsk->Var.buffer = fsk->Var.buffer + (buf[i + 1] << i);

			//��żУ��
			odd = odd + buf[i + 1];
		}
		fsk->Var.buf[fsk->Var.buf_pos++] = fsk->Var.buffer;
		fsk->Var.check_sum = fsk->Var.check_sum + fsk->Var.buffer;

		//������Ϣ�ּ���������
		if ((buf[0]==1)||(buf[9]==0))	fsk->Status.error = 2;
		
		//�����Ϣ�ֳ����Ƿ����
		if ((fsk->Var.buf_pos == 2) && (buf[1] > 28))	fsk->Status.error = 3;

		//�����żУ��
		if ((fsk->Var.buf_pos > 2) && (odd == buf[8]))
		{
			fsk->Status.error = 1;
		}
		//�жϽ����������Ϣ������
		if (fsk->Var.buf_pos == (fsk->Var.buf[1] + 3))
		{
			//���У����Ƿ����
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

/*��ȡ����*/
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

	//�������
	fsk->fsk_state = 2;

}

/*��Ϣ���*/
void message_process(fsk_demode_t *fsk)
{

	switch (fsk->Message.message_state)
	{
	case 0:
		//300��01��������������
		threshold_cur(fsk);

		break;
	case 1:
		//���180��1
		flag_detect(fsk);

		break;
	case 2:
		//�����Ϣ��
		word_detect(fsk);

		break;
	case 3:
		//�����Ϣ��
		decode_word(fsk);

		break;
	case 4:
		//������,��ȡ����
		get_message(fsk);

		break;
	default:
		break;
	}

}

int defsk_signal(fsk_demode_t *fsk)
{
	int temp;
	//���״̬��
	switch (fsk->fsk_state)
	{
	case 0:
		//������
		ring_detect(fsk);

		break;
	case 1:
		//��Ϣ���
		message_process(fsk);
		//temp = data_process(fsk);
		//printf("%d\n",temp);

		break;
	case 2:
		//�������,��ӡ������
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
		printf("\n�������%d\n", fsk->Status.error);

		return 0;

		break;
	default:
		break;
	}
	//ѭ������ǿ��ж�
	return (fsk->ScrBuffer.LeftSize == 0) ? 0 : 1;

}



/*�ӿں���*/
int interfunc(short *src, int srcLen, fsk_demode_t *fsk)
{

	fsk->Read_Cnt++;
	if (fsk->Read_Cnt == 119)
	{
		fsk->Read_Cnt = 119;
	}

	//3����ֵ->20�������1bit
	data_3inter(fsk, src, srcLen);

	//ѭ����⽫���������ڵ����ݽ�����
	while (defsk_signal(fsk));

	//���������
	if (fsk->Status.demode_ok == 1)
	{
		return 1;					//�������
	}
	return 0;						//���ڽ��
}
