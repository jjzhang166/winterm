/*
 * message.h
 *
 *  Created on: 2012-9-10
 *      Author: zhangbo
 */

#ifndef MESSAGE_H_
#define MESSAGE_H_

#define REGISTKEY "注册码"
#define SCREENNUM "屏幕号"

#define APPLICATION_NAME "山东汇金信息科技有限公司 仿真终端  V4.1.1.0"
#define TITLE_POINT "提示"
#define FONT_NAME "新宋体"
#define RET_NAME "仿宋_GB2312"
#define CURRENTUSER "当前用户名"
#define INACTIVE "脱机"
#define CURRENTCOM "    当前的辅口:"
#define SURE_EXIT_MESSAGE "您确定要关闭当前窗口吗?"
#define SURE_CLOSE_CURRENTWIND "您确定要关闭当前窗口么?"
#define SURE_REVERT_DEFAULT "您确定要恢复为默认值吗?"
#define POINT_ISPRINT_SCREEN "要屏幕打印吗?"
#define POINT_SELECT_HOST "请选择一个服务器IP"
#define POINT_PRINTINGDATA_NOSCREEN "打印机正在打印数据，暂时不能打印屏幕"
#define POINT_PRINTING_DATA "打印机正在打印数据......\n\n*确定*继续打印,打印完成后自动退出程序\n*取消*终止打印"

#define RESUME_SET_COLOUR "恢复默认设置成功,下次运行生效"
#define SAVE_SET_COLOUR "保存颜色设置成功,下次运行生效"
#define SAVE_SET_PORT "保存端口设置成功       "
#define SAVE_SET_HOST "设置默认服务器地址成功！下次运行生效"
#define SAVE_SET_CUSTOM "用户自定义键设置保存成功！"

#define ERROR_NOTCONNECT "错误！不能连接到\n"
#define ERROR_002 "错误代码：0002,抱歉!本软件不能在本机上使用,请联系软件提供商."
#define ERROR_003 "错误代码：0003,读取注册文件信息失败"
#define ERROR_SEND_PRINT "发送到打印机错误！请检查打印机是否联机"
#define ERROR_PASTE "错误!粘贴失败,请先登录系统！"

#define MSG_ERROR_REGKEY "注册码不正确，请重新输入"
#define MSG_COPY_SUCCESS "机器码已经成功复制"

#define WARNING_HOST_EMPTY "服务器地址不能为空，请重新输入"
#define WARNING_PORT_WRONG "服务器端口号输入不正确，提示：端口号范围0~65535，请重新输入"
#define WARNING_HOST_EXIST "该服务器地址已存在，不可重复添加"
#define WARNING_NOREMOVABLE_HOST "没有可删除的服务器IP"
#define SUCCESS_ADD_HOSTPORT "添加服务器地址及端口成功！"
#define SUCCESS_DEL_HOST "删除一个服务器IP成功"

#define TABITEM_BASIC "基本设置"
#define TABITEM_DEFINEKEY  "用户自定义键"
#define TABITEM_FUNCTION "功能键设置"
#define TABITEM_COLOUR		"高级设置"
#define SELECTWAVFILE  "请选择WAV格式的声音文件"
#define INPUTTERMNAME "请输入新快捷方式名称"
#define NEWTERMSUCCESS "新快捷方式创建成功!"
#define ERROR_PRINT "打印机正在打印或者打印机未联机，请检查打印机。\n如多次弹出该对话框，请重启打印机!\n是否继续打印？"
#define ERROR_INITPRINT "初始化打印机失败，请重新启动本软件"
#define ALREADYOPEN   "已经打开一个仿真程序，一次只能有一个仿真程序运行!"
#define OPENCOMFAIL  "打开串口失败，请查看串口是否设置正确，或者是否有其他程序占用该串口"

#define FILENAME "文件名称"
#define PRINTERNAME "打印机名称"
#define PRINTCOM "打印辅口"
#define FILEPRINT "文件打印"
#define FILEPDIR "文件名只可以是LPT+数字的形式，如LPT1"
#define SERVICEPRINT "服务打印"
#define SERVICEPDIR "当允许开启多屏时可以采用该打印方式"
#define SERIALPRINT "串口打印"
#define SERIALPDIR "使用串口打印机时，采用此种打印方式，选择连接的串口"
#define DIRECTPRINT "直接打印"
#define DIRECTPDIR "直接打印到并口，不需要安装打印驱动"
#define DRIVEPRINT "驱动打印"
#define DRIVEPDIR "采用驱动打印的形式，需要安装相应的打印机驱动"
#define PRINTNAME "采用驱动打印方式，请在“设置--属性设置--打印设置”中设置打印机名称。"
#define FINDERROR "出现错误,失败"
#define REPEAT "允许同时运行多个本程序可能会造成打印机和部分串口设备不能使用"
#define INPUTPASSWORD "您的密码有误，请重新输入。"

#define COMNAME1 "刷卡器"
#define COMNAME2 "密码键盘"
#define COMNAME3 "柜员卡/指纹仪"
#define COMNAME4 "辅口设备4"
#define COMNAME5 "辅口设备5"
#define COMNAME6 "身份证识别仪"

#define NONE "无校验"
#define ODD "奇校验"
#define EVEN "偶校验"
#endif /* MESSAGE_H_ */
