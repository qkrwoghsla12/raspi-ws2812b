/*
*********************************************************************************************************
*                                  				WS2812B_H
*********************************************************************************************************
*/
#ifndef __WS2812B_H__
#define __WS2812B_H__

/*디버그 옵션*/
//#define LED_PRINT

/*디바이스 드라이버 정보*/
#define DEV_LED_MAJOR_NUMBER   221              /* 주번호 */
#define DEV_LED_MINOR_NUMBER   0                /* 주번호 */
#define DEV_LED_NAME           "ws2812b-led"   /* 모듈 이름 */
#define DEV_NAME               "/dev/ws2812b-led"

/*
* 최대 모듈 갯수
* 모듈을 직렬로 연결시 해당하는 매크로의 숫자를 변경.
*/
#define MAX_NUM_OF_LEDS_PER_WS2812B_MODULE   8

//색
#define RED             0xFF0000  // red
#define ORANGE          0xFF7F00  // orange
#define YELLOW          0xFFFF00  // yellow
#define GREEN           0x00FF00  // green
#define LIGHTBLUE       0x00FFFF  // lightblue
#define BLUE            0x0000FF  // blue
#define PURPLE          0x7F007F  // purple
#define PINK            0xFF007F  // pink


/*write함수, 디바이스 드라이버에 사용할 구조체*/
typedef struct{
    uint8_t index;
    unsigned int rgb;
    uint8_t brightness;
}ws2812b_led;

/*
*********************************************************************************************************
*                                  				PREDEFINE ws2812 FUNC
*********************************************************************************************************
*/
int ws2812b_setup(void);
void ws2812b_close(void);
int ws2812b_set_led(int index, int color, int brightness);
int ws2812b_set_static_led(int index, int color, int brightness);
int ws2812b_clear_led(void);

#endif  //__WS2812B_H__
