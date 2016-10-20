#include <bcm2835.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>

volatile int flag = 1;
void sighandler(int signum);

int main() {

    if(!bcm2835_init()) {
        printf("bcm2835_init failed. Are you running as root ?\n");
        return 1;
    }

    if(!bcm2835_spi_begin()) {
        printf("bcm2835_spi_begin failed. Are you running as root?\n");
        return 1;
    }

    signal(SIGINT, sighandler);

    bcm2835_spi_chipSelect(BCM2835_SPI_CS1);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE3);
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS1, LOW);

    char reset[] = {0xff,0xff,0xff,0xff}; //hold line low for 32 clock pulses -> reset
    char mode16[] = {0x08,0x80}; //0x08: register addr  0x80: 16bit mode
    char continuous_mode[] = {0x54}; //allows continous read

    bcm2835_spi_transfern(reset, sizeof(reset));
    bcm2835_spi_transfern(mode16, sizeof(mode16));
    bcm2835_spi_transfern(continuous_mode, sizeof(continuous_mode));

    while(flag) {
       char reading[]= {0x00,0x00}; //send 2 bytes to get 2 bytes in return
       bcm2835_spi_transfern(reading, sizeof(reading));
       int16_t temp = reading[1]|(reading[0]<<8); //LSB or shift left 8 MSB
       printf("%f deg celsius\n",(double)temp/128.0);
       sleep(1);
    }

    puts("reset and close spi");
    bcm2835_spi_transfern(reset, sizeof(reset));
    bcm2835_spi_end();
    bcm2835_close();

    return 0;
}

void sighandler(int signum)
{
    printf("recieved signal %s\n", strsignal(signum));
    flag = 0;
}
