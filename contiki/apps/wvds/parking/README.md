# examples/tests/sensor说明

本目录为MMC3316磁传感器测试程序。

## 基本用法

- 一个节点作为接收节点，无线接收多个节点发送的磁场采样数据后通过串口发送到PC；
- 多个节点作为发送节点，采样两个磁传感器的数据并无线广播发送，数据为原始采样值；
- 在PC上运行<things-mist>/apps/sniffer/gui/sniffer程序，从串口接收数据并显示。

### sniffer用法

- 进入源码目录<things-mist>/apps/sniffer/gui/sniffer；
- 双击sniffer.bat运行GUI程序。
	- Packet标签页中上方Description表中ID列为节点号，多个发送节点时该列不同；
	- Magnetic标签页以曲线图显示磁场数据，可在图上方复选框选择要显示的数据轴；
	- Magnetic标签页图上方Mote下拉框可选择查看数据的目标节点；
	- Magnetic标签页图下方右侧MinY/MaxY文本框内可输入指定的纵轴数值范围；
	- Magnetic标签页图下方左侧XScroll复选框选中时，曲线自动随时间左移，不选中不自动左移。该复选框右侧文本框中可输入横轴的时间范围，以秒为单位。
- 如果修改了源代码，双击build.bat进行编译，需安装好Java编译环境和gradle构建工具。

## 编译方法

- 接收节点编译时需在工程Options - C/C++ Compiler - Preprocessor - Defined Symbols中定义 NODEID=1。如果使用AP节点作为接收节点，定义 BOARD\_CADRE1120\_AP=1，如果使用VD
节点作为接收节点，定义 BOARD\_CADRE1120\_VD=1。
- 发送节点编译时需定义 NODEID=?，其中?为大于1的数，有多个发送节点时每个节点需定义不
同的该值重新编译后烧录到节点。需定义 BOARD\_CADRE1120\_VD=1。

## 代码说明

由于节点有2个传感器，传感器相关操作的API均带有一个设备id作为参数，取值为0或1，相关调用必须注意指定正确的设备id。
