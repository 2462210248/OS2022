#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <algorithm>
#include <dirent.h>
using namespace std;

//参数数量
int argc = 0;

//输入命令的字符串
string input;

//参数数组,比如输入ls -a,argv[0]="ls",argv[1]="-a"
vector<string> argv;

//为了给execvp传参的数组
char *argv1[100];

//存放所有命令的数组
vector<string> command;

//标志符号:判断是否有重定向输出符号">"
int outputredirect = 0;

//标志符号:判断是否有重定向输入符号"<"
int inputredirect = 0;

//标志符号:判断是否有追加重定向输出符号">>"
int outputappendredirect = 0;

// 标志符号,判断是否有管道符号"|"
int haspipe = 0;

// 标志符号,判断是否有后台运行符号"&""
int background = 0;

// parse解析命令
void parse();

// 执行命令
void do_cmd();

// cd命令
int CD();

// 打印最近n条历史命令
void History(int n);

// 输出重定向
void OutputRedirections();

// 输入重定向
void InputRedirections();

// 追加输出重定向
void OutputAppendRedirections();

// 管道
void Pipe();

// 后台运行
void Background();

// mytop
void mytop();

void parse(string s)
{
    // 解析命令, 即将命令按照空格分割
    // 将命令加入command数组
    command.push_back(s);

    // 在最后添加上空格这样遍历到最后一个字符串的时候就不会出错
    s += " ";

    // 设置临时字符串
    string temp = "";
    for (char ch : s)
    {
        // 遇到空格,说明前面是一个字符串
        if (ch == ' ')
        {
            // 将命令加入argv数组
            argv.push_back(temp);

            // 有输出重定向符号
            if (temp == ">")
                outputredirect = 1;

            //有输入重定向符号
            if (temp == "<")
                inputredirect = 1;

            //有追加输出重定向符号
            if (temp == ">>")
                outputappendredirect = 1;

            // 管道符号
            if (temp == "|")
                haspipe = 1;

            // 后台运行符号
            if (temp == "&")
                background = 1;

            // 清除临时字符串
            temp.clear();
        }
        else
            // 将该字符添加到临时字符串中
            temp += ch;
    }

    // 初始化argv1数组
    for (int i = 0; i < 100; i++)
        argv1[i] = NULL;

    // 将string类型的命令转换为char*类型并传入argv1中,同时更新参数数量argc
    for (string str : argv)
        argv1[argc++] = const_cast<char *>(str.c_str());
}

int CD()
{
    // 参数数量错误
    if (argc != 2)
        return 0;

    // chdir函数传入const char*类型变量,使用c_str()函数将string类型转换为该类型
    const char *path = argv[1].c_str();

    // chdir函数成功返回0,失败返回-1
    return chdir(path) ? 0 : 1;
}

void mytop()
{
    /*
        第一部分:
        获取总体内存大小,空闲内存大小,缓存大小
        使用方法:c中文件流的操作方法(open,read)
    */
    // 存放文件内容的数组
    char buff[255];

    // 只读方式打开meminfo文件
    int fd = open("/proc/meminfo", O_RDONLY);

    // 打开文件失败
    if (fd < 0)
    {
        cout << "Fail to open a file!";
        return;
    }

    // 将文件内容复制到buff中
    int res = read(fd, buff, 255);

    // 读文件失败
    if (res < 0)
    {
        cout << "Fail to read a file!";
        return;
    }

    /*
        strtok(char*s,char*delim):将字符串s按照分隔符delim分割,
        在第一次调用时,strtok()必需给予参数s字符串,往后的调用则将参数s设置成NULL.
        每次调用成功则返回指向被分割出片段的指针(char*类型).
    */

    char *token;

    // 页面大小
    token = strtok(buff, " ");
    int pagesize = atoi(token);

    // 总页数量
    token = strtok(NULL, " ");
    int total = atoi(token);

    // 空闲页数量
    token = strtok(NULL, " ");
    int free = atoi(token);

    // 最大页数量
    token = strtok(NULL, " ");
    int largest = atoi(token);

    // 缓存页数量
    token = strtok(NULL, " ");
    int cached = atoi(token);

    int totalMemory = 0, freeMemory = 0, largestPageMemory = 0, cacheMemory = 0;

    // 计算总体内存大小
    totalMemory = pagesize * total / 1024;

    // 计算空闲内存大小
    freeMemory = pagesize * free / 1024;

    // 计算最大页内存大小
    largestPageMemory = pagesize * largest / 1024;

    // 计算缓存内存大小
    cacheMemory = pagesize * cached / 1024;

    // 打印信息
    cout << "main memory: " << totalMemory << "K total, "
         << freeMemory << "K free, " << cacheMemory << "K cached";

    /*
        第二部分:
        总体CPU使用占比,进程任务数量
    */
    // 只读方式打开kinfo文件
    char buff1[255];
    int fd1 = open("/proc/kinfo", O_RDONLY);

    // 打开文件失败
    if (fd1 < 0)
    {
        cout << "Fail to open a file!";
        return;
    }

    // 将文件内容复制到buff中
    int res1 = read(fd, buff1, 255);

    // 读文件失败
    if (res1 < 0)
    {
        cout << "Fail to read a file!";
        return;
    }
    FILE *fp = NULL;
    fp = fopen("/proc/kinfo", "r"); // 以只读方式打开meminfo文件
    memset(buff, 0x00, 255);        // 格式化buff字符串
    fgets(buff, 255, (FILE *)fp);   // 读取meminfo文件内容进buff
    fclose(fp);

    // 获取进程数量
    int processNumber = 0;
    int i = 0;
    while (buff[i] != ' ')
    {
        processNumber = 10 * processNumber + buff[i] - 48;
        i++;
    }
    printf("processNumber = %d\n", processNumber);

    // 获取任务数量
    i++;
    int tasksNumber = 0;
    while (buff[i] >= '0' && buff[i] <= '9')
    {
        tasksNumber = 10 * tasksNumber + buff[i] - 48;
        i++;
    }
    printf("tasksNumber = %d\n", tasksNumber);

    // /* 3. 获取psinfo中的内容 */
    DIR *d;
    struct dirent *dir;
    d = opendir("/proc");
    int totalTicks = 0, freeTicks = 0;
    if (d)
    {
        // 遍历proc文件夹
        while ((dir = readdir(d)) != NULL)
        { 
            if (strcmp(dir->d_name, ".") != 0 &&
                strcmp(dir->d_name, "..") != 0)
            {
                char path[255];
                memset(path, 0x00, 255);
                strcpy(path, "/proc/");
                strcat(path, dir->d_name); // 连接成为完成路径名
                struct stat s;
                if (stat(path, &s) == 0)
                {
                    if (S_ISDIR(s.st_mode))
                    { // 判断为目录
                        strcat(path, "/psinfo");

                        FILE *fp = fopen(path, "r");
                        char buf[255];
                        memset(buf, 0x00, 255);
                        fgets(buf, 255, (FILE *)fp);
                        fclose(fp);

                        // 获取ticks和进程状态
                        int j = 0;
                        for (i = 0; i < 4;)
                        {
                            for (j = 0; j < 255; j++)
                            {
                                if (i >= 4)
                                    break;
                                if (buf[j] == ' ')
                                    i++;
                            }
                        }
                        // 循环结束, buf[j]为进程的状态, 共有S, W, R三种状态.
                        int k = j + 1;
                        for (i = 0; i < 3;)
                        { // 循环结束后k指向ticks位置
                            for (k = j + 1; k < 255; k++)
                            {
                                if (i >= 3)
                                    break;
                                if (buf[k] == ' ')
                                    i++;
                            }
                        }
                        int processTick = 0;
                        while (buf[k] != ' ')
                        {
                            processTick = 10 * processTick + buff[k] - 48;
                            k++;
                        }
                        totalTicks += processTick;
                        if (buf[j] != 'R')
                        {
                            freeTicks += processTick;
                        }
                    }
                    else
                        continue;
                }
                else
                    continue;
            }
        }
    }
    printf("CPU states: %.2lf%% used,\t%.2lf%% idle",
           (double)((totalTicks - freeTicks) * 100) / (double)totalTicks,
           (double)(freeTicks * 100) / (double)totalTicks);
    return;
}

void History(int n)
{
    // 打印包括history命令的最近n条命令
    int num = command.size();

    // 最近命令数小于n
    if (n > num)
        cout << "history command number error!";
    else
        for (int i = num - n; i < num; i++)
            cout << command[i] << endl;
}

void OutputRedirections()
{
    // 进程号
    pid_t pid;

    // 输出的文件
    const char *outputfile;

    // 找到重定向输出符号的下标index
    int index = 0;
    for (int i = 0; i < argv.size(); i++)
        if (argv[i] == ">")
            index = i;

    // 如果">"符号后面没有参数了,说明缺少输出文件
    if (index == argv.size() - 1)
        cout << "Outputfile missing!";

    // 输出文件,并转换成const char*类型
    string output = argv[index + 1];
    outputfile = output.c_str();

    // 初始化输入文件命令数组
    char *argv2[100];
    for (int i = 0; i < 100; i++)
        argv2[i] = NULL;

    // 构造输入文件命令数组,把string类型转换为char*数组类型
    int argc1 = 0;
    for (int i = 0; i < index; i++)
        argv2[argc1++] = const_cast<char *>(argv[i].c_str());

    // fork子进程
    switch (pid = fork())
    {
    case -1:
    {
        cout << "Fail to create a subprocess";
        return;
    }
    case 0:
    {
        // 输出重定向的思路:将输出文件的文件描述符转移到标准输出STDOUT_FILENO
        // open函数打开输出文件(以只写方式,如果没有就创建新文件,所有者拥有写权限),返回值为文件描述符
        int fd = open(outputfile, O_WRONLY | O_CREAT, S_IWUSR);

        // 文件无法打开,异常退出
        if (fd < 0)
            exit(1);

        // 将输出文件描述符复制(重定向)到标准输出上
        dup2(fd, STDOUT_FILENO);

        // 执行命令
        execvp(argv2[0], argv2);

        // 恢复标准输出
        if (fd != STDOUT_FILENO)
            close(fd);

        // 如果execvp没有执行成功才回到这一步,意味着发生错误
        cout << "command error!";
        exit(1);
    }
    default:
    {
        int status;
        waitpid(pid, &status, 0);      // 等待子进程返回
        int err = WEXITSTATUS(status); // 读取子进程的返回码
        if (err)
            cout << strerror(err);
    }
    }
}

void OutputAppendRedirections()
{
    // 进程号
    pid_t pid;

    // 输出的文件
    const char *outputfile;

    // 找到重定向追加输出符号的下标index
    int index = 0;
    for (int i = 0; i < argv.size(); i++)
        if (argv[i] == ">>")
            index = i;

    // 如果">>"符号后面没有参数了,说明缺少输出文件
    if (index == argv.size() - 1)
        cout << "Outputfile missing!";

    // 输出文件,并转换成const char*类型
    string output = argv[index + 1];
    outputfile = output.c_str();

    // 初始化输入文件命令数组
    char *argv2[100];
    for (int i = 0; i < 100; i++)
        argv2[i] = NULL;

    // 构造输入文件命令数组,把string类型转换为char*数组类型
    int argc1 = 0;
    for (int i = 0; i < index; i++)
        argv2[argc1++] = const_cast<char *>(argv[i].c_str());

    // fork子进程
    switch (pid = fork())
    {
    case -1:
    {
        cout << "Fail to create a subprocess";
        return;
    }
    case 0:
    {
        // 与输出重定向的区别在于open函数使用追加方法:O_APPEND
        int fd = open(outputfile, O_WRONLY | O_APPEND | O_CREAT, S_IWUSR);

        // 文件无法打开,异常退出
        if (fd < 0)
            exit(1);
        dup2(fd, STDOUT_FILENO);

        // 执行命令
        execvp(argv2[0], argv2);

        // 恢复标准输出
        if (fd != STDOUT_FILENO)
            close(fd);

        // 如果execvp没有执行成功才回到这一步,意味着发生错误
        cout << "command error!";
        exit(1);
    }
    default:
    {
        int status;
        waitpid(pid, &status, 0);      // 等待子进程返回
        int err = WEXITSTATUS(status); // 读取子进程的返回码
        if (err)
            cout << strerror(err);
    }
    }
}

void InputRedirections()
{
    pid_t pid;

    // 输入文件
    const char *inputfile;

    // 找到重定向输入符号下标index
    int index = 0;
    for (int i = 0; i < argv.size(); i++)
        if (argv[i] == "<")
            index = i;

    // 缺少输入重定向的文件
    if (index == argv.size() - 1)
        cout << "Inputfile missing!";

    // 输入重定向文件
    string input = argv[index + 1];
    inputfile = input.c_str();

    // 初始化输入重定向文件命令数组
    char *argv2[100];
    for (int i = 0; i < 100; i++)
        argv2[i] = NULL;

    // 构造输入重定向文件命令数组,把string类型转换为char*数组类型
    int argc1 = 0;
    for (int i = 0; i < index; i++)
        argv2[argc1++] = const_cast<char *>(argv[i].c_str());

    // fork子进程
    switch (pid = fork())
    {
    case -1:
    {
        cout << "Fail to create a subprocess";
        return;
    }
    case 0:
    {
        // 输出重定向的思路:将输入重定向文件的文件描述符转移到标准输入STDIN_FILENO
        // open函数打开输入重定向文件(以只写方式,如果没有就创建新文件,所有者拥有写权限),返回值为文件描述符
        int fd = open(inputfile, O_RDONLY, S_IWUSR);

        // 文件无法打开,异常退出
        if (fd < 0)
            exit(1);
        dup2(fd, STDIN_FILENO);
        execvp(argv2[0], argv2);
        if (fd != STDIN_FILENO)
            close(fd);
        cout << "command error!";
        exit(1);
    }
    default:
    {
        int status;
        waitpid(pid, &status, 0);
        int err = WEXITSTATUS(status);
        if (err)
            cout << strerror(err);
    }
    }
}

void Pipe()
{
    /*
        管道操作思路:
        管道的实现其实相当于两个重定向的结合:即先输出重定向再输入重定向,但是这两个
        重定向不能写在一个进程中,否则会发生冲突,需要多线程处理,即:子进程先写管道,父进程后读管道;

        具体方法:
        子进程写管道的时候关闭读端,参考输出重定向的思路将子进程输出重定向到标准输出;
        父进程读管道的时候关闭写端,参考输入重定向的思路将父进程输入重定向到标准输入,从而实现管道.
    */

    // 进程号
    pid_t pid;

    // 找到管道符号的下标index
    int index = 0;
    for (int i = 0; i < argv.size(); i++)
        if (argv[i] == "|")
            index = i;

    // output数组存放输出命令,input数组存放输入命令
    char *output[100];
    char *input[100];

    // 初始化输入输出数组
    for (int i = 0; i < 100; i++)
    {
        output[i] = NULL;
        input[i] = NULL;
    }

    // 构造输出文件
    for (int i = 0; i < index; i++)
        output[i] = const_cast<char *>(argv[i].c_str());

    // 构造输入文件
    for (int i = 0; i < argv.size() - index; i++)
        input[i] = const_cast<char *>(argv[i + index + 1].c_str());

    // pipe函数参数数组filedes[2]:文件描述符数组,fd[0]:读管道,fd[1]:写管道
    int fd[2];

    // pipe函数成功返回0,否则返回-1
    int p = pipe(fd);
    if (p < 0)
    {
        cout << "command error!";
        exit(1);
    }

    switch (pid = fork())
    {
    case -1:
    {
        cout << "Fail to create a subprocess";
        return;
    }
    // 子进程写管道
    case 0:
    {
        // 关闭子进程的读端
        close(fd[0]);

        // 将子进程输出重定向到标准输出
        dup2(fd[1], STDOUT_FILENO);

        // 执行写管道命令
        execvp(output[0], output);

        // 恢复标准输出
        if (fd[1] != STDOUT_FILENO)
            close(fd[1]);
    }
    // 父进程读管道
    default:
    {
        // 父进程需要先等待子进程进行完毕后才开始
        int status;
        waitpid(pid, &status, 0);      // 等待子进程返回
        int err = WEXITSTATUS(status); // 读取子进程的返回码
        if (err)
            cout << strerror(err);

        // 关闭父进程的写端
        close(fd[1]);

        // 将父进程输入重定向到标准输入
        dup2(fd[0], STDIN_FILENO);

        // 执行读管道命令
        execvp(input[0], input);

        // 恢复标准输入
        if (fd[0] != STDIN_FILENO)
            close(fd[0]);
    }
    }
}

void Background()
{
    // 进程号
    pid_t pid;

    // 后台运行符号必然在命令最后
    if (argv.back() != "&" || argv[0] == "&")
    {
        cout << "command error!";
        return;
    }

    // 初始化输入文件命令数组
    char *argv2[100];
    for (int i = 0; i < 100; i++)
        argv2[i] = NULL;

    // 构造输入文件命令数组,把string类型转换为char*数组类型
    for (int i = 0; i < argv.size() - 1; i++)
        argv2[i] = const_cast<char *>(argv[i].c_str());

    // fork子进程
    switch (pid = fork())
    {
    case -1:
    {
        cout << "Fail to create a subprocess";
        return;
    }
    case 0:
    {
        // 重定向黑洞文件到标准输入输出
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "r", stdin);

        // 忽略SIGCHLD信号
        signal(SIGCHLD, SIG_IGN);

        // 执行命令
        execvp(argv2[0], argv2);
        
        // 如果命令执行失败异常退出
        exit(1);
    }
    default:
    {
        // 父进程不需要等待子进程而是直接
        exit(0);
    }
    }
}

void do_cmd()
{
    pid_t pid;

    // 输出重定向
    if (outputredirect)
    {
        OutputRedirections();
        outputredirect = 0;
        return;
    }

    // 输入重定向
    if (inputredirect)
    {
        InputRedirections();
        inputredirect = 0;
        return;
    }

    // 追加输出重定向
    if (outputappendredirect)
    {
        OutputAppendRedirections();
        outputappendredirect = 0;
        return;
    }

    // 管道
    if (haspipe)
    {
        Pipe();
        haspipe = 0;
        return;
    }

    if(background)
    {
        Background();
        background = 0;
        return;
    }

    // 改变盘符命令cd
    if (argv[0] == "cd")
    {
        int res = CD();
        if (!res)
            cout << "command error!";
        return;
    }

    // mytop命令
    if (argv[0] == "mytop")
    {
        mytop();
        return;
    }

    // 打印历史命令记录
    else if (argv[0] == "history")
        History(atoi(argv[1].c_str()));

    // 正常退出
    else if (argv[0] == "exit")
        exit(0);

    //下面都是非内置命令,即program程序,需要创建子进程进行多线程操作
    else
    {
        // 创建子进程
        switch (pid = fork())
        {
        case -1:
        {
            cout << "Fail to create a subprocess";
            return;
        }
        case 0:
        {
            // 将string类型文件名(比如ls)转换成const char*类型方便execvp调用
            const char *filename = argv[0].c_str();

            // 执行argv1命令
            execvp(filename, argv1);

            // 出现错误
            cout << "command error!";
            exit(1);
        }
        default:
        {
            int status;
            waitpid(pid, &status, 0);      // 等待子进程返回
            int err = WEXITSTATUS(status); // 读取子进程的返回码
            if (err)
                printf("Error: %s\n", strerror(err));
        }
        }
    }
}

int main()
{
    while (1)
    {
        cout << "$";
        getline(cin, input);
        parse(input);
        do_cmd();
        argc = 0;
        argv.clear();
    }
}