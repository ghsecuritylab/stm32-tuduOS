/**
 * 轻量级线程 Light Weight Process
 * 由于是RTT4.02版本的再构建,所以暂时取名为LWT,Process->Task
 * 主要用来实现.bin文件的加载执行等
**/


#include <lwt.h>
#include <rtthread.h>
#include <rthw.h>

#define DBG_TAG    "LWP"
#define DBG_LVL    DBG_WARNING
#include <rtdbg.h>

//---->以下为日志单元的配置项
#define LOG_TAG     "lwt"     // 该模块对应的标签。不定义时，默认：NO_TAG
//#define LOG_LVL     LOG_LVL_DBG   // 该模块对应的日志输出级别。不定义时，默认：调试级别
#include <ulog.h>                 // 必须在 LOG_TAG 与 LOG_LVL 下面
//日志单元配置项结束<----



/**
 * 参数复制 把参数拷贝到lwt的结构体中
**/
static int lwt_argscopy(struct rt_lwt *lwp, int argc, char **argv)
{
    int size = sizeof(int)*3; /* store argc, argv, NULL */
    int *args;
    char *str;
    char **new_argv;
    int i;
    int len;

    for (i = 0; i < argc; i ++)
    {
        size += (rt_strlen(argv[i]) + 1);
    }
    size  += (sizeof(int) * argc);

    args = (int*)rt_malloc(size);
    if (args == RT_NULL)
        return -1;

    str = (char*)((int)args + (argc + 3) * sizeof(int));
    new_argv = (char**)&args[2];
    args[0] = argc;
    args[1] = (int)new_argv;

    for (i = 0; i < argc; i ++)
    {
        len = rt_strlen(argv[i]) + 1;
        new_argv[i] = str;
        rt_memcpy(str, argv[i], len);
        str += len;
    }
    new_argv[i] = 0;
    lwp->args = args;

    return 0;
}

static int lwt_load(const char *filename, struct rt_lwt *lwt, uint8_t *load_addr, size_t addr_size)
{
    int fd;
    uint8_t *ptr;
    int result = RT_EOK;
    int nbytes;
    struct lwt_header header;
    struct lwt_chunk  chunk;

    /* check file name */
    RT_ASSERT(filename != RT_NULL);
    /* check lwp control block */
    RT_ASSERT(lwt != RT_NULL);

    /* 根据加载地址判断地址是否为fix */
    if (load_addr != RT_NULL)
    {
        lwt->lwt_type = LWP_TYPE_FIX_ADDR;
        ptr = load_addr;
    }
    else
    {
        lwt->lwt_type = LWP_TYPE_DYN_ADDR;
        ptr = RT_NULL;
    }


    char* itemname;
    /* 查找文件名(也就是去除掉目录) -> 若不存在待查字符,则返回空指针*/
    itemname = strrchr( filename, '/');
    if(itemname != RT_NULL)
        itemname = filename;//不存在'/',那么直接拿来用
    else
        itemname++;//去除掉'/'只要后面的 比如 bin/app.bin -> app.bin
    //这个参数干嘛用的呢..
    rt_strncpy(lwt->cmd,itemname, 8);
    
    /* 这里需要更换成xipfs 现在暂时是fatfs */
    fd = open(filename, 0, O_RDONLY);
    if (fd < 0)
    {
        dbg_log(DBG_ERROR, "open file:%s failed!\n", filename);
        result = -RT_ENOSYS;
        goto _exit;
    }

    //if()

#if 0
//.bin文件不存在这些
    /* read lwp header */
    nbytes = read(fd, &header, sizeof(struct lwt_header));
    if (nbytes != sizeof(struct lwt_header))
    {
        dbg_log(DBG_ERROR, "read lwp header return error size: %d!\n", nbytes);
        result = -RT_EIO;
        goto _exit;
    }

    /* check file header */
    if (header.magic != LWT_MAGIC)
    {
        dbg_log(DBG_ERROR, "erro header magic number: 0x%02X\n", header.magic);
        result = -RT_EINVAL;
        goto _exit;
    }
#endif
    
_exit:
    return 0;
}

struct rt_lwt *rt_lwt_self(void)
{
    rt_thread_t tid = rt_thread_self();
    if(tid == RT_NULL)
        return RT_NULL;
    return tid->lwp;
}

struct rt_lwp *rt_lwp_new(void)
{
    struct rt_lwt *lwt = RT_NULL;
    //关闭中断 主要是MPU保护这一块
    rt_uint32_t level = rt_hw_interrupt_disable();

    //在pidmap中找一个空闲的位置,如果不存在空闲位置,则满

    
    //如果能找到位置 那么申请变量,初始化参量
    lwt = (struct rt_lwt *)rt_malloc(sizeof(struct rt_lwt));

    if(lwt == RT_NULL)
    {
        LOG_E("no memory for lwp struct!");

    }else{

        //置位,赋0
        rt_memset(lwt, 0, sizeof(*lwt));

        //重置双向链表
        lwt->wait_list.prev = &lwt->wait_list;
        lwt->wait_list.next = &lwt->wait_list;
        lwt->object_list.prev = &lwt->object_list;
        lwt->object_list.next = &lwt->object_list;
        lwt->t_grp.prev = &lwt->t_grp;
        lwt->t_grp.next = &lwt->t_grp;
        lwt->ref = 1;//引用次数

        //lwt->pid = //申请到的pid map中的位置(下标)
        

        //把这个lwt结构体放入 pid_map相应下标形成映射关系
    }



    //重新开启保护
    rt_hw_interrupt_enable(level);

    return lwt;
}

void lwt_ref_inc(struct rt_lwt *lwt)
{
    rt_uint32_t level = rt_hw_interrupt_disable();
    
    lwt->ref++;

    rt_hw_interrupt_enable(level);
}

void lwt_ref_dec(struct rt_lwt *lwt)
{
    rt_uint32_t level = rt_hw_interrupt_disable();
    
    if(lwt->ref > 0)
    {
        lwt->ref--;
        if(lwt->ref == 0)
        {
            //无任何地方引用,执行删除操作

            //共享内存
            //引用对象
            //数据删除

            //以上操作需要防止内存溢出
        }
    }

    rt_hw_interrupt_enable(level);
}

char* lwt_pid2name(pid_t pid)
{
    //
}

pid_t lwt_name2pid(char *name)
{
    //在pid_map中进行查找操作..
    return -RT_ERROR;
}




/**
 * 执行操作
 * envp里存放的是系统的环境变量
**/
void lwt_execve(char *filename, int argc, char **argv, char **envp)
{
    struct rt_lwt *lwt;
    int result;

    if (filename == RT_NULL)
        return -RT_ERROR;

    //把这里换成新建的函数体 类似于c++中的新建一个实例
    lwt = (struct rt_lwt *)rt_malloc(sizeof(struct rt_lwt));
    if (lwt == RT_NULL)
    {
        dbg_log(DBG_ERROR, "lwt struct out of memory!\n");
        return -RT_ENOMEM;
    }
    dbg_log(DBG_INFO, "lwt malloc : %p, size: %d!\n", lwt, sizeof(struct rt_lwt));

    //执行申请内存操作
    rt_memset(lwt, 0, sizeof(*lwt));

    if (lwp_argscopy(lwt, argc, argv) != 0)
    {
        rt_free(lwt);
        return -ENOMEM;
    }

}