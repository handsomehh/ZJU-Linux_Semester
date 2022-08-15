/***
 * @Author: 韩恺荣
 * @Date: 2022-08-13 10:06:18
 * @LastEditTime: 2022-08-13 11:00:40
 * @LastEditors: 韩恺荣
 * @Description:mian.cpp，实现myshell的主体架构
 */

#include "Myshell.h"
#include "Command.cpp"

//定义debug模式，当置位1时启动并帮助作者debug
#define _DEBUG_ 0

//输入函数的解析器
void Prase(string &tmp, Command *ret_com)
{
    //首先对输入的参数中含有的;进行分割，将命令分割为命令组
    char delim[5];
    strcpy(delim, ";");
    vector<char *> container;
    //调用strtok函数分割
    char *tmp_ch = strtok(const_cast<char *>(tmp.c_str()), delim);
    while (tmp_ch != nullptr)
    {
        container.push_back(tmp_ch);
        tmp_ch = strtok(NULL, delim);
    }
    //再使用空格/制表符和回车对命令分割
    strcpy(delim, " \n\t");
    vector<vector<string>> cmd_delim;
    for (auto i : container)
    {
        vector<string> tmp_str;
        tmp_ch = strtok(i, delim);
        //一直分割直到字符串结束
        while (tmp_ch != nullptr)
        {
            tmp_str.push_back(tmp_ch);
            tmp_ch = strtok(NULL, delim);
        }
        //将分割好的字符串，进入容器
        if (!tmp_str.empty())
        {
            cmd_delim.push_back(tmp_str);
        }
    }
    //准备将要返回的命令容器
    vector<vector<item>> ret;
    //遍历每一个字符串
    for (auto i : cmd_delim)
    {
        //每一个命令组对应一个vector
        vector<item> tmp_item;
        item *tmp_ele = new item();
        int conunt = 0;
        int inpath_flag = 0;
        int outpath_flag = 0;
        //遍历每一个string
        for (auto j : i)
        {
            if (inpath_flag)
            { //当要获取输入的路径时
                inpath_flag = 0;
                tmp_ele->InPath = j;
            }
            else if (outpath_flag)
            { //当要获取输出的路径时
                outpath_flag = 0;
                tmp_ele->OutPath = j;
            }
            else
            {
                if (j == "<")
                { //当要重新向输入时
                    tmp_ele->reIn = true;
                    inpath_flag = 1;
                }
                else if (j == ">")
                { //当重定向输出时
                    tmp_ele->reOut = true;
                    outpath_flag = 1;
                }
                else if (j == ">>")
                { //当重定向输出时
                    tmp_ele->reApp = true;
                    outpath_flag = 1;
                }
                else if (j == "|")
                { //当有管道操作时
                    tmp_ele->pipOut = true;
                    tmp_item.push_back(*tmp_ele);
                    tmp_ele = new item(); //新建下一个命令容器
                    // tmp_ele->content.clear();
                    tmp_ele->pipIn = true;
                }
                else if (j == "&")
                { //当有后台运行标志时
                    tmp_ele->IsBG = true;
                }
                else
                {
                    tmp_ele->content.push_back(j);
                }
            }
        }
        //将切割的命令装入容器
        tmp_item.push_back(*tmp_ele);
        if (!tmp_item.empty())
        {
            ret.push_back(tmp_item);
        }
    }
    //用容器初始化命令
    ret_com->Create(ret);
}

//查看，命令是否来自于我们自己实现的命令
bool is_in_cmd(string cmd_name)
{
    return cmd_name == "umask" ||
           cmd_name == "cd" ||
           cmd_name == "exec" ||
           cmd_name == "jobs" ||
           cmd_name == "fg" ||
           cmd_name == "clr" ||
           cmd_name == "bg" ||
           cmd_name == "exit" ||
           cmd_name == "set" ||
           cmd_name == "dir" ||
           cmd_name == "pwd" ||
           cmd_name == "test" ||
           cmd_name == "echo" ||
           cmd_name == "time";
}
//运行我们自己实现的命令
bool run_in(item &tmp)
{
    //如果不在列表中，我们返回false
    if (!is_in_cmd(tmp.content[0]))
    {
        return false;
    }
    //否则找打对应的函数并执行函数
    if (tmp.content[0] == "bg")
        BG(tmp);
    else if (tmp.content[0] == "fg")
        FG(tmp);
    else if (tmp.content[0] == "jobs")
        JOBS(tmp);
    else if (tmp.content[0] == "cd")
        CD(tmp);
    else if (tmp.content[0] == "pwd")
        PWD();
    else if (tmp.content[0] == "clr")
        CLR();
    else if (tmp.content[0] == "time")
        TIME();
    else if (tmp.content[0] == "umask")
        UMASK(tmp);
    else if (tmp.content[0] == "dir")
        dir(tmp);
    else if (tmp.content[0] == "set")
        SET();
    else if (tmp.content[0] == "echo")
        ECHO(tmp);
    else if (tmp.content[0] == "exec")
        EXEC(tmp);
    else if (tmp.content[0] == "test")
        TEST(tmp);
    else if (tmp.content[0] == "exit")
        EFUN(tmp);
    return true;
}

/***
 * @description: 命令执行器
 * @param {Command} *cur_cmd 输入将要执行的命令
 * @return {*}
 */
void ExecuteCommand(Command *cur_cmd)
{
    for (auto i : cur_cmd->cmd_container)
    {
        //获取每一个命令组
        if (i.size() == 1)
        {
            //如果只有一个命令，证明和管道运算符无关，直接执行即可
            if (is_in_cmd(i[0].content[0]))
            {
                run_in(i[0]);
            }
            else
            {
                //当输入的是help命令时，特殊处理，我们用more的方式预览我们事先提供好滴手册
                if (i[0].content[0] == "help")
                {
                    i[0].content.clear();
                    i[0].content.push_back("more");
                    //对命令修改和切割，得到我们想要的命令
                    string path_tmp = getcwd(nullptr, 0);
                    path_tmp += "/man.txt";
                    i[0].content.push_back(path_tmp);
                }
                //新建进程
                pid_t pid = fork();
                //进程创建失败时
                if (pid < 0)
                {
                    fprintf(stderr, "create process failed\n");
                    exit(1);
                }
                else if (pid == 0)
                {
// debug模式下输出一些有用的信息帮助我们debug
#if _DEBUG_ == 1
                    cout << getpid() << endl;
                    cout << Pro_manager->FindJobs(getpid()) << endl;
                    char *arg[i[0].content.size()];
                    int cnt = 0;
                    for (auto j : i[0].content)
                    {
                        arg[cnt++] = (char *)j.c_str();
                    }
                    arg[cnt] = nullptr;
                    cout << 2 << endl;
                    if (!execvp(i[0].content[0].c_str(), arg))
                        fprintf(stderr, "MYSHELL:NOT FOUND COMMAND");
#endif
                    //等待父进程对进程列表操作完成后，再执行我们的操作
                    usleep(1);
                    int cnt = 0;
                    while (true)
                    {
                        if (Pro_manager->FindJobs(getpid()) == -1)
                        { //没有找打，证明还未完成进程列表的添加
                            usleep(1);
                        }
                        else
                        {
                            //找到之后，并且发现找到的进程状态被修改成为true，开始执行命令
                            if (Pro_manager->jobs[Pro_manager->FindJobs(getpid())].run)
                                break;
                        }
                        usleep(1);
                    }
                    //当有重定向时
                    if (i[0].reIn)
                    {
                        //关闭标准输入
                        close(0);
                        int Fd = open(i[0].InPath.c_str(), O_RDONLY);
                    }
                    //重定向输出时
                    if (i[0].reOut)
                    {
                        //关闭标准输出
                        close(1);
                        int Fd = open(i[0].OutPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
                    }
                    //重定向追加
                    if (i[0].reApp)
                    {
                        //关闭标准输出
                        close(1);
                        int Fd = open(i[0].OutPath.c_str(), O_RDWR | O_CREAT | O_APPEND, 0666);
                    }
                    //执行命令
                    if (!is_in_cmd(i[0].content[0]))
                    {
                        //要执行的命令不在我们的列表中，调用execvp方法
                        // cout<<"size:"<<i[0].content.size()<<endl;
                        char *arg[i[0].content.size()];
                        int cnt = 0;
                        //对命令进行切割，以满足execvp方法对命令的要求
                        for (auto j : i[0].content)
                        {
                            arg[cnt++] = new char[strlen(const_cast<char *>(j.c_str()))];
                            strcpy(arg[cnt - 1], const_cast<char *>(j.c_str()));
#if _DEBUG_ == 1
                            cout << cnt - 1 << " " << j << " " << arg[cnt - 1] << endl;
#endif
                        }
                        arg[cnt] = nullptr;
#if _DEBUG_ == 1
                        cout << i[0].content[0].c_str() << endl;
                        for (int k = 0; k < cnt; k++)
                        {
                            cout << k << " " << arg[k] << endl;
                        }
#endif
                        //执行失败，证明命令不存在
                        if (!execvp(i[0].content[0].c_str(), arg))
                            fprintf(stderr, "MYSHELL:NOT FOUND COMMAND");
                    }
                    else
                    {
                        //否则，简单调用我们的命令执行函数
                        run_in(i[0]);
                    }
                    //恢复我们之前的输入和输出到标准流
                    close(0);
                    close(1);
                }
                else
                {
                    //父进程负责管理进程表的添加
                    int index;
                    if (!i[0].IsBG)
                    {
                        //当是后台进程时
                        index = Pro_manager->Addjob(pid, i[0].content[0], true, false, false);
                    }
                    else
                    {
                        //当是前台进程时
                        index = Pro_manager->Addjob(pid, i[0].content[0], false, false, false);
                    }
                    //获取当前所在的文件夹
                    string rootpath = getcwd(NULL, 0);
                    //添加环境变量
                    setenv("PARENT", rootpath.c_str(), 1);
                    Pro_manager->jobs[index].run = true;
                    //如果不是后台命令
                    if (!i[0].IsBG)
                    {
                        //使用阻塞的方式等待命令的结束
                        waitpid(pid, NULL, WUNTRACED);
                        //执行完成后，修改我们的进程表
                        if (!Pro_manager->jobs[index].run && Pro_manager->jobs[index].fg)
                            Pro_manager->Delete(pid);
                    }
                    else
                    {
                        //后台命令使用非阻塞的方式执行
                        waitpid(pid, NULL, WNOHANG);
                    }
                }
            }
        }
        else if (i.size() > 1)
        {
            //当要执行的命令个数超过一个时，证明有管道
            int cnt_outer = 0;
            string pipe_txt = "MYSHELL_pipe.txt";
            //指定管道命令的中间文件
            for (auto cur_cmd : i)
            {
                //对于每一个命令，更改输入和输出流
                int fd_o, fd_i, store_o, store_i;
                //当是中间的命令时，输入输出都要改变
                if (cnt_outer != 0 && cnt_outer != i.size() - 1)
                {
                    //打开新文件作为输入和输出流
                    fd_i = open(pipe_txt.c_str(), O_RDONLY);
                    fd_o = open("MYSHELL_pipeo.txt", O_WRONLY | O_CREAT | O_TRUNC, 0777);
                    //保存原来的输入和输出
                    store_i = dup(0);
                    store_o = dup(1);
                    //更改输入和输出
                    dup2(fd_o, 1);
                    dup2(fd_i, 0);
                }
                else if (cnt_outer == 0)
                {
                    //对于第一个命令只更改输出流
                    fd_o = open(pipe_txt.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
                    store_o = dup(1);
                    dup2(fd_o, 1);
                }
                else
                {
                    //最后一个命令只更改输入流
                    fd_i = open(pipe_txt.c_str(), O_RDONLY);
                    store_i = dup(0);
                    dup2(fd_i, 0);
                }
                //当是列表中自定义函数时，直接执行
                if (is_in_cmd(cur_cmd.content[0]))
                {
                    run_in(cur_cmd);
                }
                else
                {
                    //当时help命令
                    if (cur_cmd.content[0] == "help")
                    {
                        cur_cmd.content.clear();
                        cur_cmd.content.push_back("more");
                        //修改help命令的参数为我自创的手册路径
                        string path_tmp = getcwd(nullptr, 0);
                        path_tmp += "/man.txt";
                        cur_cmd.content.push_back(path_tmp);
                    }
                    //新建一个命令
                    pid_t pid = fork();
                    if (pid < 0)
                    {
                        //新建失败
                        fprintf(stderr, "create process failed\n");
                        exit(1);
                    }
                    //子进程负责执行命令，和单条命令类似
                    else if (pid == 0)
                    {
                        // debug模式下的输出一些调试中有用的信息
#if _DEBUG_ == 1
                        cout << getpid() << endl;
                        cout << Pro_manager->FindJobs(getpid()) << endl;
                        char *arg[cur_cmd.content.size()];
                        int cnt = 0;
                        for (auto j : cur_cmd.content)
                        {
                            arg[cnt++] = (char *)j.c_str();
                        }
                        arg[cnt] = nullptr;
                        cout << 2 << endl;
                        if (!execvp(cur_cmd.content[0].c_str(), arg))
                            fprintf(stderr, "MYSHELL:NOT FOUND COMMAND");
#endif
                        usleep(1);
                        //等待父进程执行进程表插入完毕
                        int cnt = 0;
                        while (true)
                        {
                            if (Pro_manager->FindJobs(getpid()) == -1)
                            {
                                //等待插入完毕
                                usleep(1);
                            }
                            else
                            {
                                if (Pro_manager->jobs[Pro_manager->FindJobs(getpid())].run)
                                    break;
                            }
                            usleep(1);
                        }
                        //当重定向输入
                        if (cur_cmd.reIn)
                        {
                            //更改标准输入流
                            close(0);
                            int Fd = open(cur_cmd.InPath.c_str(), O_RDONLY);
                        }
                        //重定向输出时，更改标准输出流
                        if (cur_cmd.reOut)
                        {
                            close(1);
                            int Fd = open(cur_cmd.OutPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
                        }
                        //重定向追加
                        if (cur_cmd.reApp)
                        {
                            close(1);
                            int Fd = open(cur_cmd.OutPath.c_str(), O_RDWR | O_CREAT | O_APPEND, 0666);
                        }
                        //当是自定义命令的执行时
                        if (!is_in_cmd(cur_cmd.content[0]))
                        {
                            // cout<<"size:"<<cur_cmd.content.size()<<endl;
                            //更改格式以便于适配execvp方法的执行
                            char *arg[cur_cmd.content.size()];
                            int cnt = 0;
                            for (auto j : cur_cmd.content)
                            {
                                //将命令重新转化为字符串
                                arg[cnt++] = new char[strlen(const_cast<char *>(j.c_str()))];
                                strcpy(arg[cnt - 1], const_cast<char *>(j.c_str()));
#if _DEBUG_ == 1
                                cout << cnt - 1 << " " << j << " " << arg[cnt - 1] << endl;
#endif
                            }
                            arg[cnt] = nullptr;
#if _DEBUG_ == 1
                            cout << cur_cmd.content[0].c_str() << endl;
                            for (int k = 0; k < cnt; k++)
                            {
                                cout << k << " " << arg[k] << endl;
                            }
#endif
                            if (!execvp(cur_cmd.content[0].c_str(), arg)) //执行失败时，输出报错信息
                                fprintf(stderr, "MYSHELL:NOT FOUND COMMAND");
                        }
                        else
                        {
                            //当时自定义命令，直接执行就好
                            run_in(cur_cmd);
                        }
                    }
                    else
                    {
                        //父进程负责进程表的添加和控制
                        int index;
                        if (!cur_cmd.IsBG)
                        {
                            //当是后台进程是
                            index = Pro_manager->Addjob(pid, cur_cmd.content[0], true, false, false);
                        }
                        else
                        {
                            //当不是后台进程
                            index = Pro_manager->Addjob(pid, cur_cmd.content[0], false, false, false);
                        }
                        //添加父进程路径到环境变量
                        string rootpath = getcwd(NULL, 0);
                        setenv("PARENT", rootpath.c_str(), 1);
                        Pro_manager->jobs[index].run = true;
                        //当不是后台命令
                        if (!cur_cmd.IsBG)
                        {
                            //阻塞等待
                            waitpid(pid, NULL, WUNTRACED);
                            //删除进程表中的数据
                            if (!Pro_manager->jobs[index].run && Pro_manager->jobs[index].fg)
                                Pro_manager->Delete(pid);
                        }
                        else
                        {
                            //后台命令我们非阻塞
                            waitpid(pid, NULL, WNOHANG);
                        }
                    }
                }
                //恢复标准输出和输入流
                if (cnt_outer != 0 && cnt_outer != i.size() - 1)
                {
                    //用保存好的输入和输出再次更改我们的输入和输出流
                    dup2(store_i, 0);
                    dup2(store_o, 1);
                    close(fd_i);
                    close(fd_o);
                    fd_i = open(pipe_txt.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0777);
                    fd_o = open("MYSHELL_pipeo.txt", O_RDONLY);
                    char c;
                    while (read(fd_o, &c, 1) == 1)
                        write(fd_i, &c, 1);
                    close(fd_i);
                    close(fd_o);
                    remove("MYSHELL_pipeo.txt");
                    cnt_outer++;
                }
                else if (cnt_outer == 0)
                {
                    //同理，恢复输出流
                    dup2(store_o, 1);
                    close(fd_o);
                    cnt_outer++;
                }
                else
                {
                    //同理，恢复输入流
                    dup2(store_i, 0);
                    close(fd_i);
                    remove("MYSHELL_pipe.txt");
                    cnt_outer++;
                }
            }
        }
    }
    return;
}
/***
 * @description: mian函数，负责执行我们的命令
 */
int main(int argc, char **argv)
{
    Init(); //初始化
    //显示当前路径
    string cur_path = getcwd(NULL, 0);
    setenv("SHELL", cur_path.c_str(), 1);
    //当输入多个参数，证明要执行文件
    if (argc != 1)
    {
        //执行文件，有多个的话循环依次执行
        for (int i = 1; i < argc; i++)
        {
            ifstream fin(argv[i]);
            string tmp;
            //每次读取一行
            while (getline(fin, tmp))
            {
                //显示提示信息
                cur_path = getcwd(NULL, 0);
                cout << "ROOT@" << cur_path << "$" << tmp << endl;
                Command *cur_cmd = new Command;
                //解析后执行
                Prase(tmp, cur_cmd);
                ExecuteCommand(cur_cmd);
            }
        }
        return 0;
    }
    //当从控制台输入时
    string tmp;
    cur_path = getcwd(NULL, 0);
    //打印提示信息
    cout << "ROOT@" << cur_path << "$";
    getline(cin, tmp);
    while (true)
    {
        //循环读取
        Command *cur_cmd = new Command;
        //解析后执行命令
        Prase(tmp, cur_cmd);
        ExecuteCommand(cur_cmd);
        cur_path = getcwd(NULL, 0);
        cout << "ROOT@" << cur_path << "$";
        getline(cin, tmp);
    }
}
/***
 * @description: 处理CHLD信号
 * @param {int} sign 信号的辨识标志
 * @param {siginfo_t} *catcher 捕获信号额外的信息
 * @param {void} *pointer
 * @return {*}
 */
void Handler_S(int sign, siginfo_t *catcher, void *pointer)
{
#if _DEBUG_ == 1
    cout << catcher->si_pid << " over" << endl;
#endif
    //使用flag以避免错误的删除
    if (BG_FLAG)
    {
        BG_FLAG = 0;
        return;
    }
    else if (Z_FLAG)
    {
        Z_FLAG = 0;
        return;
    }
    //确认捕获的信号为CHLD
    if (sign == SIGCHLD)
    {
        //找不到进程表中的对应命令
        int index = Pro_manager->FindJobs(catcher->si_pid);
        if (index == -1)
        {
            cout << "fatal error!" << endl;
            exit(1);
        }
        else
        {
            //找到后，当是前台命令，证明这个命令执行完毕
            if (Pro_manager->jobs[index].fg)
            {
                //打印信息
                // cout << "[" << Pro_manager->jobs[index].pid << "]" << Pro_manager->jobs[index].name << " 已完成" << endl;
                //修改进程表
                Pro_manager->jobs[index].run = false;
                Pro_manager->jobs[index].over = true;
            }
            else if (!Pro_manager->jobs[index].fg && Pro_manager->jobs[index].run)
            {
                //当是后台命令已完成
                cout << "[" << Pro_manager->jobs[index].pid << "]" << Pro_manager->jobs[index].name << " 已完成" << endl;
                Pro_manager->Delete(Pro_manager->jobs[index].pid);
                //确保命令完成
                kill(Pro_manager->jobs[index].pid, 9);
                Pro_manager->Delete(Pro_manager->jobs[index].pid);
            }
        }
    }
}
/***
 * @description: 处理挂起命令
 * @param {int} sign 信号标志
 * @return {*}
 */
void Handler_Z(int sign)
{
    //确认是挂起信号
    if (sign == SIGTSTP || sign == SIGSTOP)
    {
#if _DEBUG_ == 1
        Output();
#endif
        // cout<<"1"<<endl<<Pro_manager->jobs[Pro_manager->sum_jobs-1].run<<Pro_manager->jobs[Pro_manager->sum_jobs-1].fg<<endl;
        //保证将要处理的命令是一个正在运行的命令，找到最近的正在运行命令，执行如下操作
        if (Pro_manager->jobs[Pro_manager->sum_jobs - 1].run && Pro_manager->jobs[Pro_manager->sum_jobs - 1].fg && Pro_manager->jobs[Pro_manager->sum_jobs - 1].pid != 0)
        {
            //在对命令挂起前，首先处理run和flag标志
            Z_FLAG = 1;
            Pro_manager->jobs[Pro_manager->sum_jobs - 1].run = false;
            Pro_manager->jobs[Pro_manager->sum_jobs - 1].fg = false;
            //发送挂起信号
            kill(Pro_manager->jobs[Pro_manager->sum_jobs - 1].pid, SIGTSTP);
            Pro_manager->jobs[Pro_manager->sum_jobs - 1].bg_id = (++Pro_manager->sum_bgs);
            Pro_manager->jobs[Pro_manager->sum_jobs - 1].over = false;
            //输出提示信息
            cout << endl;
            cout << "[" << Pro_manager->jobs[Pro_manager->sum_jobs - 1].bg_id << "]" << Pro_manager->jobs[Pro_manager->sum_jobs - 1].pid << " + is already stop" << endl;
        }
    }
}
