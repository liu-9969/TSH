#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

std::vector<string> _split(const string &str, char pattern)
{
	std::vector<string> res;
	if (str == "")
		return res;
	// 在字符串末尾也加入分隔符，方便截取最后一段
	string strs = str + pattern;
	size_t pos = strs.find(pattern);

	while (pos != strs.npos)
	{
		string temp = strs.substr(0, pos);
		res.push_back(temp);
		while (pos < strs.length() && strs[pos] == pattern)
		{
			pos++;
		}
		// 去掉已分割的字符串,在剩下的字符串中进行分割
		strs = strs.substr(pos, strs.size());
		pos = strs.find(pattern);
	}

	return res;
}

int main()
{
    string cmd;
    getline(cin, cmd);

    
}
