#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid[2];
    pid[0] = fork();
    if (!pid[0]) {
        signal(SIGINT, SIG_IGN);
        close(1);
        open("./log/openocd.log", O_WRONLY | O_CREAT | O_TRUNC);
        dup2(1, 2);

        execl("/usr/bin/openocd", "openocd", \
              "-f", "interface/stlink.cfg", \
              "-f", "target/stm32f1x.cfg", \
              NULL
             );
    }
    sleep(1);
    pid[1] = fork();
    if (!pid[1]) {
        execl("/usr/bin/telnet", "telnet", \
              "localhost", "4444", \
              NULL
             );
    }
    signal(SIGINT, SIG_IGN);

    while (wait(NULL) != pid[1]);

    kill(pid[0], SIGTERM);
    waitpid(pid[0], NULL, 0);

    return 0;
}
