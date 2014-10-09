#ifndef __PSTN_H__
#define __PSTN_H__

#define FXO_SET_HANGUP			0x00 //挂机
#define FXO_SET_PICKUP2CALL		0x01 //摘机拨号
#define FXO_SET_TALK 			0x02 //通话
#define FXO_SET_RCV_RING 		0x03 //接收铃音
#define FXO_EN_REMOTE_SPEAK		0x04 //允许对端电话用户发言
#define FXO_DIS_REMOTE_SPEAK 	0x05 //禁止对端电话用户发言
#define FXO_EN_REMOTE_LISTEN 	0x06 //允许对端电话用户听到会场发言
#define FXO_DIS_REMOTE_LISTEN 	0x07 //禁止对端电话用户听到会场发言
#define FXO_GET_STATE		 	0x80 //查询FXO当前状态

//FXO_GET_STATE
#define FXO_STATE_RING		0x00   //振铃
#define FXO_STATE_PICKUP 	0x01   //摘机
#define FXO_STATE_HANGUP 	0x02   //挂机

#endif
