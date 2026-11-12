#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char const *argv[])
{
    int p2c[2];
    int c2p[2];
    char buf = 'A';

    // create pipe
    if (pipe(p2c) < 0 || pipe(c2p) < 0)
    {
        fprintf(2, "pipe failed\n");
        exit(1);
    }

    // create sub process
    int pid = fork();
    if (pid < 0)
    {
        fprintf(2, "fork failed\n");
        exit(1);
    }

    if (pid == 0)
    {
        // sub process fork
        // shut down non-used pipe
        close(p2c[1]);
        close(c2p[0]);

        // read father process ping
        if (read(p2c[0], &buf, 1) != 1)
        {
            fprintf(2, "child read failed\n");
            exit(1);
        }

        // output in format
        printf("%d: received ping\n", getpid());

        // return to father process(pong)
        if (write(c2p[1], &buf, 1) != 1)
        {
            fprintf(2, "child write failed\n");
            exit(1);
        }

        close(p2c[0]);
        close(c2p[1]);
        exit(0);
    }
    else
    {
        // father process
        // close non-used pipe
        close(p2c[0]);
        close(c2p[1]);

        // send one byte to son process
        if (write(p2c[1], &buf, 1) != 1)
        {
            fprintf(2, "parent write failed\n");
            exit(1);
        }

        // read one byte that return by son process
        if (read(c2p[0], &buf, 1) != 1)
        {
            fprintf(2, "parent read failed\n");
            exit(1);
        }

        // output in strict format
        printf("%d: received pong\n", getpid());

        close(p2c[1]);
        close(c2p[0]);

        // wait son process completely quit
        wait(0);
        exit(0);
    }
    return 0;
}
