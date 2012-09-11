#include "BoneHeader.h"
#include "i2c-dev.h"
#include <poll.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>


//inherits
//
// int export_gpio(int gpio);
// int unexport_gpio(int gpio);
// int set_gpio_direction(int gpio, char* direction);
// int set_gpio_value(int gpio, int value);
// int set_gpio_edge(int gpio, char* edge);
// int gpio_fd_open(int gpio);
// int gpio_fd_close(int fd);
// int set_mux_value(char* mux, int value);
// int read_ain(char* ain);
int keepgoing = 1;

//signal handler that breaks program loop and cleans up
void signal_handler(int signo){
	if (signo == SIGINT) {
		printf("\n^C pressed, unexporting gpios and exiting..\n");
		keepgoing = 0;
	}
}

int main(int argc, char** argv){
	
	//variable declarations
	struct pollfd fdset[1];
	int nfds = 1;
	int timeout = 100;
	int rc;
	char* buf[MAX_BUF];	
	
	int gpio1, gpio2, gpio_count1, gpio_count2;
	int gpio1_fd, gpio2_fd;
	int gpio2_value = 0;
	int program = 0;
	int ain = 0;
	float duty_cycle = 0;
	int i2c_read;
		
	//check that at least two arguments are passed in
	if(argc < 3){
		printf("Usage: %s <input-gpio> <output-gpio>\n", argv[0]);
		printf("polls input-gpio, and writes value to output-gpio\n");
		fflush(stdout);
		return 1;
	}

	//set signal handler
	if (signal(SIGINT, signal_handler) == SIG_ERR)
		printf("\ncan't catch SIGINT\n");

	//assign gpio values
	gpio1 = atoi(argv[1]);
	gpio2 = atoi(argv[2]);

	//argument 1 will be input
	export_gpio(gpio1);
	set_gpio_direction(gpio1, "in");
	set_gpio_edge(gpio1, "falling");
	gpio1_fd = gpio_fd_open(gpio1);

	//argument 2 will be output
	export_gpio(gpio2);
	set_gpio_direction(gpio2, "out");
	set_gpio_value(gpio2, gpio2_value);
	gpio2_fd = gpio_fd_open(gpio2);

	//prepare gpio_count1 and gpio_count2
	gpio_count1 = 30;
	gpio_count2 = 31;
	export_gpio(gpio_count1);
	export_gpio(gpio_count2);
	set_mux_value("gpmc_wait0",7);
	set_gpio_direction(gpio_count1, "out");
	set_gpio_direction(gpio_count2, "out");
	

	//prepare i2c bus
	int file;
	

	while(keepgoing){
		memset((void*)fdset, 0, sizeof(fdset));
	
		fdset[0].fd = gpio1_fd;
		fdset[0].events = POLLPRI;

		//fdset[1].fd = gpio2_fd;
		//fdset[1].events = POLLPRI;

		rc = poll(fdset, nfds, timeout);

		if (rc < 0){
			printf("\npoll() failed!\n");
		}

		if((fdset[0].revents & POLLPRI) == POLLPRI) {
			read(fdset[0].fd, buf, MAX_BUF);
			usleep(100);
			gpio2_value = ~(gpio2_value)&1;
			set_gpio_value(gpio2, gpio2_value);
			program++;
			if(program > 3) program = 0;
			set_gpio_value(gpio_count1, program & 1);
			set_gpio_value(gpio_count2, program & 2);
		}			
		
	
		switch(program){
			case 0:
				duty_cycle = read_ain("ain6");
				duty_cycle = duty_cycle/4095 * 100;
				set_mux_value("gpmc_a2", 6);
				set_pwm("ehrpwm.1:0", 1000, (int) duty_cycle);

			break;

			case 1:
				ain = read_ain("ain6");
				printf("ain5 = %d\n", ain);
				unset_pwm("ehrpwm.1:0");
			break;

			case 2: 
				unset_pwm("ehrpwm.1:0");
			break;

			case 3:
				if((file = open("/dev/i2c-3", O_RDWR)) < 0){
					printf("failed to open i2c-3 bus\n");
					return 1;
				}

				if (ioctl(file, I2C_SLAVE, 72) < 0){
					printf("Could not set address for i2c\n");
					return 1;
				}
				i2c_read = i2c_smbus_read_byte_data(file, 0);
				close(file);
				printf("0x%02x (%d)\n", i2c_read, i2c_read);
			break;

			default:
			break;

		}
	}

	gpio_fd_close(gpio1_fd);
	gpio_fd_close(gpio2_fd);
	unexport_gpio(gpio1);
	unexport_gpio(gpio2);
	unexport_gpio(gpio_count1);
	unexport_gpio(gpio_count2);
	unset_pwm("ehrpwm.1:0");
	set_mux_value("gpmc_a2", 7);

	fflush(stdout);
	return 0;
}
