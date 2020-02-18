/**
 * XIP文件系统
 * 主要是存放一些可以执行xip操作的文件
 * 参考FrostOS
**/
#include <xipfs.h>
#include <rtthread.h>

#include <dfs_fs.h>
#include <dfs_file.h>

#include <easyflash.h>

struct ef_env_dev{
    uint32_t env_start_addr;
    const ef_env *default_env_set;//key->value
    size_t default_env_set_size;
    _Bool init_ok;
    _Bool gc_request;
    _Bool in_recovery_check;
    void *flash;
    size_t sector_size;
    struct env_cache_node env_cache_table[16];
    struct sector_cache_node sector_cache_table[4];
};
typedef struct ef_env_dev *ef_env_dev_t;


struct root_direct{
    struct ef_env_dev env_dev;
    //env_meta_data env;
    uint32_t sec_addr;
};

struct root_direct* xip_mount_table[2] = {RT_NULL};

struct root_direct* get_env_by_dev(rt_device_t dev_id)
{
    void* deviceType = dev_id->parent.type;//type of kernel object

    for(int table_id = 0; table_id < 2; ++table_id)
    {
        if(xip_mount_table[table_id]->env_dev.flash == dev_id->parent.type)
            return xip_mount_table[table_id];
    }

    return 0;
}

/**
 * 文件系统 挂载
 * 把fs->path下挂载文件系统,其中含有私有数据data
 * fs   : path,ops,dev_id
 * data : 文件系统的私有数据 在dfs_mount调用中自行传入
**/
int dfs_xipfs_mount(struct dfs_filesystem *fs,
                    unsigned long          rwflag,
                    const void            *data)
{
    if(fs->dev_id->type == RT_NULL)
    {
        rt_kprintf("The flash device type must be Char!\n");
        /* Not a character device */
        return -ENOTTY;
    }

    /* get env by dev_id */
    if( get_env_by_dev(fs->dev_id) )
        return RT_EOK;

    /* 如果没有挂载表 那么在这里新建挂载表 */
    for(int table_id = 0; table_id < 2; ++table_id)
    {
        ef_env_dev_t ef_env_dev = rt_malloc(sizeof(ef_env_dev_t));
        ef_env_dev->flash = fs->dev_id->parent.type;//得到flash?
        ef_env_dev->sector_size = fs->dev_id;//得到sector_size

        //ef_env_init_by_flash(ef_env_dev);

    }

    //如果挂载表新建成功 那么需要执行清理等步骤?
    //清理:就是把app分区里面的碎片给整理了,
    //换句话说 可以把app一个个的搬运到新分区中,这样可以形成新表
    return RT_EOK;

}

/**
 * 取消挂载
 * 在这里需要停止所有xip操作吗?
**/
int dfs_xipfs_unmount(struct dfs_filesystem *fs)
{
    struct root_direct* root = get_env_by_dev(fs->dev_id);
    
    if(root)
        rt_free(root);

    return RT_EOK;
}

/**
 * 在某个dev_id上初始化xipfs文件系统
**/
int dfs_xipfs_mkfs(rt_device_t dev_id)
{
    struct root_direct* mount_table;
    mount_table = get_env_by_dev(dev_id);
    
    //以下函数需要改造
    if(mount_table)
        ef_env_set_default();//ef_env_set_default(mount_table->env_dev);

    return RT_EOK;
}

/**
 * 获取文件磁盘信息 放在buf中
**/
int dfs_xipfs_statfs(struct dfs_filesystem *fs, struct statfs *buf)
{
    buf->f_bsize  = 512;
    buf->f_blocks = 512;//ramfs->memheap.pool_size / 512;
    buf->f_bfree  = 512;//ramfs->memheap.available_size / 512;
    return RT_EOK;
}


/**
 * 文件系统 改名
**/
int dfs_xipfs_rename(struct dfs_filesystem *fs,
                     const char            *oldpath,
                     const char            *newpath)
{
    return -1;
}


/**
 * XIP文件系统 删除操作
**/
int dfs_xipfs_unlink(struct dfs_filesystem *fs, const char *path)
{
    return -RT_ERROR;
}

/**
 * XIP文件系统 获得文件状态
**/
int dfs_xipfs_stat(struct dfs_filesystem *fs,
                   const char            *path,
                   struct stat           *st)
{
    //
    return RT_EOK;
}

/**
 * XIP下 文件操作方法
**/
static const struct dfs_file_ops _dfs_xip_fops =
{
    RT_NULL,//dfs_elm_open,
    RT_NULL,//dfs_elm_close,
    RT_NULL,//dfs_elm_ioctl,
    RT_NULL,//dfs_elm_read,
    RT_NULL,//dfs_elm_write,
    RT_NULL,//dfs_elm_flush,
    RT_NULL,//dfs_elm_lseek,
    RT_NULL,//dfs_elm_getdents,
    RT_NULL, /* poll interface */
};

static const struct dfs_filesystem_ops _dfs_xipfs =
{
    "xip",
    DFS_FS_FLAG_DEFAULT,
    &_dfs_xip_fops,

    dfs_xipfs_mount,
    dfs_xipfs_unmount,
    dfs_xipfs_mkfs,
    dfs_xipfs_statfs,

    dfs_xipfs_unlink,
    dfs_xipfs_stat,
    dfs_xipfs_rename,
};

int dfs_xipfs_init(void)
{
    /* register ram file system */
    dfs_register(&_dfs_xipfs);

    return 0;
}
//INIT_COMPONENT_EXPORT(dfs_xipfs_init);
