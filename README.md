## Linux TinyShell

a simple Command-Line-Rat-Tool with C++ for Linux

## Features

done:

* 反弹交互式shell
* 文件下载（断点续传、分片、进度条、同步异步两种模式）
* http协议
* 快速切换session， session列表增删改查
* agent保活：健康检查



planed for future

* 流量加密：aes对称加密、https
* agent开机自启 正向建立连接
* 文件上传



## Screenshot

more in img

![](https://github.com/liu-9969/TSH/blob/main/img/CC%E6%8E%A7%E5%88%B6%E5%8F%B01.png)

## how to work

![](https://github.com/liu-9969/TSH/blob/main/img/Linux%20%E8%BF%9C%E6%8E%A7TSH%E5%B7%A5%E4%BD%9C%E5%8E%9F%E7%90%86%E5%9B%BE.png)





## how to use

1. set: core/setting.h 、Log/log.conf
2. make

## 优化
1. 自加的前缀（TSH/ip>）
       * 这本质是http会话状态的问题 
       * 有时候很难看，根本原因在于程序无法显示的知道一条命令何时响应完了，
         比如ls命令, agent很可能会发送三个包过来，而CC是不知道这一点的，agent也不知到自己关于这个命令发个几个包。
         why not response a cc-cmd by one http-pack? (haha 我也想啊，但伪终端不太容易这么做)
       * 优化方法：CC端遍历每个httpbody去搜索结束行那个字符串，如果有就是最后一个包，没有则不是，但这听着就恶心又荒唐...
       * 结束行就是你的shell的前缀（root@ubuntu:/usr/local/src#），每个命令最后都会有这个字符串。
