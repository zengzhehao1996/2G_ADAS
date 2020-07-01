## 编译指南
进入apps目录下，运行相应版本的编译脚本。编译成功后生成的bin文件会出现在out目录下  
```
/* eg: 编译用于V1.3.3 或 v1.3.4 或 v1.3.5版发布版软件 */
$ cd apps
$ ./v133_build.sh -s
```
```
/* eg: 编译用于V1.3.1 或 v1.3.2版发布版软件 */
$ cd apps
$ ./v131_build.sh -s
```
### 编译选项说明
  -s 连接正式服务器固件,无日志[发布版]  
  -i 连接正式服务器固件,有日志[内测发布版]  
  -d 连接测试服务器固件,有日志[内测版]  
  -f 仅编译fota测试固件  
  --logoff 关日志  


## 硬件支持
- [ ] Linde Forklift Rev1.3
- [x] Linde Forklift Rev1.3.1
- [x] Linde Forklift Rev1.3.2
- [x] Linde Forklift Rev1.3.3
- [x] Linde Forklift Rev1.3.4
- [x] Linde Forklift Rev1.3.5
- [x] Linde Forklift Rev1.4
- [x] Linde Forklift Rev1.41
- [x] Linde Forklift Rev1.42
- [x] Linde Forklift Rev1.43
