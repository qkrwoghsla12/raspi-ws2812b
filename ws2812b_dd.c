/*
*********************************************************************************************************
*                                   RASPBERRY PI3 WS2812B DEVICE DRIVER
* 해당 디바이스 드라이버 파일은 라즈베리파이3의 ws2812B 모듈에 대한 캐릭터 디바이스 드라이버 입니다.
* http://www.nulsom.com/의 NS-led-02모델을 대상으로 테스트를 완료하였고 
* 기타 이외의 NEOPIXEL의 WS2812B 모델에 대해서도 적용 가능합니다.
* 기본적으로는 https://github.com/FiveNinjas/linux/blob/slice-4.1.y/drivers/misc/ws2812.c
* 의 ws2812b 플랫폼 디바이스 드라이버를 참조하여 작성하였습니다.
* 라즈베리파이의 PWM0 신호를 사용하여 제어하므로 pin18(BCM기준)에 연결하여 동작 시킵니다.
* 그 외의 연결상 주의 사항은  https://learn.adafruit.com/adafruit-neopixel-uberguide/downloads?view=all을
* 참조하여 주시기 바랍니다.
*********************************************************************************************************
*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/ioctl.h>
#include <asm/uaccess.h>  
#include "ws2812b.h"

/*디버그 옵션*/
//#define LED_DEBUG
/*
*********************************************************************************************************
*                                  RASPBERRY PI3 DEFINE PIN & REGISTER
*********************************************************************************************************
*/
/*라즈베리파이 핀 설정*/
#define DAT_PIN            18                               /* 사용할 핀 설정 */
#define DAT_PIN_FNC_MSK    0b010                            /* pin 18에서의 Func. 5 ==> pwm0 */

/* I/O Base 주소 */ 
#ifndef BCM2708_PERI_BASE
    #define BCM2708_PERI_BASE   0x3F000000
    #define GPIO_BASE       (BCM2708_PERI_BASE + 0x200000)  /* GPIO controller */
#endif
#define PWM_BASE        (BCM2708_PERI_BASE + 0x20C000)      /* PWM controller */
#define CLOCK_BASE      (BCM2708_PERI_BASE + 0x101000)      /* CLOCK controller */

/* I/O 가상 주소 BCM2835 데이터 시트 참조*/
static int iom_gpio, iom_pwm, iom_clk;
#define PWMCTL             ((void *)(iom_pwm+0x00))         /*PWM Control*/
#define PWMSTA             ((void *)(iom_pwm+0x04))         /*PWM Status*/
#define PWMRNG1            ((void *)(iom_pwm+0x10))         /*PWM Channel1 Range*/
#define PWMFIF1            ((void *)(iom_pwm+0x18))         /*PWM FIFO Input*/
#define CM_PWMCTL          ((void *)(iom_clk+0xa0))         /*GPIO CLK Control*/
#define CM_PWMDIV          ((void *)(iom_clk+0xa4))         /*GPIO CLK DIVI*/
#define GPFSEL_BASE        ((void *)(iom_gpio+0x00))        /*GPIO SET*/
#define GPFCLR_BASE        ((void *)(iom_gpio+0x28))        /*GPIO CLR*/

/* PWM 파라미터 */
#define PWM_COUNT          12
#define PWM_DIV            2

/* 전역 변수 선언*/
spinlock_t ws2812b__lock;

typedef struct{
    uint8_t grb[3];
}led_data;

/*
*********************************************************************************************************
*                                       WS2812B OPERATE FUNCTION
*********************************************************************************************************
*/

/*
* PWM 클락 동작 정지
* static int ws2812_led_init_clockStop(void)
* 입력 값 : 없음
* 반환 값 : 성공 0 / 실패 -1
* 설명 : PWM 클락을 원하는 주기의 클락으로 실행시키기 이전에 PWM 클락의 동작을 정지시킨다.
*/
static int ws2812_led_init_clockStop(void)
{
    int timeout;
    uint32_t  old;
    /* Stop clock generator by just changing the ENAB bit, so read-modify-write*/
        /* Make sure read updated value */  
    rmb();  
    old=readl(CM_PWMCTL)&0b11100111111;
    writel((0x5a000000|old)&(~(1<<4)), CM_PWMCTL ); //Stop clock generator  

#ifdef LED_DEBUG
    printk("CM_PWMCTL ==> %x : 0x%8x\n",CM_PWMCTL,readl(CM_PWMCTL));
#endif
    
        /* Make sure value is writen to register */ 
    wmb();
    /* Mow check if clock generator is stopped*/
        /* Make sure clock generator is not busy before doing anything*/
    timeout=0;  
    /* Make sure read updated value */  
    rmb();
    while(readl(CM_PWMCTL)&0x80)              //wait until busy flag clear
    {
        rmb();
        timeout++;
        if (timeout>=8000)
        {
            return -EBUSY;
        }
    }
    

    return 0;
}

/*
* PWM 클락 시작
* static int ws2812b_led_init_pwmClock(void)
* 입력 값 : 없음
* 반환 값 : 성공 0 / 실패 -1
* 설명 : 원하는 주기의 클락주기에 맞춰 클락을 동작시킨다.
*/ 
static int ws2812b_led_init_pwmClock(void)
{
    int divisor=PWM_DIV ;

    /* Set divisor*/
    writel(0x5a000000|(divisor<<12), CM_PWMDIV ); 
#ifdef LED_DEBUG
    printk("CM_PWMDIV ==> %x : 0x%8x\n",CM_PWMDIV,readl(CM_PWMDIV));    
#endif
    /* To start clock generator, ENAB and other bits cannot be set simultaneously, so, two steps */
    
    /* 1. Set clock source*/
    writel(0x5a000000|(1<<0), CM_PWMCTL );  //Set clock source: oscillator  
        /* Make sure value is writen to register before going to next step */
#ifdef LED_DEBUG
    printk("CM_PWMCTL ==> %x : 0x%8x\n",CM_PWMCTL,readl(CM_PWMCTL));
#endif
    wmb();
    /* 2. Set ENAB bit to start clock generator*/
    writel(0x5a000000|(1<<4)|(1<<0), CM_PWMCTL );   //Start clock generator 
#ifdef LED_DEBUG
    printk("CM_PWMCTL ==> %x : 0x%8x\n",CM_PWMCTL,readl(CM_PWMCTL));
#endif
    /* start clock generator as soon as possible*/
    wmb();
    
    return 0;
}
/*
* GPIO핀 설정
* static int ws2812_led_init_gpioPin(void)
* 입력 값 : 없음
* 반환 값 : 성공 0 / 실패 -1
* 설명 : GPIO핀에 대하여 ALT 설정 및 모드를 설정한다.
*/ 
static int ws2812_led_init_gpioPin(void)
{
    uint32_t old,msk;
    int registerOffset,bitOffset;
    
    registerOffset=4*((DAT_PIN)/10);
    /* Read-Modify-Write*/
    rmb();  
    old=readl((void *)(GPFSEL_BASE+registerOffset)); 
    
    bitOffset=3*((DAT_PIN)%10);
    msk=0b111 << bitOffset;          
    writel((old& ~msk), ((void *)(GPFSEL_BASE+registerOffset)) );  // First set pin as input 
    writel((old& ~msk) | ((DAT_PIN_FNC_MSK<<bitOffset)&msk), ((void *)(GPFSEL_BASE+registerOffset)) ); // then, set it as alt. func. 5 (PWM0)
#ifdef LED_DEBUG
    printk("GPIO SET ==> %x :  0x%8x\n",((void *)(GPFSEL_BASE+registerOffset)),readl(((void *)(GPFSEL_BASE+registerOffset))));
#endif
    return 0;
}

/*
* PWM 시작
* static int ws2812b_pwm_init(void)
* 입력 값 : 없음
* 반환 값 : 성공 0 / 실패 -1
* 설명 : PWM을 동작시킨다
*/
static int ws2812b_pwm_init(void)
{
    int count=PWM_COUNT;
    /* Clear PWMCTL register*/
    writel(0, PWMCTL ); 
#ifdef LED_DEBUG
    printk("CLEAR PWM CTL ==> %x : 0x%8x\n",PWMCTL,readl(PWMCTL));
#endif    
    /* Set PWM range*/
    writel(count, PWMRNG1 ); 
#ifdef LED_DEBUG
    printk("SET PWM RNG ==> %x : 0x%8x\n",PWMRNG1,readl(PWMRNG1));
#endif    
    /* Clear PWM FIFO buffer by setting CLRF1 bit*/
    writel((1<<6), PWMCTL );  //Clear FIFO
#ifdef LED_DEBUG
    printk("CLEAR PWM FIFO BUFFER ==> %x : 0x%8x\n",PWMCTL,readl(PWMCTL));
#endif
    /* Make sure FIFO is cleared before starting PWM*/
    wmb();

    /* Start PWM*/
    /*MSEN1=1; USEF1=1; POLA1=0; SBIT1=0; RPTL1=0; MODE1=0; PWEN1=1 */
    //MSEN M/S 전송 사용, FIFO 활성화, 비트 반전은 0, 전송 안할때 출력 없음,  데이터 반복 없음, PWM모드, 채널 활성화 
    writel((1<<7)|(1<<5)|(0<<3)|(0<<4)|(0<<2)|(1<<0)|(0<<1), PWMCTL); 
#ifdef LED_DEBUG
    printk("PWM CTL ==> %x : 0x%8x\n",PWMCTL,readl(PWMCTL));
#endif    
    return 0;
}

/* 
* 비트에 따른 PWM 신호 생성
* static int inline ws2812b_led_sendBit(uint8_t b)
* 입력 값 : bit : 0 ==> 0을 보냄. 총 1200ns의 펄스 폭에서 400ns동안 1 / 800ns 동안 0 ==> 0
                1 ==> 1을 보냄. 총 1200ns의 펄스 폭에서 800ns동안 1 / 400ns 동안 0 ==> 1
                2 ==> Reset 를 보냄. 총 1200ns 펄스 폭 전체에서 0 ==> Reset
* 반환 값 : 성공 0 / 실패 -1
* 설명 : PWM_FIFO에 데이터를 써서 PWM 신호 생생 
*       자세한 사항은 ws2812b 데이터 시트 참조. 
*/
static int inline ws2812b_led_sendBit(uint8_t b)
{
    int timeout=0;
    /* Make sure read updated register value*/
    rmb();
    while(readl(PWMSTA)&0x01)              //wait until fifo full flag clear
    {
        timeout++;
        if (timeout>=8000)
        {   
            /*  here is how to handle timeout*/
            return -EIO;
        }
        /* Make sure each readl reads latest value*/
        rmb();
    }
    
    switch (b)      
    {
    case 0:
        writel(4, PWMFIF1 ); // Send short pulse (400 ns) = 0
        break;
    case 1:
        writel(8, PWMFIF1 ); // send wide pulse (800 ns) = 1
        break;
    default:
        writel(0, PWMFIF1 ); // send no pulse, for WS2812b reset, send ~40 this pulses. 
        break;
    }
    /* Make sure date is writen to FIFO as soon as possible*/
    wmb();

    return 0;
}

/*
* LED 8비트 데이터(R,G,B) 보내기
* static int inline ws2812b_led_sendData(uint8_t dat)
* 입력 값 : 8bit data ==> R,G,B에 해당하는 각각의 데이터 
* 반환 값 : 성공 0 / 실패 -1
* 설명 : 한비트씩 시프트 하며 최상위 비트부터 순서대로 보냄. 
*/
static int inline ws2812b_led_sendData(uint8_t dat)
{
    int i,ret;
    
    
    for (i=0;i<8;i++)
    {
        ret=ws2812b_led_sendBit((dat&0x80)?1:0);
        if (ret<0)
        {
            return ret;
        }       
        dat<<=1;
    }
    return 0;

}

/*
* LED 리셋
* static int inline ws2812b_led_sendRst(void)
* 입력 값 : 없음
* 반환 값 : 성공 0 / 실패 -1
* 설명 : 데이터간의 구분을 위한 리셋 신호를 보냄
*       리셋 신호는 50us이상 보내야 함.
*       따라서 PWM reset 신호를 50회 반복
*       자세한 사항은 ws2812b 데이터 시트 참조. 
*/
static int inline ws2812b_led_sendRst(void)
{
    int i,ret;
    for (i=0;i<50;i++)
    {
        ret=ws2812b_led_sendBit(2);
        if (ret<0)
        {
            return ret;
        }
    }
    return 0;
}

/*
* LED array rgb 데이터 쓰기
* static inline int ws2812b_led_update(led_data * data)
* 입력 값 : data ==> led의 rgb값이 저장되어있는 led_data 포인터
* 반환 값 : 성공 0 / 실패 -1
* 설명 : 직렬 연결된 led 모듈 개수의 배열 포인터를 받아 순서대로 데이터를 보냄.
*       본 예제에서는 8개의 led 모듈에 대해서 순서대로 r,g,b값을 씀.
*/
static inline int ws2812b_led_update(led_data * data)
{
    unsigned long flags;
    int i,j,ret;

    /* this spin lock prevent task scheduling thus prevent buffer underflow*/
    spin_lock_irqsave(&ws2812b__lock,flags);

    ret=ws2812b_led_sendRst();
    if (ret<0)
    {
        spin_unlock_irqrestore(&ws2812b__lock,flags);
        return ret;
    }
    for (i=0;i<(MAX_NUM_OF_LEDS_PER_WS2812B_MODULE);i++)
    {
        for(j=0;j<3;j++){
            ret=ws2812b_led_sendData(data[i].grb[j]);
            if (ret<0)
            {
                spin_unlock_irqrestore(&ws2812b__lock,flags);
                return ret;
            }
        }
    }
    /* not sure why I need to send a 0 to force the PWM stop, even after disabling the RPTL1 in PWMCTL*/
    ret=ws2812b_led_sendBit(2);
    if (ret<0)
    {
        return ret;
    }

    spin_unlock_irqrestore(&ws2812b__lock,flags);
    return 0;
}

/*
* WS2812B gamma correction
* static unsigned char gamma_(unsigned char brightness, unsigned char val)
* 입력 값 : brihtness ==> 밝기값
          val ==> 색상 값
* 반환 값 : 밝기 보정된 색상 값
* 설명 : 감마 보정을 위한 함수.
*       밝기와 색상을 입력받아 해당 밝기의 색상을 리턴. 
*       GammaE=255*(res/255).^(1/.45)
*       From: http://rgb-123.com/ws2812-color-output/
*/
static unsigned char gamma_(unsigned char brightness, unsigned char val)
{
    int bright = val;
    unsigned char GammaE[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2,
    2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5,
    6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 11, 11,
    11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18,
    19, 19, 20, 21, 21, 22, 22, 23, 23, 24, 25, 25, 26, 27, 27, 28,
    29, 29, 30, 31, 31, 32, 33, 34, 34, 35, 36, 37, 37, 38, 39, 40,
    40, 41, 42, 43, 44, 45, 46, 46, 47, 48, 49, 50, 51, 52, 53, 54,
    55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70,
    71, 72, 73, 74, 76, 77, 78, 79, 80, 81, 83, 84, 85, 86, 88, 89,
    90, 91, 93, 94, 95, 96, 98, 99,100,102,103,104,106,107,109,110,
    111,113,114,116,117,119,120,121,123,124,126,128,129,131,132,134,
    135,137,138,140,142,143,145,146,148,150,151,153,155,157,158,160,
    162,163,165,167,169,170,172,174,176,178,179,181,183,185,187,189,
    191,193,194,196,198,200,202,204,206,208,210,212,214,216,218,220,
    222,224,227,229,231,233,235,237,239,241,244,246,248,250,252,255};
    bright = (bright * brightness) / 255;
    return GammaE[bright];
}


/*
*********************************************************************************************************
*                                   DEVICE DRIVER FUNC
*********************************************************************************************************
*/
/* 
* led_open 함수
* static int led_open(struct inode *inode, struct file *filp)
* 설명 : 유저 영역의 어플리케이션에서 open()을 호출시 동작.
*/
static int led_open(struct inode *inode, struct file *filp){
    printk("[WS2812b] led_open()\n");
    return 0;
}

/* 
* led_release 함수
* static int led_release(struct inode *inode, struct file *filp)
* 설명 : 유저 영역의 어플리케이션에서 close()을 호출시 동작.
*/ 
static int led_release(struct inode *inode, struct file *filp){
    printk("[WS2812b] led_close()\n");
    return 0;
}

/* 
* led_ioctl 함수
* static int led_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
* 설명 : 유저 영역의 어플리케이션에서 ioctl()을 호출시 동작.
*/ 
static int led_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
    #ifdef LED_DEBUG 
        printk("ioctl enter!\n");
    #endif
        return 0;
}

/* 
* led_write 함수
* static ssize_t led_write(struct file *filp, const void * buf, size_t size, loff_t *offset){ 
* 설명 : 유저 영역의 어플리케이션에서 write()을 호출시 동작.
*       ws2812b_led 구조체 배열을 받아 저장한 후 각각 led의 rgb 값을 grb순서로 변경하여 led_data 배열에 저장.
*       이때 밝기와 색상에 따라 데이터값을 조정하여 저장함. 그 후 led_data 배열을 led_update()함수에 넘겨주어 pwm 펄스를 생성하여 보냄. 
*/ 
static ssize_t led_write(struct file *filp, const void * buf, size_t size, loff_t *offset){ 

    ws2812b_led tmp_data[MAX_NUM_OF_LEDS_PER_WS2812B_MODULE]={0,};
    led_data led_buffer[MAX_NUM_OF_LEDS_PER_WS2812B_MODULE] = {0,};
    int ret,i,rtn;

#ifdef LED_DEBUG 
    printk("write enter!\n");
#endif

    if((ret = copy_from_user(tmp_data, (ws2812b_led *)buf, sizeof(ws2812b_led)*MAX_NUM_OF_LEDS_PER_WS2812B_MODULE))<0)
        printk("[gpio-led] write data copy error \n");

#ifdef LED_DEBUG
    for(i=0 ; i<MAX_NUM_OF_LEDS_PER_WS2812B_MODULE; i++){
         printk("led index : %d RGB : %x Brightness : %x\n",(tmp_data+i)->index,(tmp_data+i)->rgb,(tmp_data+i)->brightness);
    }
#endif

    for(i=0;i<MAX_NUM_OF_LEDS_PER_WS2812B_MODULE;i++){
        led_buffer[(tmp_data+i)->index].grb[0] = gamma_((tmp_data+i)->brightness,((tmp_data+i)->rgb >> 8)); 
        led_buffer[(tmp_data+i)->index].grb[1] = gamma_((tmp_data+i)->brightness,((tmp_data+i)->rgb >> 16)); 
        led_buffer[(tmp_data+i)->index].grb[2] = gamma_((tmp_data+i)->brightness,((tmp_data+i)->rgb >> 0)); 
    
#ifdef LED_DEBUG
    printk("tmp_data G: %x R: %x B: %x brightness: %x \n",(tmp_data+i)->rgb>>8,(tmp_data+i)->rgb>>16,(tmp_data+i)->rgb>>0,(tmp_data+i)->brightness);
    printk("led_data G: %x R: %x B: %x \n",led_buffer[(tmp_data+i)->index].grb[0],led_buffer[(tmp_data+i)->index].grb[1],led_buffer[(tmp_data+i)->index].grb[2]);
#endif

    }
    
    if((rtn = ws2812b_led_update(led_buffer))<0)
       printk("[gpio-led] write error\n");

    return ret;    
}

static struct file_operations led_fops = {
    .owner      = THIS_MODULE,
    .open       = led_open,
    .release    = led_release,
    //.read     = led_read,
    .write      = led_write,
    .unlocked_ioctl    = led_ioctl,
};

/* 
* led_init함수
* static int led_init(void)
* 설명 : insmod 시 동작.
*       라즈베리파이 하드웨어 레지스터 맵을 가상 주소에 매핑하고 필요한 기능들을 초기화함.
*/
static int led_init(void){
    int retry,ret=0;   
    printk("[WS2812b] WS2812B Init Func.\n");
    
    if (MAX_NUM_OF_LEDS_PER_WS2812B_MODULE<=0)  
        return -ENODEV;

    /* Initialize system resource */
    /* Get register base address */
    iom_gpio = (int)ioremap(GPIO_BASE, PAGE_SIZE);
    iom_pwm = (int)ioremap(PWM_BASE, PAGE_SIZE);
    iom_clk = (int)ioremap(CLOCK_BASE, PAGE_SIZE);

    printk("WS2812B Register Base Address: gpio : %x clk : %x  pwm : %x.\n",iom_gpio,iom_clk,iom_pwm);

    /* Initialize hardware*/
    printk("WS2812B HW_Init \n");

    /* Try to stop clock generator */
    retry=0;
    do  
    {
        ret=ws2812_led_init_clockStop();
        if (ret<0)
        {
            printk("WS2812B PWM_Clock_Timeout. \n");
            retry++;
        }
    }while((ret<0) && (retry<3));
    
    if (ret<0)
        return ret;
    
    /* Set new clock generator parameters*/
    printk("WS2812B Clock_Init \n");
    ret=ws2812b_led_init_pwmClock();
    if (ret<0)
        return ret;
    
    /* Set data pin as pwm0*/
    printk("WS2812B set pin func.\n");  
    ret=ws2812_led_init_gpioPin();
    if (ret<0)
        return ret;

    /*set pwm registers*/
    printk("WS2812B set PWM registers.\n"); 
    ret=ws2812b_pwm_init();
    if (ret<0)
        return ret;

    register_chrdev(DEV_LED_MAJOR_NUMBER, DEV_LED_NAME, &led_fops);

    return 0;
}

/* 
* led_exit함수
* static void led_exit(void)
* 설명 : rmmod 시 동작. 
*       매핑된 가상주소를 해제하고 테이블의 디바이스 드라이버의 등록을 해제함.
*/
static void led_exit(void){
    printk("[WS2812b] WS2812B Un-register.\n");
    iounmap((void *)iom_gpio);
    iounmap((void *)iom_pwm);
    iounmap((void *)iom_clk);
    unregister_chrdev(DEV_LED_MAJOR_NUMBER, DEV_LED_NAME);
}
 
module_init(led_init);
module_exit(led_exit);
 
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("LED charcter driver for WS2812B NeoPixel");
MODULE_AUTHOR("Shin uk cheol");
