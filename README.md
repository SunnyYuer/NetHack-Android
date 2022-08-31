# NetHack安卓汉化版

更改于英文NetHack安卓版，地址为https://github.com/gurrhack/NetHack-Android

[NetHack](http://www.nethack.org/)是一个类似于龙与地下城的游戏，在此你(一位冒险家)要深入地牢，寻找传说中的岩德护身符，而它被藏在了第二十层以下的某个地方。你和一个可以在各个方面帮助你的宠物开始了冒险，而它可以被训练来做几乎任何事。你会在旅途中发现有用(或没有用)的很有可能包含着魔法的各种物品，以及怪物们。你可以通过移动到怪物所在的格子来攻击怪物(但是通常无视它是更明智的)。

<p align="center">
  <img src="http://imgsrc.baidu.com/forum/w%3D580/sign=66a87f29992f07085f052a08d925b865/387d4459252dd42ac94a6ccd043b5bb5c8eab8e9.jpg">
</p>


##编译说明

=== 准备 ===

1. 下载本项目master分支源码并解压，并下载NetHack的安卓端源码并解压
[下载](https://github.com/SunnyYuer/NetHack-cn/archive/refs/heads/NetHack-Android.zip)

2. 将本项目的android目录整个复制进NetHack-Android/NetHack/sys/目录下

3. 在linux中安装ndk-r19c

4. 安装Android Studio

=== 编译NetHack ===

1. 进入NetHack-Android/NetHack/sys/android目录
2. 打开Makefile.src文件并更改NDK目录为你的NDK目录
3. 终端操作./setup.sh
4. 终端操作cd ../..
5. 终端操作make install

=== 编译app ===

1. 打开Android Studio
2. 选导入Eclipse ADT工程，选择NetHack-Android/NetHack/sys/android目录
3. 工程结构里，选择编译SDK版本，目标SDK版本28，最小SDK版本7
4. 生成签名的APK，勾选签名版本V1和V2

##问题反馈或建议
* 百度贴吧http://tieba.baidu.com/p/4720022918
* [Github Issues](https://github.com/SunnyYuer/NetHack-Android/issues)
* 邮箱sunnyuer@qq.com
