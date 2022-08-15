/***
 * @Author: 韩恺荣
 * @Date: 2022-08-13 10:07:16
 * @LastEditTime: 2022-08-13 11:52:47
 * @LastEditors: 韩恺荣
 * @Description: 文件名：Command.cpp负责命令的实现
 */
#include "Myshell.h"
/***
 * @description: 将字符串转正整数，失败时返回-1
 * @param {string} &a
 * @return {*} 返回转换的数
 */
int TurnToInt(string &a)
{
    int sum = 0;
    for (int i = 0; i < a.size(); i++)
    {
        //遍历字符串的每一个元素并转换
        if (a[i] >= '0' && a[i] <= '9')
        {
            sum *= 10;
            sum += (int)(a[i] - '0');
        }
        else
        {
            //失败返回-1
            return -1;
        }
    }
    return sum;
}
/***
 * @description: 将挂起的命令转到后台运行，支持bg %n和不带参两种模式
 * @param {item} &cmd
 * @return {*}
 */
void BG(item &cmd)
{
    if (cmd.content.size() == 1)
    {
        //当只有一个参数时，找到最后的后台挂起命令
        int i;
        for (i = 0; i < Pro_manager->sum_jobs; i++)
        {
            if (!Pro_manager->jobs[i].fg && !Pro_manager->jobs[i].run && !Pro_manager->jobs[i].over)
            {
                BG_FLAG = 1;
                //向进程发生继续信号
                kill(Pro_manager->jobs[i].pid, SIGCONT);
                Pro_manager->jobs[i].fg = false;
                Pro_manager->jobs[i].run = true;
                //打印提示信息
                cout << "[" << Pro_manager->jobs[i].bg_id << "]" << Pro_manager->jobs[i].name << " 转至后台运行" << endl;
                break;
            }
        }
        if (i == Pro_manager->sum_jobs)
        {
            cout << "There is no process to deal" << endl;
            return;
        }
    }
    else
    {
        //当输出多个参数时，确保格式正确
        if (cmd.content[1][0] == '%')
        {
            int id;
            string tmp = cmd.content[1].substr(1);
            if ((id = TurnToInt(tmp)) != -1)
            {
                //转换字符串为后台进程号
                int index = Pro_manager->Findbgs(id);
                if (index != -1)
                {
                    //当找到时
                    BG_FLAG = 1;
                    //发送进程继续的信号
                    kill(Pro_manager->jobs[index].pid, SIGCONT);
                    Pro_manager->jobs[index].fg = false;
                    Pro_manager->jobs[index].run = true;
                    //打印提示信息
                    cout << "[" << Pro_manager->jobs[index].bg_id << "]" << Pro_manager->jobs[index].name << " 转至后台运行" << endl;
                }
                else
                {
                    cout << "MYSHELL:Not Found target" << endl;
                }
            }
            else
            {
                cout << "illegal input" << endl;
            }
        }
        else
        {
            cout << "Format is not allowed" << endl;
        }
    }
}
/***
 * @description: 打印当前后台运行和挂起的进程，支持-l参数选项
 * @param {item} &cmd
 * @return {*}
 */
void JOBS(item &cmd)
{
    int cnt = 0;
    if (cmd.content.size() == 1)
    {
        //只输入一个参数
        for (; cnt < Pro_manager->sum_jobs; cnt++)
        {
            //遍历并打印
            if (!Pro_manager->jobs[cnt].fg && !Pro_manager->jobs[cnt].over)
            {
                //对于后台未over的进程
                cout << setw(5) << "[" << Pro_manager->jobs[cnt].bg_id << "]";
                if (Pro_manager->jobs[cnt].run)
                {
                    //当该进程运行时
                    cout << setw(3) << " " << setw(3) << Pro_manager->jobs[cnt].name << " 正在运行" << endl;
                }
                else
                {
                    //当是挂起的进程时
                    cout << setw(3) << "+ " << setw(3) << Pro_manager->jobs[cnt].name << " 已挂起" << endl;
                }
            }
        }
    }
    else if (cmd.content.size() == 2)
    {
        if (cmd.content[1] == "-l")
        {
            //对于-l参数的选项
            for (; cnt < Pro_manager->sum_jobs; cnt++)
            {
                //遍历进程表
                if (!Pro_manager->jobs[cnt].fg && !Pro_manager->jobs[cnt].over)
                {
                    //打印详细的进程信息
                    cout << setw(5) << "[" << Pro_manager->jobs[cnt].bg_id << "]" << Pro_manager->jobs[cnt].pid;
                    if (Pro_manager->jobs[cnt].run)
                    {
                        //运行的
                        cout << setw(3) << " " << setw(3) << Pro_manager->jobs[cnt].name << " 正在运行" << endl;
                    }
                    else
                    {
                        //挂起的
                        cout << setw(3) << "+ " << setw(3) << Pro_manager->jobs[cnt].name << " 已挂起" << endl;
                    }
                }
            }
        }
        else
        {
            cout << "MYSHELL: JOBS :invalid input" << endl;
        }
    }
    else
    {
        cout << "MYSHELL: JOBS :invalid input" << endl;
    }
}
/***
 * @description: 将后台或挂起的进程转到前台，支持fg %n和不带参两种模式
 * @param {item} &cmd
 * @return {*}
 */
void FG(item &cmd)
{
    int cnt = Pro_manager->sum_jobs - 1;
    if (cmd.content.size() == 1)
    {
        //不含参数时，默认操作对象是最进的后台进程
        for (; cnt > 0; cnt--)
        {
            if (!Pro_manager->jobs[cnt].fg && !Pro_manager->jobs[cnt].over)
            {
                break;
            }
        }
        //没有找到后台进程
        if (cnt == 0)
        {
            cout << "MYSHELL : NOT BACKGROUND PROCESS" << endl;
        }
        else
        {
            //找到之后
            for (int i = cnt + 1; i < Pro_manager->sum_jobs - 1; i++)
            {
                //修改后台id总数
                if (!Pro_manager->jobs[i].fg)
                {
                    Pro_manager->jobs[i].bg_id--;
                }
            }
            //如果正在运行
            if (Pro_manager->jobs[cnt].run)
            {
                //转前台并阻塞等待
                Pro_manager->jobs[cnt].fg = true;
                waitpid(Pro_manager->jobs[cnt].pid, NULL, 0);
            }
            else
            {
                //如果挂起，转前台
                Pro_manager->jobs[cnt].fg = true;
                Pro_manager->jobs[cnt].run = true;
                Pro_manager->jobs[cnt].over = false;
                BG_FLAG = 1;
                //阻塞运行，并发送继续的信号
                kill(Pro_manager->jobs[cnt].pid, SIGCONT);
                waitpid(Pro_manager->jobs[cnt].pid, NULL, 0);
            }
        }
    }
    else if (cmd.content.size() == 2)
    {
        //输入多个参数
        if (cmd.content[1][0] == '%')
        {
            int id;
            string tmp = cmd.content[1].substr(1);
            if ((id = TurnToInt(tmp)) != -1)
            {
                //获取后台id
                int index = Pro_manager->Findbgs(id);
                if (index != -1)
                {
                    //如果找到进程
                    for (int i = index + 1; i < Pro_manager->sum_jobs - 1; i++)
                    {
                        //修改后台id总数
                        if (!Pro_manager->jobs[i].fg)
                        {
                            Pro_manager->jobs[i].bg_id--;
                        }
                    }
                    //后台正在运行的进程
                    if (Pro_manager->jobs[index].run)
                    {
                        //改为前台并阻塞和运行
                        Pro_manager->jobs[index].fg = true;
                        waitpid(Pro_manager->jobs[index].pid, NULL, 0);
                    }
                    else
                    {
                        //挂起的进程
                        Pro_manager->jobs[index].fg = true;
                        Pro_manager->jobs[index].run = true;
                        Pro_manager->jobs[index].over = false;
                        BG_FLAG = 1;
                        //发送继续的信号，并运行
                        kill(Pro_manager->jobs[index].pid, SIGCONT);
                        waitpid(Pro_manager->jobs[index].pid, NULL, 0);
                    }
                }
                else
                {
                    cout << "MYSHELL:Not Found target" << endl;
                }
            }
            else
            {
                cout << "illegal input" << endl;
            }
        }
        else
        {
            cout << "MYSHELL: JOBS :invalid input" << endl;
        }
    }
    else
    {
        cout << "MYSHELL: JOBS :invalid input" << endl;
    }
    //更新进程
    Pro_manager->Update();
}
/***
 * @description: 输出当前文件夹
 * @return {*}
 */
void PWD()
{
    cout << getcwd(NULL, 0) << endl;
}
/***
 * @description: cd命令，不带参简单输出当前文件夹，带参时转移当前文件夹
 * @param {item} &cmd
 * @return {*}
 */
void CD(item &cmd)
{
    //不带参
    if (cmd.content.size() == 1)
        PWD();
    else if (cmd.content.size() == 2)
    {
        //带参时
        if (chdir(cmd.content[1].c_str()))
            fprintf(stderr, "Error:没有%s此目录!\n", cmd.content[1].c_str()); //改变目录失败，没有此目录
        else
        {
            setenv("PWD", cmd.content[1].c_str(), 1); // CD指令要改变PWD环境变量
        }
    }
    else
        fprintf(stderr, "Error:参数过多\n");
}
/***
 * @description: 清屏
 * @return {*}
 */
void CLR()
{
    cout << "\e[1;1H\e[2J";
}
/***
 * @description: 输出当前时间
 * @return {*}
 */
void TIME()
{
    time_t now_time = time(NULL);
    tm *t_tm = localtime(&now_time);
    printf("%s\n", asctime(t_tm));
}
/***
 * @description: 改变掩码
 * @param {item} &cmd
 * @return {*}
 */
void UMASK(item &cmd)
{
    if (cmd.content.size() == 1)
    {
        //不带参输出当前掩码
        unsigned int old;
        old = umask(0); //改变掩码以获取当前掩码
        umask(old);     //再改回去
        cout << "UMASK:" << old << endl;
    }
    else if (cmd.content.size() == 2)
    {
        //当输入多个参数
        if (TurnToInt(cmd.content[1]) != -1)
        {
            //简单改变掩码即可
            unsigned int tmp = stoi(cmd.content[1], nullptr, 8);
            umask(tmp);
        }
        else
            fprintf(stderr, "Error:不合法的输入\n");
    }
    else
    {
        //参数过多
        fprintf(stderr, "Error:参数过多\n");
    }
}
/***
 * @description: 输出dir信息，支持带参和不带参，且支持-A，-l，-a参数，但是这些选项不能同时使用
 * @param {item} &cmd
 * @return {*}
 */
void dir(item &cmd)
{
    string option;
    if (cmd.content.size() == 1)
    {
        //不带参，简单输出当前文件夹
        cmd.content.push_back(getcwd(nullptr, 0));
    }
    else if (cmd.content.size() == 3)
    {
        //带参，将参数提取出来
        option = cmd.content[1];
        cmd.content[1] = cmd.content[2];
    }
    else if (cmd.content.size() == 2)
        ;
    else
    {
        fprintf(stderr, "Error:参数过多\n");
        return;
    }
    //获取dir信息
    DIR *dp = opendir(cmd.content[1].c_str());
    if (dp == nullptr)
    {
        fprintf(stderr, "打开%s文件夹失败\n", cmd.content[1].c_str());
    }
    //提取信息
    dirent *info;
    try
    {
        if (cmd.content.size() == 2 || cmd.content.size() == 1)
        {
            //不带参数时
            while ((info = readdir(dp)) != nullptr)
            {
                //不输出隐藏文件
                if (info->d_name[0] != '.')
                    cout << setw(10) << setiosflags(ios::left) << info->d_name;
            }
            cout << endl;
        }
        else
        {
            //带参数选项
            while ((info = readdir(dp)) != nullptr)
            {
                if (option == "-A")
                {
                    //输出隐含但不包括'.','..'这两个文件
                    if (info->d_name != "." || info->d_name != "..")
                        cout << setw(10) << setiosflags(ios::left) << info->d_name;
                }
                else if (option == "-a")
                {
                    //输出隐含文件
                    cout << setw(10) << setiosflags(ios::left) << info->d_name;
                }
                else if (option == "-l")
                {
                    //长列表形式输出
                    cout << setw(8) << setiosflags(ios::left) << info->d_ino
                         << setw(10) << setiosflags(ios::left) << info->d_name
                         << setw(8) << setiosflags(ios::left) << info->d_type << endl;
                }
                else
                {
                    //无用的参数，默认为一般输出
                    if (info->d_name[0] != '.')
                        cout << setw(10) << setiosflags(ios::left) << info->d_name;
                }
            }
            if (option != "-l")
                cout << endl;
        }
    }
    catch (...)
    {
        //处理未知异常
        cout << "unexcepted error" << endl;
    }
}
/***
 * @description: 打印环境变量
 * @return {*}
 */
void SET()
{
    extern char **environ; //直接获取环境变量
    for (int i = 0; environ[i]; i++)
        cout << environ[i] << endl;
}
/***
 * @description: 打印命令
 * @param {item} &cmd
 * @return {*}
 */
void ECHO(item &cmd)
{
    //输入什末打印什末
    for (auto i : cmd.content)
    {
        cout << i << " ";
    }
    cout << endl;
}
/***
 * @description: 执行外部命令
 * @param {item} &cmd
 * @return {*}
 */
void EXEC(item &cmd)
{
    //提取字符串并调用execvp方法
    char *arg[cmd.content.size()];
    int cnt = 0;
    for (auto j : cmd.content)
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
    //调用execvp方法
    if (!execvp(cmd.content[0].c_str(), arg))
        fprintf(stderr, "MYSHELL:NOT FOUND COMMAND");
}
/***
 * @description: test对比命令
 * @param {item} &cmd
 * @return {*}
 */
void TEST(item &cmd)
{
    int res = 0;
    //对比数字
    if (cmd.content[2] == "-eq")
        res = (TurnToInt(cmd.content[1]) == TurnToInt(cmd.content[3]));
    else if (cmd.content[2] == "-ge")
        res = (TurnToInt(cmd.content[1]) >= TurnToInt(cmd.content[3]));
    else if (cmd.content[2] == "-lt")
        res = (TurnToInt(cmd.content[1]) < TurnToInt(cmd.content[3]));
    else if (cmd.content[2] == "-ne")
        res = (TurnToInt(cmd.content[1]) != TurnToInt(cmd.content[3]));
    else if (cmd.content[2] == "-gt")
        res = (TurnToInt(cmd.content[1]) > TurnToInt(cmd.content[3]));
    else if (cmd.content[2] == "-le")
        res = (TurnToInt(cmd.content[1]) <= TurnToInt(cmd.content[3]));
    //字符串对比
    else if (cmd.content[2] == "=")
        res = (cmd.content[1] == cmd.content[3]);
    else if (cmd.content[2] == "!=")
        res = (cmd.content[1] != cmd.content[3]);
    cout << res << endl;
}
/***
 * @description: exit命令
 * @param {item} &tmp
 * @return {*}
 */
void EFUN(item &tmp)
{
    exit(0);
}