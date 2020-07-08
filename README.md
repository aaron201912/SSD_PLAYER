# SSD_PLAYER
Ssplaer base on SSD UI

IDE:

	基于ZK UI播放器的IDE。
	编译方法：
	1. 使用FlyThings IDE导入IDE工程；
	2. 选中项目，点击右键选择清空项目，构建项目；
	3. 配置是否启用wifi，player和cloud play功能：
	   在jni/Makefile中有设置模块使能宏：
	   启用wifi：
	   #wlan功能启用开关
	   CONFIG_WLAN_SWITCH := "enable"
	   #CONFIG_WLAN_SWITCH :=
		
	   禁用wifi：
	   #wlan功能启用开关
	   #CONFIG_WLAN_SWITCH := "enable"
	   CONFIG_WLAN_SWITCH :=
	
	   启用player：
	   #player功能启用开关
	   CONFIG_PLAYER_SWITCH := "enable"
           #CONFIG_PLAYER_SWITCH :=
	  
	   关闭player：
	   #player功能启用开关
	   #CONFIG_PLAYER_SWITCH := "enable"
	   CONFIG_PLAYER_SWITCH :=
	   
	   开启cloud play：
	   #cloudplay功能启用开关
	   CONFIG_CLOUD_PLAY_SWITCH := "enable"
	   #CONFIG_CLOUD_PLAY_SWITCH :=
	   
	   关闭cloud play：
	   #cloudplay功能启用开关
	   #CONFIG_CLOUD_PLAY_SWITCH := "enable"
	   CONFIG_CLOUD_PLAY_SWITCH :=
	   
	   注：cloud play功能默认不开启，界面上默认不显示。若需启用此功能，除开启cloud play宏开关外，还需将IDE/ui目录下的main_ftu_with_cloudplay重命名为main.ftu，然后重新编译。
	       若要使用cloud play功能，必须打开player功能开关。
	
	4. 切换AMIC & DMIC
	   修改 arch/arm/boot/dts/infinity2m-ssc011a-s01a-display.dtsi
	   digmic-padmux = <1>;  	// AMIC
	   digmic-padmux = <2>;    	// DMIC
	   
	   修改 jni/voicedetect/voicedetect.cpp
	   #define USE_AMIC	1	// AMIC
	   #define USE_AMIC	0	// DMIC
	
	5. customer_player.tar.gz和customer_without_player.tar.gz是程序的运行环境：
	   customer_player.tar.gz为开启用播放器功能的运行环境，会依赖于ffmpeg库；
	   customer_without_player.tar.gz为关闭播放器功能的运行环境，不依赖于ffmpeg库。
	   根据配置情况解压对应的包即可。
	   如果有修改UI资源相关部分，需要将构建项目后生成的ui文件夹替换到解压后的res目录中。
		
app:

	用来点屏的app，它会call到IDE中编译出来的so
	编译方法：
	1. 根据需要点的panel修改sstardisp.c打开对应的配置：
	   #define UI_1024_600 1
	2. make clean;make即可
		
customer_player.tar.gz:

	带播放器的app运行环境。
	1. 解压后将etc下内容拷贝至板子/etc目录下；
	2. 将bin下zkgui bin拷贝到板子/customer目录下，根据使用的panel选择zkgui_1024x600或zkgui_800x480，以1024x600的panel为例：
	   copy zkgui_1024x600 /customer/zkgui
	3. 将lib，res目录拷贝到板子/customer目录下；
	4. 如果打开了cloud_play功能，则将libdns拷贝到板子/customer目录下；
	5. 将IDE导出生成的libzkgui.so复制到/customer/lib下，覆盖同名文件；
	6. 将IDE导出生成的ui目录复制到/customer/res下，覆盖同名目录
	
customer_without_player.tar.gz	

	不带播放器的app运行环境。
	操作步骤同上。
	
tool:

	用来改变触屏分辨率配置的文件：
	echo 1024x600.bin > /sys/bus/i2c/devices/1-005d/gtcfg
	echo 800x480.bin > /sys/bus/i2c/devices/1-005d/gtcfg

多进程播放器运行说明：
        增加播放器与UI独立进程的模式, 通过配置Makefile中CONFIG_MULTIPROC_SWITCH选项决定是否开启
        使用sdk下zk_full例程演示, 运行多进程播放器按如下操作.
        1.cd myplayer
        2.make clean;make 
        3.将生成的MyPlayer拷贝到板子/customer分区
        4.在IDE中修改Makefile, 设置CONFIG_MULTIPROC_SWITCH := "enable", 将编译出来的libzkgui.so拷贝到板子/customer分区
        5.cd /customer/lib, 运行：export LD_LIBRARY_PATH=/lib:/customer/libdns:/config/wifi:$PWD:$LD_LIBRARY_PATH
        6.cd /customer, 首先运行：./MyPlayer &, 然后运行：./zkgui &

运行播放器：

	1.将app编译出来的zkgui copy到customer分区；
	2.将customer_zk的res和lib copy到customer分区；
	3.将customer_zk的etc下面的文件拷贝到板子/etc下面；
	4.cd /customer/lib,执行：export LD_LIBRARY_PATH=/lib:/customer/libdns:/config/wifi:$PWD:$LD_LIBRARY_PATH
	5.运行zkgui: ./zkgui
	
	注：如果没有声音可能是功放的gpio没有拉高，可以参考如下步骤拉高：
	1. cd /sys/class/gpio/
	2. echo 12 > export
	3. echo out > direction
	4. echo 1 > value


