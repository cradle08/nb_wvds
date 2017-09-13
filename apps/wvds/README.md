# WVDS系统备忘

陈亮 <953343497@qq.com>

## 相关工程

以下路径均为相对于根目录的相对路径：
```
apps
    wvds
        boot                             # 引导程序(bootloader)源码
        device                           # VD程序主线程
        router                           # RP程序主线程
        gateway                          # AP程序主线程
        updater                          # 升级节点程序主线程
        parking                          # 算法测试程序主线程
        activator                        # 激活器模拟程序
        t
            gui
                src
                    echart               # 电流仪器数据图形化GUI源码
                    gprsmon              # GPRS串口输出分析GUI源码
                    wvdsota              # 无线升级GUI程序源码
                dist
                    echart               # 电流仪器数据图形化GUI的发布文件
                    gprsmon              # GPRS串口输出分析GUI的发布文件
                    parkdemo             # 测试用后台GUI软件的发布文件
                    wvdsota              # 无线升级GUI程序的发布文件
            tool
                wvds-prog                # 菜单式一键烧录工具包
    sniffer
        sniffer.c                        # 监听节点程序主线程
        t
            gui
                src
                    sniffer              # 监听上位机软件(Sniffer)的源码
                dist
                    sniffer              # 监听上位机软件的发布文件
dev
    cc1120                               # CC1120射频驱动程序
    ds3231                               # DS3231 RTC芯片驱动程序
    hmc5983                              # HMC5983磁传感器驱动程序
    m25pe                                # M25PE Flash芯片驱动程序
    m26                                  # M26 GPRS模块驱动程序
    mmc3316                              # MMC3316磁传感器驱动程序
    qmc5883                              # QMC5883磁传感器驱动程序(当前使用)
platform
    cadre1120                            # 适用于本系统各设备PCB的平台代码
    mist-exp5438                         # 基于msp430f5438的通用平台代码
```
说明:
1. 本软件包基于Thingsquare公司发布的thingsquare-mist-1.1.0.zip软件包，该公司是Contiki系统发明人Adam Dunkels创立的商业公司。相对于原始的thingsquare-mist-1.1.0.zip软件包删除了无需使用的对其他节点平台的支持文件，主要是目录platform、dev、contiki\\platform、contiki\\cpu目录下的无关平台文件，便于分析底层代码里找到正确的代码。必要时可从原始软件包得到其他平台的文件。

## 程序编译

- 调试阶段使用Debug配置，编译输出.d43文件，可以进行断点查看、单步运行等联机调试操作；

- 发布阶段使用Release配置，将编译为与bootloader配合使用的输出文件、格式为msp430-txt；

- 发布阶段请使用build.bat进行编译，将自动根据版本信息重命名输出文件并复制到相关
  目录。应预先定义如下两个环境变量：1. WVDS_PROD，指向生产或测试烧录的固件目录，可以使用WVDS.bat进行目录式一键烧录各设备程序；2. WVDS_DIST，指向发布目录(如无线升级软件的固件目录、发布文件备份目录)，支持多个目录、以;分隔。

- Release配置编译后生成的msp430-txt格式文件建议由版本控制软件一并管理，则在软件修改编译后可明显看出程序的变化。保存到代码库后，后续检出后可直接使用，也可以验证该版本源代码的编译结果。需注意的是，代码中有些行如log\_w()使用了其所在文件的路径，如果不同开发人员在不同机器上本软件包解压后的路径不一致，则编译输出的txt文件有不同；建议各开发人员统一路径，比如C:\\WVDS或D:\\WVDS。

## 程序烧录

- 调试阶段，使用IAR直接烧录并进行调试。
- 生产或最终测试阶段，使用菜单式一键烧录脚本进行烧录。

## 版本号问题

- 在各设备程序的主线程文件如device.c、gateway.c中有如下的代码段，定义了发布时应指定的版本号。
```c
#ifndef APP_VER_MAJOR
#define APP_VER_MAJOR 1
#endif
#ifndef APP_VER_MINOR
#define APP_VER_MINOR 2
#endif
#ifndef APP_VER_PATCH
#define APP_VER_PATCH 0
#endif
```
以上代码段对应的软件版本号即为1.2.0。开发人员修改代码后，应手动修改以上几个版本号常量的值，APP\_VER\_MAJOR和APP\_VER\_MINOR只能取值0到9，APP\_VER\_PATCH可取值0到255。修改条件是:
  - 只增加了很小的功能或修正了小错误时，可以只将APP\_VER\_PATCH的值加1；
  - 当修改完成时的某个版本需要通过无线更新发布到已部署的现场设备时，至少应把APP\_VER\_MINOR值加1，后台系统只采用MAJOR和MINOR组成的一字节BCD版本号。

- 应使用git管理本系统的源码，编译时prebuild.sh将自动获取git版本信息附加到代码中进行编译，完成编译后postbuild.sh将根据git版本信息等自动重新命名发布文件。各类型节点重启时将发送消息报告自己的git版本号，便于快速找到对应版本的软件代码。

更详细的软件说明参见t\\doc子目录下的《无线组网\_rev2》等文档。
