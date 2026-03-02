//创建和移除硬链接
link("file.txt", "file_link.txt");
unlink("file_link.txt");
//创建符号链接和读取符号链接目标
symlink("file.txt", "file_symlink");
readlink("file_symlink", buf, sizeof(buf));
//创建目录
mkdir("test", 0755);
rmdir("test");
//打开目录+读取目录
DIR* dir = opendir(".");
struct dirent* entry;
while((entry=readdir(dir))!=NULL){
    printf();
}
closedir(dir);
//获取当前工作目录+改变当前工作目录
getcwd(buf, sizeof(buf));
chdir("/tmp");
//针对目录文件描述符的相关操作
int fd = open(".", O_RDONLY | O_DIRRCETORY);
DIR* dir = fdopendir(fd);
struct dirent* entry = readdir(dir);
closedir(dir);
//解析路径名,返回绝对路径
realpath("test", buf);
//提取目录部分和文件部分
dirname(path);
basename(path);