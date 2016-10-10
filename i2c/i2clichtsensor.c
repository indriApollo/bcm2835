#include <bcm2835.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <unistd.h> //sleep()
#include <signal.h> //SIGINT

#define CHIPADDR 0x29
#define CMDWORD 0xa0
#define CMDNOWORD 0x80
#define CHIPON 0x03
#define ADC1LOW 0x0e

int flag = 1;

void sighandler(int signum);

int main() {

    if(!bcm2835_init()) {
        printf("bcm2835_init failed. Are you running as root ?\n");
        return 1;
    }

    if(!bcm2835_i2c_begin()) {
        printf("bcm2835_i2c_begin failed. Are you running as root?\n");
        return 1;
    }

    signal(SIGINT, sighandler);

    bcm2835_i2c_setSlaveAddress(CHIPADDR);
    //bcm2835_i2c_setClockDivider(2500); //100khz

    const char on_buf[] = {CMDNOWORD, CHIPON};
    bcm2835_i2c_write(on_buf,sizeof(on_buf));

    while(flag) {
	char buf[] = {(CMDWORD | ADC1LOW)};
        char rec_buf[16]; //2 x 8bit regs
        
bcm2835_i2c_read_register_rs(buf,rec_buf,sizeof(buf)+sizeof(rec_buf));
        printf("%i\n", rec_buf[0] + rec_buf[1]);
        sleep(1);
    }

    bcm2835_i2c_end();
    bcm2835_close();
    printf("i2clichtsensor terminated\n");
    return 0;
}

void sighandler(int signum)
{
    printf("recieved signal %s\n", strsignal(signum));
    flag = 0;
}

