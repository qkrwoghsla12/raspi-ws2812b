/* 라즈베리파이3 -  ws2812b 예제 프로그램
* 
* 
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/kdev_t.h>
#include <stdint.h>
#include <stdlib.h>
#include "ws2812b.h"

#define RED             0xFF0000  // red
#define ORANGE          0xFF7F00  // orange
#define YELLOW          0xFFFF00  // yellow
#define GREEN           0x00FF00  // green
#define LIGHTBLUE       0x00FFFF  // lightblue
#define BLUE            0x0000FF  // blue
#define PURPLE          0x7F007F  // purple
#define PINK            0xFF007F  // pink

void ws2812b_rgb_insert(ws2812b_led * led ,int index, unsigned int RGB, unsigned int brightness);
void print_led(ws2812b_led * led);

int main(int argc, char *argv[]){
    ws2812b_led led[8] ={0,};
    char cmd = NULL;
    int fd = 0, ret = 0 ,i;
    unsigned int rgb =0, bright = 0, Rainbow[8] ={RED,ORANGE,YELLOW,GREEN,LIGHTBLUE,BLUE,PURPLE,PINK};
    
    // /dev/ 밑의 디바이스 파일 생성    
    mknod(DEV_NAME,S_IRWXU|S_IRWXG|S_IFCHR,(DEV_LED_MAJOR_NUMBER<<8)|DEV_LED_MINOR_NUMBER);

    // 디바이스 파일 오픈
    if((fd = open(DEV_NAME, O_RDWR | O_NONBLOCK)) < 0){
        perror("open()");
        exit(1);
    }else
        printf("LED Device Driver Open Sucess!\n"); 

    printf("Usage 0 ~ 7 : Control Related led, a:ALL , b:Rainbow , c:brightness change ,d: One by one , f: clear , q: quit\n");

    while(cmd != 'q'){
        printf("\rInpunt Command : ");
        scanf("%c",&cmd);
        while (getchar() != '\n');

        switch(cmd){
            //led 초기화
            case 'f':
            case 'F':
                for(i=0; i<MAX_NUM_OF_LEDS_PER_WS2812B_MODULE; i++){        
                    ws2812b_rgb_insert(led, i, 0x0, 0);
                }
                write(fd, led,sizeof(ws2812b_led));
                break;

            //백색, 밝기를 다르게     
            case 'a':
            case 'A':
                for(i=0; i<MAX_NUM_OF_LEDS_PER_WS2812B_MODULE; i++){        
                    ws2812b_rgb_insert(led, i, 0xffffff, i*40);
                }
                write(fd, led,sizeof(ws2812b_led));
                break;

            //무지개색
            case 'b':
            case 'B':
                for(i=0; i<MAX_NUM_OF_LEDS_PER_WS2812B_MODULE; i++){
                    ws2812b_rgb_insert(led, i, Rainbow[i], 255);
                }
                write(fd, led,sizeof(ws2812b_led));
                break;

            //밝기를 변화
            case 'c':
            case 'C':
                for(bright=10; bright<255; bright+=10){
                    for(i=0; i<MAX_NUM_OF_LEDS_PER_WS2812B_MODULE; i++){
                        ws2812b_rgb_insert(led, i, Rainbow[i], bright);
                    }
                    write(fd, led,sizeof(ws2812b_led));
                    sleep(1);
                }
                break;

            //하나씩 
            case 'd':
            case 'D':
                bright = 255;
                for(i=0; i<MAX_NUM_OF_LEDS_PER_WS2812B_MODULE; i++){
                    ws2812b_rgb_insert(led, i, Rainbow[i], bright);
                    write(fd, led,sizeof(ws2812b_led));
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

                ws2812b_rgb_insert(led,atoi(&cmd),rgb,bright);
                write(fd, led,sizeof(ws2812b_led));
                break;

            case 'q' :
            default :
                break; 
        }
    }
    
    //파일 닫기
    if((ret = close(fd))<0){
        perror("close()");
        exit(1);
    }else
        printf("LED Device Driver Close Sucess!\n");
    return 0;
}

void ws2812b_rgb_insert(ws2812b_led * led ,int index, unsigned int RGB, unsigned int brightness){
    ws2812b_led * tmp_led = led;
    (tmp_led+index)->index = index;
    (tmp_led+index)->rgb = (RGB > 0xffffff) ? 0xffffff : RGB;
    (tmp_led+index)->brightness = (brightness > 255) ? 255 : brightness;
#ifdef DEBUG
    printf("led index : %d RGB : %x Brightness : %x\n",index,(tmp_led+index)->rgb,(tmp_led+index)->brightness);
#endif
}

#ifdef DEBUG
void print_led(ws2812b_led * led)
{
    int i=0;
    for(i=0 ; i<MAX_NUM_OF_LEDS_PER_WS2812B_MODULE; i++){
         printf("led index : %d RGB : %x Brightness : %x\n",i,(led+i)->rgb,(led+i)->brightness);
    }

}
#endif