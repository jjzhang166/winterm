#ifndef	_GETPCMSGAPI_H
#define	_GETPCMSGAPI_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 函数功能：获取某个网卡的MAC地址
 参数参数：
 macAddr :返回的MAC地址
 nowLanNumber:指明第几块网卡

 返回值：0返回正确，非零:没找到网卡
 程 序 员：	陈彬
 编码日期：  2010-2-24
 修改日期：
 */
int getLanMacAddress(char * macAddr, int nowLanNumber);

/*
 函数功能：获取CPU的序列号
 参数参数：
 macAddr :返回CPU的序列号


 返回值：0返回正确，非零:
 程 序 员：	陈彬
 编码日期：  2010-2-24
 修改日期：
 */
int getCpuSeleroNumber(char * cpuStr);

/*
 函数功能：获取硬盘的序列号
 参数参数：
 hddStr :返回硬盘的序列号

 返回值：0返回正确，非零:
 程 序 员：	陈彬
 编码日期：  2010-2-24
 修改日期：
 */
int getHddSeleroNumber(char * hddStr);

#ifdef __cplusplus
}
#endif

#endif	//__BJCAKEY_H
