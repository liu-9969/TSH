#include <iostream>
#include <string>
#include <string.h>
int main()
{
    std::string cmd = "get";
    std::cout << cmd << "   length:" << cmd.length() << "   size:"<<cmd.size() << std::endl;

    char arry [6] = {0};
    arry[0] = 'a';
    arry[1] = 'a';
    arry[2] = 'b';
    arry[3] = 'c';
    arry[4] = '\0';
    arry[5] = 'd';

    const char* str = "abcc";
    std::string tem(arry, 6);
    std::string tem1(arry, arry + 5);
    std::string tem2(arry, arry + strlen(arry) + 1);
    std::string tem3(arry);
    std::string tem4(str);
    std::cout << tem << tem.size()<<tem.length()<<"\n";
    std::cout << tem1 << tem1.size() << tem1.length() << "\n";
    std::cout << tem2 << tem2.size() << tem2.length() << "\n";
    std::cout << tem3 << tem3.size() << tem3.length() << "\n";
    std::cout << tem4 << tem4.size() << tem4.length() << "\n";
    return 0;
}