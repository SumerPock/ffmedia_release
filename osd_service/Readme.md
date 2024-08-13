# ffmedia-capture

ffmedia 媒体采集程序是基于ffmedia框架编写的

## 手动调试
具体参数可看osd.conf描述，更改参数之后运行测试。

```
./demo_osd osd.conf

```

## ffmedia-capture服务

### 调整osd程序及配置文件路径
安装服务之前先检查ffmedia_capture.sh脚本，将脚本的osd_program变量更改为你的程序所在路径，
脚本的osd_conf_file变量更改为你的程序配置文件所在路径。

例如下面描述的osd程序所在的位置是/home/firefly/osd_service/demo_osd, 以osd配置文件所在的位置是/home/firefly/osd_service/osd.conf

```
osd_program=/home/firefly/osd_service/demo_osd
osd_conf_file=/home/firefly/osd_service/osd.conf

```

### 调整监控的gpio
默认监控的gpio是56， 也就是gpio1_d0，如果需要更改监控其他gpio，更改gpio_number变量。
默认gpio阈值为0, 检测gpio与设置阈值相等，执行对应功能。如需更改，更改gpio_threshold_value变量。
默认超时时间为10，检测gpio与设置阈值相等，并且时长大于10秒，执行关机操作。如需更改，更改time_out变量。


```
gpio_number=56
gpio_threshold_value=0
time_out=10
```

### 安装服务
调整好对应程序及配置文件路径及监控gpio配置之后，运行安装命令：

```
sudo ./install.sh
```
demo_osd、ffmedia-capture.service、ffmedia_capture.sh及libff_media.so发生更改需再次执行安装命令进行重新安装。

### 其他

安装之后默认开启自启动，如果需要关闭开机自启动，运行下面命令：

```
#关闭ffmedia-capture服务开机自启动
sudo systemctl disable ffmedia-capture.service
```

如果修改了osd配置文件可以重新运行服务重新加载配置文件

```
#重启ffmedia-capture服务
sudo systemctl restart ffmedia-capture.service
```


其他操作

```
#开启ffmedia-capture服务
sudo systemctl start ffmedia-capture.service

#停止ffmedia-capture服务
sudo systemctl stop ffmedia-capture.service

#参看ffmedia-capture服务状态
sudo systemctl status ffmedia-capture.service

```


