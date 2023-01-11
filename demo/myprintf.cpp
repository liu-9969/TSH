#include <iomanip>
#include <iostream>
using namespace std;
struct Information
{
    int id;
    string os_;
    string ip_;
    int uid_;
    int pid_;
};

int main(void) {
    // cout.flags(ios::left); //左对齐
    // cout << setw(10) << -456.98 << "The End" << endl;
    // cout.flags(ios::internal); //两端对齐
    // cout << setw(10) << -456.98 << "The End" << endl;
    // cout.flags(ios::right); //右对齐
    // cout << setw(10) << -456.98 << "The End" << endl;
    // return 0;

    Information info[] = {
        {1,"ubuntu/x86","192.168.75.129:7788",0,45789},
        {1,"centos7cdscsd/x86","134.232.75.129:7788",0,4239},
        {1,"localhost127.0.0.1/","45.687.5.129:7738",0,4389},

    };
    cout << left << setw(5) << "ID" << left << setw(30) << "OS";
    cout << left << setw(25) << "HOST" << left << setw(5) << "UID" << left << setw(5) << "PID"<< endl;

    for (int i = 0; i < sizeof(info) / sizeof(info[0]); ++i)
    {
        cout << left << setw(5) << info[i].id;
        cout << left << setw(30) << info[i].os_;
        cout << left << setw(25) << info[i].ip_;
        cout << left << setw(5) << info[i].uid_;
        cout << left << setw(5) << info[i].pid_ << endl;
    }
}

// #include <iomanip>
// #include <iostream> 
// #include <string>
// using namespace std;
// struct COMMODITY { string Name; int Id; int Cnt; double Price; };
// int main(void) {
//     COMMODITY cmd[] = {
//         {"Fruit", 0x101, 50, 5.268},
//         {"Juice", 0x102, 20, 8.729},
//         {"Meat", 0x104, 30, 10.133},
//     };
//     cout << left << setw(8) << "NAME" << right << setw(8) << "ID";
//     cout << right << setw(8) << "COUNT" << right << setw(8) << "PRICE" << endl;
//     for (int i = 0; i < sizeof(cmd) / sizeof(cmd[0]); ++i) {
//         cout << left << setw(8) << cmd[i].Name;
//         cout << right << hex << showbase << setw(8) << cmd[i].Id;
//         cout << dec << noshowbase << setw(8) << cmd[i].Cnt;
//         cout << fixed << setw(8) << setprecision(2) << cmd[i].Price << endl;
//     }
//     return 0;
// }