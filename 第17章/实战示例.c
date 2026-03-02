//查看文件acl
getfacl test.txt
//为用户alice添加读写权限
setfacl -m u:alice:rw test.txt
//设置目录的默认ACL,让新建文件继承
setfacl -d -m u:bob:rw fir/
//删除用户alice的ACL,条目
setfacl -x u:alice test.txt
//删除所有扩展ACL,恢复为传统权限
setfacl -b test.txt
//ACL API新建
#include <stdio.h>
#include <sys/acl.h>
#include <pwd.h>
int main(){
    struct passwd* pw = getpwnam("alice");
    uid_t uid = pw->pw_uid;
    acl_t acl = acl_init(1);
    acl_entry_t entry;
    acl_create_entry(&acl, &entry);
    acl_set_tag_type(entry, ACL_USER);
    acl_set_qualifier(entry, &uid);
    acl_permset_t permset;
    acl_get_permset(entry, &permset);
    acl_add_perm(permset, ACL_READ);
    acl_add_perm(permset, ACL_WRITE);
    acl_set_file("file.txt", ACL_TYPE_ACCESS, acl);
    acl_free(acl);
    return 0;
}