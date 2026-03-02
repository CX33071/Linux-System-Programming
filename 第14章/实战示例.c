//创建设备文件并查看主/次设备号
// sudo mknod /dev/my_null c 1 3
// ls -l /dev/my_null
//查看磁盘分区信息
// lsblk//磁盘和分区布局
// blkid /dev/sda1//分区信息
//查看根文件系统类型
// df -T /
// 查看文件系统的日志模式
// mount | grep /
// 挂在和卸载U盘
// lsblk
// sudo mkdir /mnt/usb
// sudo mount /dec/sda1 /mnt/usb
// ls /mnt/usb
// sudo umount /mnt/usb
//使用mount()挂载tmpfs
#include <stdio.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>
int main(){
    mkdir("/mnt/tmpfs", 0755);
    if(mount("tmpfs","/mnt/tmpfs","tmpfs",MS_NOEXEC,"size=100m")==-1){
        perror("mount failed");
        rmdir("/mnt/tmpfs");
        return 1;
    }
    system("df -h /mnt/tmpfs");
    umount("/mnt/tmpfs");
    // if(umount2("/mnt/tmpfs",MNT_FORCE)==-1){
    //     perror("umounts failed");
    //     return 1;
    // }强制卸载
    rmdir("/mnt/tmpfs");
    return 0;
}
//命令行创建tmpfs挂载点
// sudo mkdir / mnt / mytmpfs 
// sudo mount - t tmpfs - o size =50m tmpfs / mnt / mytmpfs 
// df - h / mnt / mytmpfs
//获取文件系统信息
#include <stdio.h>
#include <sys/statvfs.h>
int main(){
    struct statvfs vfs;
    if(statvfs("/",&vfs)==-1){
        perror("statvfs failed");
        return 1;
    }
    printf("文件系统总容量:%.2f GB\n",
           (double)vfs.f_blocks * vfs.f_bsize / 1024 / 1024 / 1024);
    return 0;
}