#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <signal.h>

static int fd = -1;

/* SIGINT 处理函数 */
void sigint_handler(int sig)
{
    (void)sig;
    if (fd >= 0) {
        close(fd);
        write(1, "\n", 1);
    }
    exit(0);
}

int main(void)
{
    /* 打开串口，O_NOCTTY 防止成为控制终端 */
    fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    struct termios tty;

    /* 获取当前串口配置 */
    if (tcgetattr(fd, &tty) != 0) {
        perror("tcgetattr");
        close(fd);
        return -1;
    }

    /* 设置波特率 9600 */
    cfsetispeed(&tty, B9600);
    cfsetospeed(&tty, B9600);

    /* 控制标志：8 位数据、无校验、1 位停止位、无硬件流控 */
    tty.c_cflag &= ~(PARENB | CSTOPB | CSIZE | CRTSCTS);
    tty.c_cflag |= (CS8 | CREAD | CLOCAL);

    /* 本地标志：原始模式，关闭回显、规范模式、信号等 */
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ISIG | IEXTEN);

    /* 输入标志：关闭软件流控、换行转换等 */
    tty.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL | INLCR | IGNCR);

    /* 输出标志：关闭输出处理 */
    tty.c_oflag &= ~OPOST;

    /* 阻塞行为：read 至少读到 1 字节才返回 */
    tty.c_cc[VMIN]  = 1;
    tty.c_cc[VTIME] = 0;

    /* 应用配置 */
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        close(fd);
        return -1;
    }

    /* 设置 SIGINT 处理 */
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("signal");
        close(fd);
        return -1;
    }

    /* 创建子进程：子进程负责 串口 -> 标准输出，父进程负责 标准输入 -> 串口 */
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        close(fd);
        return -1;
    }

    if (pid == 0) {
        /* 子进程：不断从串口读 1 字节并输出到 stdout */
        char c;
        while (1) {
            ssize_t n = read(fd, &c, 1);
            if (n > 0) {
                write(STDOUT_FILENO, &c, 1);
            } else if (n < 0) {
                perror("read serial");
                break;
            }
        }
    } else {
        /* 父进程：不断从 stdin 读数据并写入串口 */
        char buf[64];
        while (1) {
            ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
            if (n > 0) {
                write(fd, buf, n);
            } else if (n < 0) {
                perror("read stdin");
                break;
            }
        }
    }

    /* 正常不会运行到这 */
    close(fd);
    return 0;
}
