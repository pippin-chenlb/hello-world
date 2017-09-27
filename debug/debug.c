#include <sys/types.h> 
#include <sys/time.h> 
#include <stdio.h> 
#include <fcntl.h> 
#include <sys/ioctl.h> 
#include <unistd.h> 
#include <pthread.h>
#include "tos.h"
#include "tdr_debug_msg.h"

static int curLevel = 0;
static char curModule[CMD_MAX_LENGTH] = {0};

//与主线程通讯结构体定义
typedef struct deb_self_msg_s{
    int destId;         //需要发送的模块端口号
    debug_msg_t info;   //需要发送的消息
}deb_self_msg_t;

typedef struct dest_dist_s{
    char * modeName;  //目的模块名字
    int  modeId;    //目的模块号
    int  port;           //目的模块端口号
}dest_dist_t;
#define  DEBUG_MODULE_NUM   (9)     //暂定支持的模块数目4
dest_dist_t g_port_dist[DEBUG_MODULE_NUM] = 
{
    {"link",TOS_MAIN,TOS_PORT_MAIN},
    {"ds",TOS_DS,TOS_PORT_DS},
    {"tag",TOS_TAG,TOS_PORT_TAG},
    {"gps",TOS_GPS,TOS_PORT_GPS},
    {"mac",TOS_MAC,TOS_PORT_MAC},
    {"gun",TOS_GUN,TOS_PORT_GUN},
    {"house",TOS_HOUSE,TOS_PORT_HOUSE},
    {"ecar",TOS_ECAR,TOS_PORT_ECAR},
    {"lights",TOS_LIGHTS,TOS_PORT_LIGHTS}
};

#define  DEBUG_GLOBAL_CMD_NUM   (DEBUG_MODULE_NUM + 1) //支持的全局命令数
char g_global_cmd[DEBUG_GLOBAL_CMD_NUM][CMD_MAX_LENGTH] = {
    {"ver"},
    {"link"},
    {"ds"},
    {"tag"},
    {"gps"},
    {"mac"},
    {"gun"},
    {"house"},
    {"ecar"},
    {"lights"}
};

//子目录支持的命令合计,用于第一道命令过滤
#define DEBUG_SUB_CMD_MAX_NUM       (20)    //子目录最多支持自命令数目
char g_global_sub_cmd[DEBUG_MODULE_NUM][DEBUG_SUB_CMD_MAX_NUM][CMD_MAX_LENGTH] = {
    {{"config"},{"cache"},{"plat"},{"tcp"},{"mm"},{"log"},{"info"}},  //TOS_MAIN
    {{"mm"},{"log"},{"info"}},  //TOS_DS
    {{"mm"},{"log"},{"info"},{"get"}},  //TOS_TAG
    {{"mm"},{"log"},{"info"}},  //TOS_GPS
    {{"mm"},{"log"},{"info"}},  //TOS_MAC
    {{"config"},{"broadcast"},{"terminal"},{"trace"},{"mm"},{"log"},{"info"}},  //TOS_GUN
    {{"mm"},{"log"},{"info"}},  //TOS_HOUSE
    {{"cache"},{"config"},{"mm"},{"log"},{"info"}},  //TOS_ECAR
    {{"mm"},{"log"},{"info"}}  //TOS_LIGHTS
};
    
typedef struct debug_cmd_s{
    int iCmdNum;   //命令个数
    char pCmdBuff[CMD_MAX_NUM][CMD_MAX_LENGTH];    //存放每个命令
}debug_cmd_t;

/** @fn: get_tty_name
*   @brief: 获取伪终端路径
*   @param [in] 
*   @return 
*/
int get_tty_name(char * buffer)
{
    char * pTtyName = NULL;

    if (NULL == buffer)
    {
        return -1;
    }
    
	pTtyName = ttyname(STDOUT_FILENO);
	if(NULL == pTtyName || strlen(pTtyName) > TTY_NAME_LENGTH)
	{
        printf("pTtyName[%p] length = %d\n",pTtyName,strlen(pTtyName));
        return -1;
	}
    memcpy(buffer, pTtyName, strlen(pTtyName));
    return 0;
}

/** @fn: cmd_help
*   @brief: 打印help提示信息
*   @param [in] 
*   @return 
*/
int cmd_help(char * pBuffer)
{
    int i;
    
    if (NULL == pBuffer)
    {
        printf("shell tools:\n");
        printf("    help               :shell帮助\n");
        printf("    ver                :查看产品的版本信息\n");
        printf("    link               :打开链路模块调试\n");
        printf("    ds                 :打开数据源模块调试\n");
        printf("    tag                :打开标签模块调试\n");
        printf("    gps                :打开gps模块调试\n");
        printf("    mac                :打开mac采集模块调试\n");
        printf("    gun                :打开稽查枪模块调试\n");
        printf("    house              :打开出租屋模块调试\n");
        printf("    ecar               :打开电动车模块调试\n");
        printf("    lights             :打开红绿灯模块调试\n");
        printf("    exit               :退出shell\n");
        return 0;
    }

    printf("%s %d pBuffer = %s\n",__FUNCTION__,__LINE__,pBuffer);
    for (i = 0;i< DEBUG_MODULE_NUM;i++)
    {
        if (0 == strcmp(g_port_dist[i].modeName,pBuffer))
        {
            break;
        }
    }

    if (i >= DEBUG_MODULE_NUM)
    {
        printf("not support\n");
        return -1;
    }
    switch(g_port_dist[i].port){
        case TOS_PORT_MAIN:
        {
            printf("    help               :帮助\n");
            printf("    config info        :获取配置信息\n");
            printf("    config X  X        :设置配置信息(X:platname/ip/port/connTime/linkConnTime/reconnTime/gprs/cdma)\n");
            printf("                       :eg config ip 192.168.1.1 配置调度中心默认ip值\n");
            printf("    cache info         :打印本地发送缓存信息\n");
            printf("    plat info          :平台连接状态信息\n");
            printf("    tcp info           :查看gprs或cdma状态信息\n");
            printf("    mm show X X        :模块内存(xx:0 全部 1 按大小 2 模块名 3 文件名 4 函数名)\n");
            printf("    mm check           :模块内存检测\n");
            printf("    log X X            :设置打印日志级别xx:调试级别和子模块号(只支持16进制输入)\n");
            printf("                       :eg: log 0x3 0x0 开启fatal和error输出\n");
            printf("    info               :获取调试信息\n");
            printf("    exit               :退出\n");
        }
        break;
        case TOS_PORT_DS:
        {
            printf("    help               :帮助\n");
            printf("    mm show X X        :模块内存(xx:0 全部 1 按大小 2 模块名 3 文件名 4 函数名)\n");
            printf("    mm check           :模块内存检测\n");
            printf("    log X X            :设置打印日志级别xx:调试级别和子模块号(默认0)\n");
            printf("                       :eg: log 0x3 0x0 开启fatal和error输出\n");
            printf("    info               :获取ds调试信息\n");
            printf("    exit               :退出\n");
        }
        break;
        case TOS_PORT_TAG:
        {
            printf("    help               :帮助\n");
            printf("    mm show X X        :模块内存(xx:0 全部 1 按大小 2 模块名 3 文件名 4 函数名)\n");
            printf("    mm check           :模块内存检测\n");    
            printf("    get  cache         :获取标签缓存信息\n");
            printf("    log X X            :设置打印日志级别xx:调试级别和子模块号(默认0)\n");
            printf("                       :eg: log 0x3 0x0 开启fatal和error输出\n");
            printf("    info               :获取标签全局信息\n");
            printf("    exit               :退出\n");
        }
        break;
        case TOS_PORT_GPS:
        {
            printf("    help               :帮助\n");
            printf("    mm show X X        :模块内存(xx:0 全部 1 按大小 2 模块名 3 文件名 4 函数名)\n");
            printf("    mm check           :模块内存检测\n");
            printf("    log X X            :设置打印日志级别xx:调试级别和子模块号(默认0)\n");
            printf("                       :eg: log 0x3 0x0 开启fatal和error输出\n");
            printf("    info               :获取gps调试信息\n");
            printf("    exit               :退出\n");
        }
        break;
        case TOS_PORT_MAC:
        {
            printf("    help               :帮助\n");
            printf("    mm show X X        :模块内存(xx:0 全部 1 按大小 2 模块名 3 文件名 4 函数名)\n");
            printf("    mm check           :模块内存检测\n");
            printf("    log X X            :设置打印日志级别xx:调试级别和子模块号(默认0)\n");
            printf("                       :eg: log 0x3 0x0 开启fatal和error输出\n");
            printf("    info               :获取mac调试信息\n");
            printf("    exit               :退出\n");
        }
        break;
        case TOS_PORT_GUN:
        {
            printf("    help               :帮助\n");
            printf("    config info        :获取配置信息\n");
            printf("    config X  X        :设置配置信息(X:broadcast_time/broadcast_interval/keep_alive/track_intreval)\n");
            printf("                       :eg config broadcast_time 5 配置广播次数5\n");
            printf("    mm show X X        :模块内存(xx:0 全部 1 按大小 2 模块名 3 文件名 4 函数名)\n");
            printf("    mm check           :模块内存检测\n");
            printf("    log X X            :设置打印日志级别xx:调试级别和子模块号(默认0)\n");
            printf("                       :eg: log 0x3 0x0 开启fatal和error输出\n");
            printf("    broadcast info     :获取广播子模块信息\n");
            printf("    terminal info      :获取终端列表信息\n");
            printf("    trace info         :获取缓存轨迹信息\n");
            printf("    info               :获取稽查枪调试信息\n");
            printf("    exit               :退出\n");
        }
        break;
        case TOS_PORT_HOUSE:
        {
            printf("    help               :帮助\n");
            printf("    mm show X X        :模块内存(xx:0 全部 1 按大小 2 模块名 3 文件名 4 函数名)\n");
            printf("    mm check           :模块内存检测\n");
            printf("    log X X            :设置打印日志级别xx:调试级别和子模块号(默认0)\n");
            printf("                       :eg: log 0x3 0x0 开启fatal和error输出\n");
            printf("    info               :获取出租屋调试信息\n");
            printf("    exit               :退出\n");
        }
        break;
        case TOS_PORT_ECAR:
        {
            printf("    help               :帮助\n");
            printf("    config info        :获取配置信息\n");
            printf("    config X  X        :设置配置信息(X:offline_time/upload_time)\n");
            printf("                       :eg config offline_time 5 配置offline定时器响应时间为5s\n");
            printf("    mm show X X        :模块内存(xx:0 全部 1 按大小 2 模块名 3 文件名 4 函数名)\n");
            printf("    mm check           :模块内存检测\n");
            printf("    log X X            :设置打印日志级别xx:调试级别和子模块号(默认0)\n");
            printf("                       :eg: log 0x3 0x0 开启fatal和error输出\n");
            printf("    cache info         :获取缓存和全局信息\n");
            printf("    info               :获取电动车调试信息\n");
            printf("    exit               :退出\n");
        }
        break;
        case TOS_PORT_LIGHTS:
        {
            printf("    help               :帮助\n");
            printf("    mm show X X        :模块内存(xx:0 全部 1 按大小 2 模块名 3 文件名 4 函数名)\n");
            printf("    mm check           :模块内存检测\n");
            printf("    log X X            :设置打印日志级别xx:调试级别和子模块号(默认0)\n");
            printf("                       :eg: log 0x3 0x0 开启fatal和error输出\n");
            printf("    info               :获取红绿灯调试信息\n");
            printf("    exit               :退出\n");
        }
        break;
        default:
            printf("something error\n");
            break;
    }
    return 0;
}

/** @fn: dec_cmd_space
*   @brief: 去除命令中无效的空格
*   @param [in] 
*   @return 
*/
int dec_cmd_space(char * pBuffer, int * pLen)
{
    char * pTmpBuff = NULL;
    int i = 0;
    int len = 0;
    
    if (NULL == pBuffer || NULL == pLen)
    {
        printf("error");
        return -1;
    }
    if (*pLen <= 0)
    {
        printf("error");
        return -1;
    }
    pTmpBuff = malloc(*pLen);
    if (NULL == pTmpBuff)
    {
        return -1;    
    }

    i = 0;
    while(' ' == pBuffer[i])
    {
        i++;
    }
    for (;i < *pLen;i++)
    {
        if (' ' == pBuffer[i] && (i > 1) && ' ' == pBuffer[i - 1])
        {
            continue;
        }
        pTmpBuff[len] = pBuffer[i];
        len++;
    }

    //去掉最后一个可能的空格
    if (pTmpBuff[len - 1] == ' ')
    {
        len--;
    }
    memset(pBuffer,0x00,*pLen);
    memcpy(pBuffer,pTmpBuff,len);
    *pLen = len;
    free(pTmpBuff);
    return 0;
}

/** @fn: calc_cmd_num
*   @brief: 计算有效命令个数
*   @param [in] 
*   @return 
*/
int calc_cmd_num(char * buffer)
{
    char * tmp = NULL;
    char * tmpNext = NULL;
    int iNum = 1;
    
    tmp = strstr(buffer," ");
    while(tmp)
    {
        iNum++;
        tmpNext = strstr(tmp + 1," ");
        tmp = tmpNext;
    }

    return iNum;
}

/** @fn: sub_cmd_check
*   @brief: 子目录命令检测，现在只做一个参数检测，后续参数由模块内部检测
*   @param [in] 
*   @return 
*/
int sub_cmd_check(int moduleId, char * cmd)
{
    int i;
    
    if (moduleId >= DEBUG_MODULE_NUM || NULL == cmd)
    {
        return -1;
    }

    for (i = 0; i < DEBUG_SUB_CMD_MAX_NUM; i++)
    {
        if (0 == strcmp(g_global_sub_cmd[moduleId][i], cmd))
        {
            break;
        }
    }
    if (i >= DEBUG_SUB_CMD_MAX_NUM)
    {
        return -1;
    }

    return 0;
}

/** @fn: parse_cmd
*   @brief: 命令解析
*   @param [in] 
*   @return 
*/
int parse_cmd(char * pBuffer, int len, debug_cmd_t * pResult)
{
    char * tmp = NULL;
    char * tmpNext = NULL;
    int iCount = 0;  //命令个数
    int i;
    
    if (NULL == pBuffer || NULL == pResult || len < 0)
    {
        printf("error");
        return -1;
    }
    if (0 != dec_cmd_space(pBuffer, &len))
    {
        return -1;
    }
    iCount = calc_cmd_num(pBuffer);
    if (iCount > CMD_MAX_NUM)
    {
        printf("参数太多，目前最多支持[%d]个参数\n",CMD_MAX_NUM);
        return -1;        
    }

    pResult->iCmdNum = iCount;
    if (iCount == 1)
    {
        memcpy(pResult->pCmdBuff[0], pBuffer,len);
        return 0;
    }
    
    tmp = tmpNext = pBuffer;
    for(i = 0;i < iCount - 1;i++)
    {
        tmpNext = strstr(tmp + 1," ");
        if (tmpNext - tmp > CMD_MAX_LENGTH)
        {
            printf("error\n");
            return -1;
        }
        if (i == iCount - 2)
        {
            memcpy(pResult->pCmdBuff[i], tmp,tmpNext - tmp);
            if (pBuffer + len - tmp > CMD_MAX_LENGTH)
            {
                printf("命令长度超过 %d \n",CMD_MAX_LENGTH);
                return -1;
            }
            memcpy(pResult->pCmdBuff[i + 1], tmpNext + 1, pBuffer + len - tmpNext );
        }
        else
        {
            memcpy(pResult->pCmdBuff[i], tmp,tmpNext - tmp);
        }
        tmp = tmpNext + 1;
    }
    return 0;
}

/** @fn: debug_level_up
*   @brief: 调试层次上升
*   @param [in] 
*   @return 
*/
void debug_level_up(char * pModName)
{
    strcpy(curModule,pModName);
    curLevel++;
}

/** @fn: debug_level_down
*   @brief: 调试层次下降
*   @param [in] 
*   @return 
*/
void debug_level_down()
{
    memset(curModule,0x00,sizeof(curModule));
    if (curLevel > 0)
    {
        curLevel--;
    }
    else
    {
        curLevel = 0;
    }
}

/** @fn: send_msg
*   @brief: 发送消息到指定进程
*   @param [in] 
*   @return 
*/
int send_msg(int srcId,char * pData,int iLen)
{
	int32_t iRet = 0;
	tos_message_t * pTosMessage = NULL;

	pTosMessage = (tos_message_t *)tos_message_alloc(sizeof(tos_message_t)+iLen);
	if (NULL == pTosMessage)
    {
		printf("tos_message_alloc failed\n");
		return -1;
	}
	
	pTosMessage->src = TOS_DEBUG;
	pTosMessage->dst = srcId;
	pTosMessage->length = iLen;

	memcpy(pTosMessage->message,pData,iLen);
	iRet = tos_message_send(pTosMessage);
	if (0 != iRet)
    {
		printf("send to [%d] failed\n",srcId);
	}
	return iRet;
}

/** @fn: deal_cmd
*   @brief: 命令处理接口
*   @param [in] 
*   @return 
*/
int deal_cmd(debug_cmd_t * pStrCmd)
{
    int i = 0;
    char * helpBuff = NULL;
    debug_msg_t sendMsg = {0};
    deb_self_msg_t strSelfInfo = {0};
    
    if (NULL == pStrCmd)
    {
        return -1;
    }

    //解析 exit和help
    if (1 == pStrCmd->iCmdNum)
    {
        if(0 == strcmp(pStrCmd->pCmdBuff[0],"exit")){
            if (0 == curLevel){
                exit(0);
            }else{
                debug_level_down();
                return 0;
            }
        }else if (0 == strcmp(pStrCmd->pCmdBuff[0],"help")){
            if (0 == curLevel){
                helpBuff = NULL;
            }else{
                helpBuff = curModule;
            }
            cmd_help(helpBuff);
            return 0;
        }

        //外层需要响应命令解析
        if (0 == curLevel)
        {
            for (i = 0; i < DEBUG_GLOBAL_CMD_NUM; i++)
            {
                if (0 == strcmp(g_global_cmd[i],pStrCmd->pCmdBuff[0]))
                {
                    break;
                } 

            }
            
            if (i < DEBUG_GLOBAL_CMD_NUM)
            {
                //处理ver命令
                if(0 == strcmp(pStrCmd->pCmdBuff[0],"ver"))
                {
                    printf("proc ver cmd %s\n",pStrCmd->pCmdBuff[0]);
                }
                else
                {
                    debug_level_up(pStrCmd->pCmdBuff[0]);
                }
                return 0;
            }
            else
            {
                printf("[%s][%d]命令不支持，请(help)确认后重新输入\n",__FUNCTION__,__LINE__);
                return -1;
            }
        }
    }

    for (i = 0; i < DEBUG_MODULE_NUM; i++)
    {
        if (0 == strcmp(g_port_dist[i].modeName,curModule))
        {
            break;
        }        
    }
    
    if (i >= DEBUG_MODULE_NUM)
    {
        printf("[%s][%d]命令不支持，请(help)确认后重新输入\n",__FUNCTION__,__LINE__);
        return -1;
    }

    //子目录命令发送

    if (0 != sub_cmd_check(i, pStrCmd->pCmdBuff[0]))
    {
        printf("[%s][%d][%s]命令不支持，请(help)确认后重新输入\n",__FUNCTION__,__LINE__,pStrCmd->pCmdBuff[0]);    
        return -1;
    }

    sendMsg.cmdNum = pStrCmd->iCmdNum;
    memcpy(sendMsg.cmd, pStrCmd->pCmdBuff, pStrCmd->iCmdNum*CMD_MAX_LENGTH);
    get_tty_name(sendMsg.acTtyName);
    //线程发送不安全性，导致直接启用线程发送，第二次发送会失败
    //现采用折中办法，先发送给主线程，再又主线程发送给相应模块

    strSelfInfo.destId = g_port_dist[i].modeId;
    strSelfInfo.info = sendMsg;
    send_msg(TOS_DEBUG, &strSelfInfo, sizeof(strSelfInfo));
    return 0;
}

/** @fn: debug_task
*   @brief: debug启动线程
*   @param [in] 
*   @return 
*/
void  debug_task()
{ 
    char buffer[NORMAL_PRINT_LENGHT]; 
    int result, nread; 
    fd_set inputs, watchfds; 
    struct timeval timeout;
    
    FD_ZERO(&inputs);//用select函数之前先把集合清零
    FD_SET(0,&inputs);//把要检测的句柄——标准输入（0），加入到集合里。
    while(1) 
    { 
        watchfds = inputs; 
        timeout.tv_sec = 50;//s; 
        timeout.tv_usec = 0;//500000; 
        printf("%s->",curModule);
        fflush(stdout);
        result = select(FD_SETSIZE, &watchfds, (fd_set *)0, (fd_set *)0, &timeout); 
        switch(result) 
        { 
        case 0: 
        {
            printf("请输入命令，否则请退出(exit or ctrl^c)\n");
            if (0 == curLevel)
            {
                cmd_help(NULL);
            }
            else
            {
                cmd_help(curModule);
            }
        }
            break;
        case -1: 
            perror("select"); 
            exit(1); 
        default: 
            if(FD_ISSET(0,&watchfds)) 
            { 
                ioctl(0,FIONREAD,&nread);//取得从键盘输入字符的个数，包括回车。 
                if(nread == 0) 
                { 
                    printf("keyboard done\n");
                    fflush(stdout);
                    exit(0); 
                } 
                nread = read(0,buffer,nread); 
                buffer[nread - 1] = '\0';  //去掉回车
                if (0 == nread - 1)
                {
                    printf("\n");
                    continue;
                }
                debug_cmd_t cmdStr = {0};
                if (0 != parse_cmd(buffer, nread - 1,&cmdStr))
                {
                    continue;
                }
                //处理
                deal_cmd(&cmdStr);
            } 
            break; 
        } 
   } 
   return ;
}

/** @fn: debug_init
*   @brief: debug模块启动初始化接口
*   @param [in] 
*   @return 
*/
int debug_init() 
{
    pthread_t debugPid = -1;
    int ret = -1;
    
    printf("%s %d main_pthread %d\n",__FUNCTION__,__LINE__,pthread_self());
    ret = tos_thread_create(&debugPid,debug_task,NULL);
    printf("%s %d ret = %d pthread %d\n",__FUNCTION__,__LINE__,ret,debugPid);
    return 0;
}

/** @fn: debug_msg_proc
*   @brief: debug模块消息接收处理接口
*   @param [in] 
*   @return 
*/
int debug_msg_proc(tos_message_t *message) 
{
    deb_self_msg_t * pSelfInfo = NULL;
    //解析发送子线程需要发送的数据
    if (NULL == message)
    {
        printf("%s %d error\n",__FUNCTION__,__LINE__);
        return -1;    
    }
    if (TOS_DEBUG != message->src)
    {
        printf("%s %d error\n",__FUNCTION__,__LINE__);
        return -1; 
    }    
    pSelfInfo = (deb_self_msg_t *)message->message; 
    send_msg(pSelfInfo->destId, &(pSelfInfo->info), sizeof(pSelfInfo->info));
    return 0;
}


