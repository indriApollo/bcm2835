#include <stdio.h>
#include <sys/mman.h>
#include <stdint.h>
#include <signal.h>
#include <fcntl.h>

#define BMC2835_RPI2_DT_FILENAME "/proc/device-tree/soc/ranges"
#define BMC2835_RPI2_DT_PERI_BASE_ADDRESS_OFFSET 4
#define BMC2835_RPI2_DT_PERI_SIZE_OFFSET 8
#define BCM2835_GPIO_BASE 0x200000
#define GPFSEL1_OFFSET 0x04
#define GPIO_OUT_BIT 0x1 << 17
#define GPSET0 0x1c //set as in group of pins
#define GPCLR0 0x28

void sighandler(int signum);
volatile uint8_t flag = 1;

int main() {

   signal(SIGINT, sighandler);

   uint32_t *bcm2835_peripherals_base;
   uint32_t bcm2835_peripherals_size;
   volatile uint32_t *bcm2835_gpio;

   FILE *fp;
   int memfd = -1;

   if((fp = fopen(BMC2835_RPI2_DT_FILENAME,"rb"))) {
      fseek(fp, BMC2835_RPI2_DT_PERI_BASE_ADDRESS_OFFSET, SEEK_SET);
      unsigned char buf[4];
      if(fread(buf, 1, sizeof(buf), fp) == sizeof(buf))
         bcm2835_peripherals_base = (uint32_t *)(buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3] << 0);
         printf("peripherals base: %x \n", bcm2835_peripherals_base);
      fseek(fp, BMC2835_RPI2_DT_PERI_SIZE_OFFSET, SEEK_SET);
      if(fread(buf, 1, sizeof(buf), fp) == sizeof(buf))
         bcm2835_peripherals_size = (buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3] << 0);
         printf("peripherals size: %x \n", bcm2835_peripherals_size);
      fclose(fp);
   } else {
      puts("Unable to open proc. Are you root ?");
      return 1;
   }

   if((memfd = open("/dev/mem", O_RDWR | O_SYNC) ) < 0) {
      puts("Unable to open /dev/mem. Are you root ?");
      return 2;
   }

   uint32_t *map = mmap(NULL, bcm2835_peripherals_size, (PROT_READ | PROT_WRITE), MAP_SHARED, memfd, (uint32_t)bcm2835_peripherals_base);
   if(map == MAP_FAILED) {
      puts("mmap failed");
      return 3;
   }

   bcm2835_gpio = map + BCM2835_GPIO_BASE/4; // xyz/4 <-- MMU is 32 bit but we work in bytes
   printf("gpio: %x \n", bcm2835_gpio);

   uint32_t GPFSEL1 = *(bcm2835_gpio + GPFSEL1_OFFSET/4);

   uint32_t pin17_gpio_out = (GPFSEL1 & ~(0b111 << 21)) | (0b001 << 21); //mask bit 21-23 of GPFSEL1 to 001 (set as output)
   *(bcm2835_gpio + GPFSEL1_OFFSET/4) = pin17_gpio_out;

   while(flag) {
      *(bcm2835_gpio + GPSET0/4) = GPIO_OUT_BIT;
      sleep(1);
      *(bcm2835_gpio + GPCLR0/4) = GPIO_OUT_BIT;
      sleep(1);
   }

   *(bcm2835_gpio + GPSET0/4) = GPIO_OUT_BIT; //turn off  led
   *(bcm2835_gpio + GPFSEL1_OFFSET/4) = GPFSEL1; //restore initial register value
   munmap(bcm2835_peripherals_base, bcm2835_peripherals_size);
   return 0;
}

void sighandler(int signum)
{
    printf("recieved signal %s\n", strsignal(signum));
    flag = 0;
}
