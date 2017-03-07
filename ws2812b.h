#ifndef __WS2812B_H__
#define __WS2812B_H__

/*최대 모듈 갯수*/
#define MAX_NUM_OF_LEDS_PER_WS2812B_MODULE   8

/*디바이스 드라이버 정보*/
#define DEV_LED_MAJOR_NUMBER   221          /* 주번호 */
#define DEV_LED_MINOR_NUMBER   0          /* 주번호 */
#define DEV_LED_NAME           "gpio-led"   /* 모듈 이름 */
#define DEV_NAME 			   "/dev/ws2812b-led"

/*선택 옵션
*디버그 옵션
*/
//#define DEBUG

/*write함수에 사용할 구조체*/
typedef struct{
    uint8_t index;
    unsigned int rgb;
    uint8_t brightness;
}ws2812b_led;

#endif