#include <iostream>
#include <sys/utsname.h>
#include <unistd.h>
#include <string>
using namespace std;

int main()
{
    int Errno, fb = 0;
    struct utsname os_Info;
    string sysname;
    string version;
    string machine;
    int uid = getuid();
    int pid = getpid();

    fb = uname(&os_Info);
    if(fb < 0)
    {
        perror("uname erro");
        sysname = "unknow";
        version = "unknow";
        machine = "unknow";
    }
    else
    {
        sysname = os_Info.sysname;
        version = os_Info.version;
        machine = os_Info.machine;
    }

    cout << sysname <<"\n";
    cout << version <<"\n";
    cout << machine <<"\n";
    cout << pid << "\n";
    cout << uid << "\n";

    return 0;
}