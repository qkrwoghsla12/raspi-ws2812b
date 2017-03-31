/* 
* 라즈베리파이3 -  ws2812b 예제 프로그램
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include "ws2812b.h"

static void pabort(const char *s)
{
    perror(s);
    abort();
}

int main(int argc, char *argv[]){

    unsigned int rgb =0, bright = 0;
    unsigned int Rainbow[8] ={RED,ORANGE,YELLOW,GREEN,LIGHTBLUE,BLUE,PURPLE,PINK};
    char cmd = NULL;
    int ret = 0 , i = 0;

    //ws2812b 디바이스 셋업
    if((ret = ws2812b_setup())<0)
        pabort("set up");
    else
        printf("ws2812b setup ok \n");

    printf("Usage 0 ~ 7 : Control Related led, a:ALL , b:Rainbow , c:brightness change ,d: One by one , f: clear , q: quit\n");
    while(cmd != 'q'){
        printf("\rInpunt Command : ");
        scanf("%c",&cmd);
        while (getchar() != '\n');

        switch(cmd){
            //led 초기화
            case 'f':
            case 'F':
                ws2812b_clear_led();
                break;

            //백색, 밝기를 다르게     
            case 'a':
            case 'A':
                for(i=0; i<MAX_NUM_OF_LEDS_PER_WS2812B_MODULE; i++)        
                    ws2812b_set_static_led(i, 0xffffff, i*40);
                break;

            //무지개색
            case 'b':
            case 'B':
                for(i=0; i<MAX_NUM_OF_LEDS_PER_WS2812B_MODULE; i++)
                    ws2812b_set_static_led(i,Rainbow[i],255);
                break;

            //밝기를 변화
            case 'c':
            case 'C':
                for(bright=10; bright<255; bright+=20){
                    for(i=0; i<MAX_NUM_OF_LEDS_PER_WS2812B_MODULE; i++)
                        ws2812b_set_static_led(i, Rainbow[i], bright);
                    sleep(1);
                }
                break;

            //하나씩 
            case 'd':
            case 'D':
                bright = 255;
                for(i=0; i<MAX_NUM_OF_LEDS_PER_WS2812B_MODULE; i++){
                    ws2812b_set_led(i, Rainbow[i], bright);
                    sleep(1);
                }                
                break;

            //특정 led 선택하여 값 조정
            case '0' :
            case '1' :
            case '2' :
            case '3' :
            case '4' :
            case '5' :
            case '6' :
            case '7' :
                printf("\rInpunt RGB : 0x");
                scanf("%x",&rgb);
                while (getchar() != '\n');

                printf("\rInpunt Brightness (0~255): ");
                scanf("%d",&bright);
                while (getchar() != '\n');

                printf("NUM : %d, RGB : %x, Bright : %d \n",atoi(&cmd),rgb,bright);
                ws2812b_set_static_led(atoi(&cmd),rgb,bright);
                break;

            default :
                break; 
        }
    }
    
    ws2812b_close();
    return 0;
}
