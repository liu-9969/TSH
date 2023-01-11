#include <iostream>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void _progress_bar(const char *file_name,float sum,float file_size)
{
	float percent = (sum / file_size) * 100;
	char *sign = "#";
	if ((int)percent != 0){
		sign = (char *)malloc((int)percent + 1);
		strncpy(sign,"####################################################",(int) percent);
	}
	printf("%s %7.2f%% [%-*.*s] %.2f/%.2f mb\r",file_name,percent,50,(int)percent / 2,sign,sum / 1024.0 / 1024.0,file_size / 1024.0 / 1024.0);
	if ((int)percent != 0)
		free(sign);
	fflush(stdout);
}



int main()
{
    int sum = 0;
    int file_size = 1466714112;
    while(1)
    {
        sum += 69632;
        sleep(0.5);
        _progress_bar("ubuntu.iso",sum, file_size );
    }

    return 0;
}

