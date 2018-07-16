# 说明

重写一个界面类似 gnome-system-tools 及 gnome-control-center 中的用户管理工具，显示在控制中心中。

## 界面参考

https://askubuntu.com/questions/66718/how-to-manage-users-and-groups

http://ubuntuhandbook.org/index.php/2014/05/install-users-groups-management-tool-ubuntu1404/

http://linuxbsdos.com/2012/04/03/creating-and-managing-user-accounts-in-a-gnome-3-or-ubuntu-desktop/

## 代码参考

http://ftp.gnome.org/pub/GNOME/sources/gnome-system-tools/
https://github.com/GNOME/gnome-control-center/

现在有 accountservice dbus 服务，提供了很多用户管理相关的功能，代码可以对接dbus进行实现。

## 编译

```
meson build -Dprefix=/usr
ninja -C build
sudo ninja -C build install
```
