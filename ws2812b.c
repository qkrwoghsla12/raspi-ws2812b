/*
*********************************************************************************************************
*                                         WS2812B FUCNTION.C
*********************************************************************************************************
*/
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/kdev_t.h>
#include <stdint.h>
#include "ws2812b.h"

static int ws2812b_fd;
static ws2812b_led s_led[MAX_NUM_OF_LEDS_PER_WS2812B_MODULE] ={0,};
/*
*********************************************************************************************************
*                                    ws2812bS2812B FUCNTION
*********************************************************************************************************
*/
/*
* ws2812b 초기화
* int ws2812b_setup(void)
* 입력 값 : 없음
* 반환 값 : 성공 0 / 실패 -1
* 설명   : 디바이스 파일을 생성하고 디바이스 파일 오픈
*/
int ws2812b_setup(void)
{
    // /dev/ 밑의 디바이스 파일 생성    
    mknod(DEV_NAME,S_IRWXU|S_IRWXG|S_IFCHR,(DEV_LED_MAJOR_NUMBER<<8)|DEV_LED_MINOR_NUMBER);
    
    // 디바이스 파일 오픈
    if((ws2812b_fd = open(DEV_NAME, O_RDWR | O_NONBLOCK)) < 0){
        perror("open()");
        return -1;
    }else
        printf("LED Device Driver Open Sucess!\n"); 

    return 0;
}
/*
* ws2812b 닫기
* void ws2812b_close(void)
* 입력 값 : 없음
* 반환 값 : 없음
* 설명   : 디바이스 파일 닫기.
*/
void ws2812b_close(void)
{
    close(ws2812b_fd);
}

/*
* ws2812b 색 설정(설정한 led 색만 유지됨)
* int ws2812b_set_led(int index, int color, int brightness)
* 입력 값 : index      ==> 설정하려는 led 번호
          color      ==> 설정하려는 RGB 색
          brightness ==> 설정하려는 밝기
* 반환 값 : 성공 0 / 실패 -1
* 설명   : 지정된 led의 색을 설정한다. 그외의 led는 모두 꺼진다.
*/
int ws2812b_set_led(int index, int color, int brightness)
{
    int ret = -1;
    ws2812b_led led[MAX_NUM_OF_LEDS_PER_WS2812B_MODULE] ={0,};
    
    s_led[index].index      = led[index].index        = index;
    s_led[index].rgb        = led[index].rgb          = (color > 0xffffff) ? 0xffffff : color;
    s_led[index].brightness = led[index].brightness   = (brightness > 255) ? 255 : brightness;

    if((ret = write(ws2812b_fd, led, sizeof(ws2812b_led)*MAX_NUM_OF_LEDS_PER_WS2812B_MODULE))<0)
        return -1;

#ifdef LED_PRINT
    printf("led index : %d RGB : %x Brightness : %x\n",index,led[index].rgb,led[index].brightness);
#endif
    return 0;
}

/*
* ws2812b 색 설정(설정한 led 색 유지됨)
* int ws2812b_set_static_led(int index, int color, int brightness)
* 입력 값 : index      ==> 설정하려는 led 번호
          color      ==> 설정하려는 RGB 색
          brightness ==> 설정하려는 밝기
* 반환 값 : 성공 0 / 실패 -1
* 설명   : led의 색을 설정한다. 설정한 색은 지속적으로 유지.
*/
int ws2812b_set_static_led(int index, int color, int brightness)
{
    int ret = -1;
    if(index > MAX_NUM_OF_LEDS_PER_WS2812B_MODULE) return -1;

    s_led[index].index        = index;
    s_led[index].rgb          = (color > 0xffffff) ? 0xffffff : color;
    s_led[index].brightness   = (brightness > 255) ? 255 : brightness;

    if((ret = write(ws2812b_fd, s_led, sizeof(ws2812b_led)*MAX_NUM_OF_LEDS_PER_WS2812B_MODULE))<0)
        return -1;

#ifdef LED_PRINT
    printf("led index : %d RGB : %x Brightness : %x\n",index,s_led[index].rgb,s_led[index].brightness);
#endif
    return 0;
}

/*
* ws2812b 끄기
* int ws2812b_clear_led(void)
* 입력 값 : 없음
* 반환 값 : 성공 0 / 실패 -1
* 설명   : led를 초기화. 설정된 색도 모두 초기화된다.
*/
int ws2812b_clear_led(void)
{
    int i,ret = -1;
    for(i=0; i <MAX_NUM_OF_LEDS_PER_WS2812B_MODULE; i++){
        s_led[i].index        = 0;
        s_led[i].rgb          = 0;
        s_led[i].brightness   = 0;
    }

    if((ret = write(ws2812b_fd, s_led, sizeof(ws2812b_led)*MAX_NUM_OF_LEDS_PER_WS2812B_MODULE))<0)
        return -1;
    return 0;
}
