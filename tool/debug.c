#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>

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
              nullptr
             );
    }
    
    pid[1] = fork();
    if (!pid[1]) {
        execl("/usr/bin/gdb", "gdb", "-q", nullptr);
        perror("execl gdb failed");
    }
    signal(SIGINT, SIG_IGN);
    
    while (wait(nullptr) != pid[1]);
    
    kill(pid[0], SIGTERM);
    waitpid(pid[0], nullptr, 0);

    return 0;
}
