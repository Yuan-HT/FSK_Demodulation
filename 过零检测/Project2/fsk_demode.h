#ifndef __DEMODE_FSK_H__
#define __DEMODE_FSK_H__

#include <stdio.h>
#include <math.h>
#include <string.h>


#define CIRCLEBUFFER_SIZE	415					//环形数组大小，128*3+31
#define RING_INT_NUM		300					//响铃积分个数
#define RING_INT_POW		0.15				//响铃积分门限，小于门限则为无信号
//#define TIME_OUT			1000				//超时计数器

static const float fir_coe[31] = { 0.000660329320734, 0.001241567547041, 0.002352931531400, 0.004318225759554, 0.007411480916187,
								   0.011812654699817, 0.017571976360572, 0.024588012453772, 0.032602962034372, 0.041216613891416,
								   0.049918077982393, 0.058132137189928, 0.065275151099105, 0.070814135981249, 0.074322123267820,
								   0.075523239929277, 0.074322123267820, 0.070814135981249, 0.065275151099105, 0.058132137189928,
								   0.049918077982393, 0.041216613891416, 0.032602962034372, 0.024588012453772, 0.017571976360572,
								   0.011812654699817, 0.007411480916187, 0.004318225759554, 0.002352931531400, 0.001241567547041,
								   0.000660329320734};//31阶低通滤波器系数

typedef struct
{
	short			CircleBuffer[CIRCLEBUFFER_SIZE];	//环形数组
	unsigned int	WriteIndex;							//环形数组写指针
	unsigned int	ReadIndex;							//环形数组读指针
	unsigned int	LeftSize;							//环形数组左值
}CircleBuffer_t;

typedef struct {
	
	unsigned char	fsk_state;				//解调状态机

	CircleBuffer_t	ScrBuffer;				//原始数据环形数组

	/*!
	 * \brief fsk解调结果
	 */
	struct Message
	{
		unsigned char	message_state;			//解调消息状态机
		unsigned int	msg_type;				//消息类型
		unsigned int	msg_len;				//消息字长度
		unsigned char	msg_time[8];			//消息字-时间  
		unsigned char	msg_tel[20];			//消息字-电话号码
		unsigned char	msg_check;				//校验和
	}Message;

	/*!
	 * \brief 响铃检测
	 */
	struct Ring
	{
		int		ring_flag;						//响铃标志，0，继续检测响铃，1已经检测过响铃了
		int		ring_cnt;						//响铃次数计数
		int		ring_int_cnt;					//响铃积分计数
		float	ring_int_value;					//响铃积分值
		int		ring_int_value_cnt;				//响铃积分值计数，计数连续小于门限的值的个数
	}Ring;

	/*!
	 * \brief 数据处理：三倍插值->>放大限幅->>微分->>整流->>脉宽调制->>低通滤波->>过零检测
	 */
	struct DSP
	{	
		int		diff_pre_value;					//前一个采样值（放大限幅后的）
		int		diff_cur_value;					//当前的采样值（放大限幅后的）

		int		pulse_value;					//脉冲值
		int		pulse_cnt;						//脉宽调制计数

		float	threshold;						//过零检测门限
		int		threshold_cnt;					//过零检测门限计数
		int		threshold_d;					//过零检测门限判别
		//int valid_data_cnt;						//有效数据技术，dsp每处理输出一个数据加一，每使用一个数据减一

		int		valid_data;						//数据有效标志
		CircleBuffer_t DSPBuffer;				//存放数据处理中间值的环形数组
	}DSP;

	/*!
	 * \brief 解调中间变量存储
	 */
	struct Var
	{
		unsigned char	buf[50];				//所有消息字缓存
		unsigned char	buf_pos;				//消息字缓存指针
		unsigned char	buffer;					//接收缓存串转并buffer
		int				flag_cnt;				//标志位计数
		int				word_cnt;				//解调消息字计数
		unsigned char	word_buf[200];			//解调消息字采样点存储存储1个字的10个bit所有采样点
		unsigned char	check_sum;				//校验和
	}Var;

	/*!
	 * \brief 解调状态指示
	 */
	struct Status
	{
		unsigned char	demode_ok;				//解调完成标志位，0未完成解调
												//1完成一帧消息解调

		unsigned char	error;					//错误类型指示
												//0->无错误,
												//1->消息字奇偶校验错误
												//2->消息字开始结束标志错误
												//3->长度错误，超过规定长度20+8
												//4->校验和错误


	}Status;



	//调试用
	int Read_Cnt;		//读数计数，读了几组数据了，判断一下当前数据的位置

} fsk_demode_t;


/*!
 * @brief 初始化函数
 *
 * @param [IN] fsk：解调fsk结构体
 * @param [OUT] None
 */
void init_demode_fsk(fsk_demode_t *fsk);

/*!
 * @brief 接口函数
 *
 * @param [IN] CircleBuffer:环形数组；src：原始数据；srcLen：原始数据长度
 * @param [OUT] fsk：解调fsk结构体
 */
int interfunc(short *src, int srcLen, fsk_demode_t *fsk);


#endif
