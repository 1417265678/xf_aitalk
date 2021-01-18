/*
* 语音听写(iFly Auto Transform)技术能够实时地将语音转换成对应的文字。
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
//#include <wiringPi.h>
#include "../../include/qisr.h"
#include "../../include/msp_cmn.h"
#include "../../include/msp_errors.h"
#include "speech_recognizer.h"
#define FRAME_LEN    640
#define    BUFFER_SIZE 4096
#define SAMPLE_RATE_16K     (16000)
#define SAMPLE_RATE_8K      (8000)
#define MAX_GRAMMARID_LEN   (32)
#define MAX_PARAMS_LEN      (1024)
#define LED_PIN 26
#define FAN_PIN 25
#define ON         1
#define OFF        2
#define LED_DEVICE 3
#define FAN_DEVICE 4
static int recEndFlag = 0; //是否识别出结果的标志
const char * ASR_RES_PATH        = "fo|res/asr/common.jet"; //离线语法识别资源路径
const char * GRM_BUILD_PATH      = "res/asr/GrmBuilld"; //构建离线语法识别网络生成数据保存路径
const char * GRM_FILE            = "control.bnf"; //构建离线识别语法网络所用的语法文件
const char * LEX_NAME            = "contact"; //更新离线识别语法的contact槽（语法文件为此示例中使用的call.bnf）
const char * led_str             = "灯";
const char * fan_str             = "风扇";
const char * open_str            = "打开";
const char * close_str           = "关闭";
typedef struct _UserData {
    int     build_fini; //标识语法构建是否完成
    int     update_fini; //标识更新词典是否完成
    int     errcode; //记录语法构建或更新词典回调错误码
    char    grammar_id[MAX_GRAMMARID_LEN]; //保存语法构建返回的语法ID
}UserData;
int build_grammar(UserData *udata); //构建离线识别语法网络
int run_asr(UserData *udata); //进行离线语法识别
void controlDevice(int deviceID, int flag)
{
    /*if(LED_DEVICE == deviceID) //control led light
    {
        if(ON == flag) //light on
        {
            digitalWrite(LED_PIN, LOW);
        }
        else //light off
        {
            digitalWrite(LED_PIN, HIGH);
        }
    }
    else //control fan device
    {
        if(ON == flag) //fan on
        {
            digitalWrite(FAN_PIN, HIGH);
        }
        else //fan off
        {
            digitalWrite(FAN_PIN, LOW);
        }
    }*/
}
int build_grm_cb(int ecode, const char *info, void *udata)
{
    UserData *grm_data = (UserData *)udata;
    if (NULL != grm_data) {
        grm_data->build_fini = 1;
        grm_data->errcode = ecode;
    }
    if (MSP_SUCCESS == ecode && NULL != info) {
        //printf("构建语法成功！ 语法ID:%s\n", info);
        if (NULL != grm_data)
            snprintf(grm_data->grammar_id, MAX_GRAMMARID_LEN - 1, info);
    }
    else
        printf("构建语法失败！%d\n", ecode);
    return 0;
}
int build_grammar(UserData *udata)
{
    FILE *grm_file                           = NULL;
    char *grm_content                        = NULL;
    unsigned int grm_cnt_len                 = 0;
    char grm_build_params[MAX_PARAMS_LEN]    = {NULL};
    int ret                                  = 0;
    grm_file = fopen(GRM_FILE, "rb");
    if(NULL == grm_file) {
        printf("打开\"%s\"文件失败！[%s]\n", GRM_FILE, strerror(errno));
        return -1;
    }
    fseek(grm_file, 0, SEEK_END);
    grm_cnt_len = ftell(grm_file);
    fseek(grm_file, 0, SEEK_SET);
    grm_content = (char *)malloc(grm_cnt_len + 1);
    if (NULL == grm_content)
    {
        printf("内存分配失败!\n");
        fclose(grm_file);
        grm_file = NULL;
        return -1;
    }
    fread((void*)grm_content, 1, grm_cnt_len, grm_file);
    grm_content[grm_cnt_len] = '\0';
    fclose(grm_file);
    grm_file = NULL;
    snprintf(grm_build_params, MAX_PARAMS_LEN - 1,
        "engine_type = local, \
        asr_res_path = %s, sample_rate = %d, \
        grm_build_path = %s, ",
        ASR_RES_PATH,
        SAMPLE_RATE_16K,
        GRM_BUILD_PATH
        );
    ret = QISRBuildGrammar("bnf", grm_content, grm_cnt_len, grm_build_params, build_grm_cb, udata);
    free(grm_content);
    grm_content = NULL;
    return ret;
}
static void show_result(char *string, char is_over)
{
    char *check = NULL;
    int device_id = 0;
    int operate_id = 0;
    //printf("\r%s", string);
    if(strstr(string,"开灯"))printf("开灯\n");
    if(strstr(string,"关灯"))printf("关灯\n");
    if(strstr(string,"开门"))printf("开门\n");
    if(strstr(string,"关门"))printf("关门\n");
    if(strstr(string,"打开风扇"))printf("打开风扇\n");
    if(strstr(string,"关闭风扇"))printf("关闭风扇\n");
    if(strstr(string,"打开遮阳板"))printf("打开遮阳板\n");
    if(strstr(string,"关闭遮阳板"))printf("关闭遮阳板\n");
    if(strstr(string,"退出程序"))exit(1);
    if(strstr(string,"打开播放器"))system("play /home/zhb/xf_aitalk/bin/2.mp3");
    if(is_over)
        putchar('\n');
    check = strstr(string, led_str);
    if(check != NULL)
    {
        device_id = 3;
    }
    check = strstr(string, fan_str);
    if(check != NULL)
    {
        device_id = 4;
    }
    check = strstr(string, open_str);
    if(check != NULL)
    {
       operate_id = 1;
    }
    check = strstr(string, close_str);
    if(check != NULL)
    {
        operate_id = 2;
    }
    if((device_id != 0) && (operate_id != 0))
    {
        controlDevice(device_id, operate_id);
    }
}
static char *g_result = NULL;
static unsigned int g_buffersize = BUFFER_SIZE;
void on_result(const char *result, char is_last)
{
    if (result) {
        size_t left = g_buffersize - 1 - strlen(g_result);
        size_t size = strlen(result);
        if (left < size) {
            g_result = (char*)realloc(g_result, g_buffersize + BUFFER_SIZE);
            if (g_result)
                g_buffersize += BUFFER_SIZE;
            else {
                printf("mem alloc failed\n");
                return;
            }
        }
        strncat(g_result, result, size);
        show_result(g_result, is_last);
    }
}
void on_speech_begin()
{
    if (g_result)
    {
        free(g_result);
    }
    g_result = (char*)malloc(BUFFER_SIZE);
    g_buffersize = BUFFER_SIZE;
    memset(g_result, 0, g_buffersize);
    //printf("Start Listening...\n");
}
void on_speech_end(int reason)
{
    recEndFlag = 1;
    if (reason == END_REASON_VAD_DETECT);
        //printf("\nSpeaking done \n");
    else
        printf("\nRecognizer error %d\n", reason);
}
/* demo recognize the audio from microphone */
static void demo_mic(const char* session_begin_params)
{
    int errcode;
    struct speech_rec iat;
    struct speech_rec_notifier recnotifier = {
        on_result,
        on_speech_begin,
        on_speech_end
    };
    recEndFlag = 0;
    errcode = sr_init(&iat, session_begin_params, SR_MIC, &recnotifier);
    if (errcode) {
        printf("speech recognizer init failed\n");
        return;
    }
    errcode = sr_start_listening(&iat);
    if (errcode) {
        printf("start listen failed %d\n", errcode);
    }
    /*mic always recording */
    while(1)
    {
        //printf("监听中。。。\n");
        if(recEndFlag)
        {
            break;
        }
        sleep(1);
    }
    //errcode = sr_stop_listening(&iat);
    if (errcode) {
        printf("stop listening failed %d\n", errcode);
    }
    sr_uninit(&iat);
}
int run_asr(UserData *udata)
{
    char asr_params[MAX_PARAMS_LEN]    = {NULL};
    const char *rec_rslt               = NULL;
    const char *session_id             = NULL;
    const char *asr_audiof             = NULL;
    FILE *f_pcm                        = NULL;
    char *pcm_data                     = NULL;
    long pcm_count                     = 0;
    long pcm_size                      = 0;
    int last_audio                     = 0;
    int aud_stat                       = MSP_AUDIO_SAMPLE_CONTINUE;
    int ep_status                      = MSP_EP_LOOKING_FOR_SPEECH;
    int rec_status                     = MSP_REC_STATUS_INCOMPLETE;
    int rss_status                     = MSP_REC_STATUS_INCOMPLETE;
    int errcode                        = -1;
    int aud_src                        = 0;
    //离线语法识别参数设置
    snprintf(asr_params, MAX_PARAMS_LEN - 1,
        "engine_type = local, \
        asr_res_path = %s, sample_rate = %d, \
        grm_build_path = %s, local_grammar = %s, \
        result_type = plain, result_encoding = UTF-8, ",
        ASR_RES_PATH,
        SAMPLE_RATE_16K,
        GRM_BUILD_PATH,
        udata->grammar_id
        );
    //printf("%s\n", plain);
    demo_mic(asr_params);
    return 0;
}
int main(int argc, char* argv[])
{
    const char *login_config    = "appid = 5962ed38"; //登录参数
    UserData asr_data;
    int ret                     = 0 ;
    /*wiringPiSetup();
    pinMode(LED_PIN, OUTPUT);
    pinMode(FAN_PIN, OUTPUT);*/
    ret = MSPLogin(NULL, NULL, login_config); //第一个参数为用户名，第二个参数为密码，传NULL即可，第三个参数是登录参数
    if (MSP_SUCCESS != ret) {
        printf("登录失败：%d\n", ret);
        goto exit;
    }
    memset(&asr_data, 0, sizeof(UserData));
    //printf("构建离线识别语法网络...\n");
    ret = build_grammar(&asr_data);  //第一次使用某语法进行识别，需要先构建语法网络，获取语法ID，之后使用此语法进行识别，无需再次构建
    if (MSP_SUCCESS != ret) {
        printf("构建语法调用失败！\n");
        goto exit;
    }
    while (1 != asr_data.build_fini)
        usleep(300 * 1000);
    if (MSP_SUCCESS != asr_data.errcode)
        goto exit;
    //printf("离线识别语法网络构建完成，开始识别...\n");
    printf("监听中。。。\n");
    while(1)
    {
        ret = run_asr(&asr_data);
        //printf("%s\n",asr_data );
        if (MSP_SUCCESS != ret) {
            printf("离线语法识别出错: %d \n", ret);
            goto exit;
        }
    }
exit:
    MSPLogout();
    printf("请按任意键退出...\n");
    getchar();
    return 0;
}