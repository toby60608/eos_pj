/*
 * Maple Chen 2022
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int etx_device_fd=0;

int gpio_init(void)
{
    etx_device_fd = open("/dev/etx_device", O_RDWR);

    if (etx_device_fd == -1)
    {
        printf("ERR: file open failed: %s\n","/dev/etx_device");
        return -1;
    }
    printf("File open done: fd=%d\n",etx_device_fd);
    return 0;
}

int gpio_exit(void)
{
    close(etx_device_fd);
    return 0;
}

int write_a_digit_to_7segLED(int digit)
{
    write(etx_device_fd, (void *)&digit, sizeof(digit));
    return 0;
}


void userInput_to_7segLED(char *s)
{
    int i = 0;

    while (s[i] != 0)
    {
        int d = s[i] - '0';
        printf("write %i to 7-seg LED\n", d);
        write_a_digit_to_7segLED(d);
        sleep(1);
        i++;
    }
}

int main(int argc, char *argv[])
{
    int user_input_i;
    char *user_input_s;

    if (argc != 2)
    {
        printf("ERROR: Need one and only one parameter!\n");
        return -1;
    }
    else
    {
        user_input_s = argv[1];
        user_input_i = atoi(user_input_s);
        printf("user_input_s=%s\n", user_input_s);
        printf("user_input_i=%d\n", user_input_i);
    }

    gpio_init();

    userInput_to_7segLED(user_input_s);

    gpio_exit();
}
