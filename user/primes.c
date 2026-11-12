#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int p[2];
    if (pipe(p) < 0)
    {
        fprintf(2, "pipe failed\n");
        exit(1);
    }

    int pid = fork();
    if (pid < 0)
    {
        fprintf(2, "fork failed\n");
        exit(1);
    }

    if (pid > 0)
    {
        // ========== 主进程：生成 2..35 的数字 ==========
        close(p[0]); // 主进程只写不读
        for (int i = 2; i <= 35; i++)
        {
            write(p[1], &i, sizeof(int));
        }
        close(p[1]); // 写完立刻关闭，向下游传递 EOF
        wait(0);     // 等待所有子进程退出
        exit(0);
    }

    // ========== 子进程：进入流水线筛选循环 ==========
    close(p[1]);
    int left_read_fd = p[0];

    while (1)
    {
        int prime;
        // 1. 读取当前阶段的第一个数，必定是素数
        int n = read(left_read_fd, &prime, sizeof(int));
        if (n == 0)
        {
            // 上游已经全部发完，关闭管道并正常退出
            close(left_read_fd);
            exit(0);
        }

        // 打印素数
        printf("prime %d\n", prime);

        // 2. 创建通向下游的管道
        int right_pipe[2];
        if (pipe(right_pipe) < 0)
        {
            fprintf(2, "pipe failed\n");
            exit(1);
        }

        int cpid = fork();
        if (cpid < 0)
        {
            fprintf(2, "fork failed\n");
            exit(1);
        }

        if (cpid == 0)
        {
            // ----- 下游子进程 -----
            // 关闭不再需要的描述符
            close(left_read_fd);
            close(right_pipe[1]);
            // 把右侧管道读端作为下一轮循环的输入管道，继续 loop
            left_read_fd = right_pipe[0];
        }
        else
        {
            // ----- 当前进程：负责过滤并发送 -----
            close(right_pipe[0]); // 当前进程不读右侧管道

            int num;
            while (read(left_read_fd, &num, sizeof(int)) != 0)
            {
                // 不能被当前素数整除的数，推给下游
                if (num % prime != 0)
                {
                    write(right_pipe[1], &num, sizeof(int));
                }
            }

            close(left_read_fd);
            close(right_pipe[1]); // 关键：关闭写端触发下游 EOF
            wait(0);              // 等待直接子进程退出
            exit(0);
        }
    }
}