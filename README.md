
![Homepage:](https://github.com/zhuyaliang/images/blob/master/000.png)
![Choose Images:](https://github.com/zhuyaliang/images/blob/master/001.png)
![Add User:](https://github.com/zhuyaliang/images/blob/master/002.png)
# Explain

Rewrite an interface similar to user management tools in gnome-system-tools and gnome-control-center, displayed in the control center.

## Interface reference

https://askubuntu.com/questions/66718/how-to-manage-users-and-groups

http://ubuntuhandbook.org/index.php/2014/05/install-users-groups-management-tool-ubuntu1404/

http://linuxbsdos.com/2012/04/03/creating-and-managing-user-accounts-in-a-gnome-3-or-ubuntu-desktop/

## Code reference

http://ftp.gnome.org/pub/GNOME/sources/gnome-system-tools/
https://github.com/GNOME/gnome-control-center/

Now there are accountservice DBUS services, which provide many user management related functions, and code can be implemented in DBUS.

## Compile

```
meson build -Dprefix=/usr
ninja -C build
sudo ninja -C build install



