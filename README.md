raspberry pi3 - NS-LED-2 Device Driver & App 
===============
(ws2812b character device driver)
----

##NS-LED-2 device
device site : http://www.nulsom.com/

ws2812b data sheet : http://www.nulsom.com/datasheet/WS2812B_datasheet.pdf


##Connectivity NS-LED-2 & raspberry pi3

NS-LED-2  //  raspberry pi3

+5V   --  pin 2,4(physical)   

DI    --  pin 12(physical)

GND   --  pin 6,9,14,20,25,30,34(physical)

##동작 방법

>*soundcard disable*

(raspberrypi) $ sudo blacklist snd_bcm2835 >> /etc/modprobe.d/raspi-lacklist.conf

(raspberrypi) $ sudo reboot

>after reboot,

(raspberrypi) $ aplay -l

>check no soundcards found...

>ref. http://www.instructables.com/id/Disable-the-Built-in-Sound-Card-of-Raspberry-Pi/step3/Test-that-sound-card-is-NOT-detected-by-ALSA-Nativ/

(raspberrypi) $ git clone https://github.com/WookCheolShin/raspi-ws2812b.git

(raspberrypi) $ cd raspi-ws2812b

(raspberrypi) $ make

(raspberrypi) $ gcc 1_ws2812b_app.c -o ws2812b_app

(raspberrypi) $ sudo insmod 1_ws2812b_dd.ko

(raspberrypi) $ sudo ./ws2812b_app
  
