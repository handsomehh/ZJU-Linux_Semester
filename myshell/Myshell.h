/***
 * @Author: 韩恺荣
 * @Date: 2022-08-13 10:06:44
 * @LastEditTime: 2022-08-13 11:51:43
 * @LastEditors: 韩恺荣
 * @Description: Myshell.h 头文件引用和数据结构定义
 */

#ifndef _MYSHELL_H_
#define _MYSHELL_H_
/*-------头文件引用-------*/
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <unistd.h>
#include <stdlib.h>
#include <ctime>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <iomanip>
#include <dirent.h>
#include <sys/shm.h>
#include <wait.h>
#include <fcntl.h>
#include <signal.h>
#include <iomanip>
#include <time.h>

using namespace std;
/*-------宏定义---------*/
#define MAX_JOB_SIZE 100
/*—-----返回值定义-------*/
enum Pr_State
{
    NOT_FOUND = -1,
    SUCCEED
};
/*-----数据结构定义区-------*/

// job类，用于定义不同的工作进程
class job
{
public:
    string name; //进程名称
    int pid;     //进程id
    bool fg;     //是否为前台程序
    bool run;    //是否正在运行
    bool over;   //是否已经结束运行
    int bg_id;   //后台进程号
};

// process类，用于管理进程
class Process
{
public:
    int sum_jobs;           //所有的工作进程数量
    int sum_bgs;            //后台运行的进程数量
    job jobs[MAX_JOB_SIZE]; // jobs数组，定义每一个进程的状态
    Process() : sum_jobs(0), sum_bgs(0)
    { //缺省初始化函数
    }
    Process(int sum_cnt_, int bg_cnt_) : sum_jobs(sum_cnt_), sum_bgs(bg_cnt_) //含参的初始化函数
    {
    }
    void Init()
    { //重新初始化赋值
        sum_jobs = 0;
        sum_bgs = 0;
    }
    int Addjob(int pid_, string name_, bool fg_, bool run_, bool over_); //添加一个进程
    Pr_State Delete(int pid_);                                           //删除一个进程
    Pr_State Update();                                                   //更新进程表，并删除所有已经结束的进程
    int FindJobs(int pid_);                                              //查找进程id，失败时返回-1
    int Findbgs(int pid_);                                               //查找后台id，失败时返回-1
};
//更新函数
Pr_State Process::Update()
{
    for (int i = 0; i < sum_jobs - 1; i++)
    {
        //遍历进程表，对已经结束 的进程执行删除
        if (jobs[i].over)
        {
            Delete(jobs[i].pid);
        }
    }
    return SUCCEED;
}
//查找进程id
int Process::FindJobs(int pid_)
{
    for (int i = 0; i < sum_jobs; i++)
    {
        //遍历进程表，找到pid一致的进程时，返回下标，否则返回-1
        if (pid_ == jobs[i].pid)
            return i;
    }
    return -1;
}
//查找后台id
int Process::Findbgs(int pid_)
{
    for (int i = 0; i < sum_jobs; i++)
    {
        //遍历进程表，找pid一致的后台进程时，返回下标，否则返回-1
        if (pid_ == jobs[i].bg_id)
            return i;
    }
    return -1;
}
// item类。用于定义一个执行命令的容器
class item
{
public:
    vector<string> content; //命令及参数
    bool IsBG;              //命令是否为后台运行
    bool pipIn;             //命令是否有管道输入
    bool pipOut;            //命令是否有管道输出
    bool reIn;              //重定向输入标记
    bool reOut;             //重定向输出(覆盖)
    bool reApp;             //重定向输出(覆盖)
    string InPath;          //重定向输入路径
    string OutPath;         //重定向输出路径
    item()                  //初始化函数
    {
        vector<string> tmp;
        IsBG = false;
        pipIn = false;
        pipOut = false;
        reIn = false;
        reOut = false;
        reApp = false;
        InPath = "";
        OutPath = "";
        content = tmp;
    }
    ~item() {} //析构函数
};

//全局变量，Pro_manager用于管理所有的进程
Process *Pro_manager;
//命令的类
class Command
{
public:
    vector<vector<item>> cmd_container;                        //命令及参数
    Command() {}                                               //构造函数
    Command(vector<vector<item>> &cmd) : cmd_container(cmd) {} //带参数的构造
    void Create(vector<vector<item>> &cmd)
    { //构造后的赋值
        cmd_container = cmd;
    }
    ~Command()
    { //析构函数
        for (auto i : cmd_container)
        {
            for (auto j : i)
            {
                j.~item();
            }
        }
    }
};
// zflag全局变量，用于标志是否按下了ctrl+z
int Z_FLAG = 0;
// bgflag,用于标志是否发生了kill(pid,SIGCONT)行为
int BG_FLAG = 0;
//信号类，用于定义新的信号处理以及保存旧的信号处理
struct sigaction *new_sig = new struct sigaction;
struct sigaction old_sig;
//该函数用于输出debug信息，打印当前的命令表
void Output()
{
    cout << Pro_manager->sum_jobs << Pro_manager->sum_bgs << endl;
    cout << "name pid over run bg_id fg" << endl;
    for (int i = 0; i < Pro_manager->sum_jobs; i++)
    {
        //格式控制，以便表格美观
        cout << setw(10) << Pro_manager->jobs[i].name << setw(10) << Pro_manager->jobs[i].pid;
        cout << setw(3) << Pro_manager->jobs[i].over << setw(3) << Pro_manager->jobs[i].run << setw(3) << Pro_manager->jobs[i].bg_id << setw(3) << Pro_manager->jobs[i].fg << endl;
    }
}
//新定义的信号处理函数
void Handler_S(int sign, siginfo_t *catcher, void *pointer);
void Handler_Z(int sign);
//信号处理的函数
void Sig_manager()
{
    //绑定新函数处理CHDL信号
    new_sig->sa_sigaction = Handler_S;
    //处理过后恢复，并且保存处理时的相关信息
    new_sig->sa_flags = SA_RESTART | SA_SIGINFO;
    //置空忽略的信号集
    sigemptyset(&new_sig->sa_mask);
    //信号绑定
    signal(SIGTSTP, Handler_Z);
    signal(SIGSTOP, Handler_Z);
    sigaction(SIGCHLD, new_sig, &old_sig);
}
//全局初始化
void Init()
{
    // IPC资源处理，实现共享内存
    int shmid;
    if ((shmid = shmget(IPC_PRIVATE, sizeof(Process), 0666 | IPC_CREAT)) == -1)
    {
        fprintf(stderr, "共享内存创建失败");
        exit(1);
    }
    //绑定共享内存当当前的进程
    void *shm = NULL;
    if ((shm = shmat(shmid, 0, 0)) == (void *)-1)
    {
        fprintf(stderr, "共享内存连接失败");
        exit(1);
    }
    //用共享内存处理赋值给进程管理器
    Pro_manager = (Process *)shm;
    Pro_manager->Init();
    //默认新添加一个进程，，为MYSHELL
    Pro_manager->Addjob(0, "MYSHELL", true, true, false);
    //信号的初始化
    Sig_manager();
    // Output();
}
//新建一个job
int Process::Addjob(int pid_, string name_, bool fg_, bool run_, bool over_)
{
    //用输入的参数赋值给当前的jobs
    jobs[sum_jobs].pid = pid_;
    jobs[sum_jobs].name = name_;
    jobs[sum_jobs].fg = fg_;
    jobs[sum_jobs].run = run_;
    jobs[sum_jobs].over = over_;
    //如果是后台进程，还要更新后台进程号
    if (!fg_)
    {
        jobs[sum_jobs].bg_id = (++sum_bgs);
    }
    //进程的总数加1
    sum_jobs++;
    return sum_jobs - 1;
}
//删除一个进程
Pr_State Process::Delete(int pid_)
{
    int i;
    for (i = 0; i < sum_jobs; i++)
    {
        //遍历进程表，找到要删除的进程
        if (pid_ == jobs[i].pid)
        {
            break;
        }
    }
    //如果没有找到，返回值为NOT_FOUND
    if (i == sum_jobs)
        return NOT_FOUND;
    bool flag = false;
    //如果要删除的进程是后台进程
    if (!jobs[i].fg)
    {
        flag = true;
        sum_bgs--;
    }
    //之后的每个后台进程，都要更新他们的后台进程号
    for (int j = i; j < sum_jobs - 1; j++)
    {
        jobs[j] = jobs[j + 1];
        //更新后台进程号
        if (flag && !jobs[j].fg)
        {
            jobs[j].bg_id--;
        }
    }
    sum_jobs--;
    //返回删除成功的标志
    return SUCCEED;
}

#endif