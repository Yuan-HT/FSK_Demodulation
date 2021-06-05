#ifndef __DEMODE_FSK_H__
#define __DEMODE_FSK_H__

#include <stdio.h>
#include <math.h>
#include <string.h>


#define CIRCLEBUFFER_SIZE	415					//���������С��128*3+31
#define RING_INT_NUM		300					//������ָ���
#define RING_INT_POW		0.15				//����������ޣ�С��������Ϊ���ź�
//#define TIME_OUT			1000				//��ʱ������

static const float fir_coe[31] = { 0.000660329320734, 0.001241567547041, 0.002352931531400, 0.004318225759554, 0.007411480916187,
								   0.011812654699817, 0.017571976360572, 0.024588012453772, 0.032602962034372, 0.041216613891416,
								   0.049918077982393, 0.058132137189928, 0.065275151099105, 0.070814135981249, 0.074322123267820,
								   0.075523239929277, 0.074322123267820, 0.070814135981249, 0.065275151099105, 0.058132137189928,
								   0.049918077982393, 0.041216613891416, 0.032602962034372, 0.024588012453772, 0.017571976360572,
								   0.011812654699817, 0.007411480916187, 0.004318225759554, 0.002352931531400, 0.001241567547041,
								   0.000660329320734};//31�׵�ͨ�˲���ϵ��

typedef struct
{
	short			CircleBuffer[CIRCLEBUFFER_SIZE];	//��������
	unsigned int	WriteIndex;							//��������дָ��
	unsigned int	ReadIndex;							//���������ָ��
	unsigned int	LeftSize;							//����������ֵ
}CircleBuffer_t;

typedef struct {
	
	unsigned char	fsk_state;				//���״̬��

	CircleBuffer_t	ScrBuffer;				//ԭʼ���ݻ�������

	/*!
	 * \brief fsk������
	 */
	struct Message
	{
		unsigned char	message_state;			//�����Ϣ״̬��
		unsigned int	msg_type;				//��Ϣ����
		unsigned int	msg_len;				//��Ϣ�ֳ���
		unsigned char	msg_time[8];			//��Ϣ��-ʱ��  
		unsigned char	msg_tel[20];			//��Ϣ��-�绰����
		unsigned char	msg_check;				//У���
	}Message;

	/*!
	 * \brief ������
	 */
	struct Ring
	{
		int		ring_flag;						//�����־��0������������壬1�Ѿ�����������
		int		ring_cnt;						//�����������
		int		ring_int_cnt;					//������ּ���
		float	ring_int_value;					//�������ֵ
		int		ring_int_value_cnt;				//�������ֵ��������������С�����޵�ֵ�ĸ���
	}Ring;

	/*!
	 * \brief ���ݴ���������ֵ->>�Ŵ��޷�->>΢��->>����->>�������->>��ͨ�˲�->>������
	 */
	struct DSP
	{	
		int		diff_pre_value;					//ǰһ������ֵ���Ŵ��޷���ģ�
		int		diff_cur_value;					//��ǰ�Ĳ���ֵ���Ŵ��޷���ģ�

		int		pulse_value;					//����ֵ
		int		pulse_cnt;						//������Ƽ���

		float	threshold;						//����������
		int		threshold_cnt;					//���������޼���
		int		threshold_d;					//�����������б�
		//int valid_data_cnt;						//��Ч���ݼ�����dspÿ�������һ�����ݼ�һ��ÿʹ��һ�����ݼ�һ

		int		valid_data;						//������Ч��־
		CircleBuffer_t DSPBuffer;				//������ݴ����м�ֵ�Ļ�������
	}DSP;

	/*!
	 * \brief ����м�����洢
	 */
	struct Var
	{
		unsigned char	buf[50];				//������Ϣ�ֻ���
		unsigned char	buf_pos;				//��Ϣ�ֻ���ָ��
		unsigned char	buffer;					//���ջ��洮ת��buffer
		int				flag_cnt;				//��־λ����
		int				word_cnt;				//�����Ϣ�ּ���
		unsigned char	word_buf[200];			//�����Ϣ�ֲ�����洢�洢1���ֵ�10��bit���в�����
		unsigned char	check_sum;				//У���
	}Var;

	/*!
	 * \brief ���״ָ̬ʾ
	 */
	struct Status
	{
		unsigned char	demode_ok;				//�����ɱ�־λ��0δ��ɽ��
												//1���һ֡��Ϣ���

		unsigned char	error;					//��������ָʾ
												//0->�޴���,
												//1->��Ϣ����żУ�����
												//2->��Ϣ�ֿ�ʼ������־����
												//3->���ȴ��󣬳����涨����20+8
												//4->У��ʹ���


	}Status;



	//������
	int Read_Cnt;		//�������������˼��������ˣ��ж�һ�µ�ǰ���ݵ�λ��

} fsk_demode_t;


/*!
 * @brief ��ʼ������
 *
 * @param [IN] fsk�����fsk�ṹ��
 * @param [OUT] None
 */
void init_demode_fsk(fsk_demode_t *fsk);

/*!
 * @brief �ӿں���
 *
 * @param [IN] CircleBuffer:�������飻src��ԭʼ���ݣ�srcLen��ԭʼ���ݳ���
 * @param [OUT] fsk�����fsk�ṹ��
 */
int interfunc(short *src, int srcLen, fsk_demode_t *fsk);


#endif
