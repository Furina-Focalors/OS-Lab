struct stat;
struct rtcdate;

// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
/**
 * @brief 许多的Unix系统都会从文件描述符0指向的文件（包括文件、管道、设备，统称文件）读取数据，然后向文件描述符1指向的文件写入数据。\n 从buf中取至多n个字节写入文件的 当前偏移量 处
 * @param int fd: 文件描述符，指向一个打开的文件。0表示从文件中读取（process的标准输入流），1表示向文件写入（process的标准输出流）。2表示向文件写入错误信息（process的标准错误流）
 * @param const_void* buf: 指向某一段内存的指针，通过该指针可以访问相应的内存片段
 * @param int n: 需要写入的 最大 长度
 * @returns 写入的字节数。如果出现错误，返回-1.
 */
int write(int, const void*, int);
/**
 * @brief 许多的Unix系统都会从文件描述符0指向的文件（包括文件、管道、设备，统称文件）读取数据，然后向文件描述符1指向的文件写入数据。\n 从一个文件的 当前偏移量 起，读取n个字节至buf中
 * @param int fd: 文件描述符，指向一个打开的文件。0表示从文件中读取（process的标准输入流），1表示向文件写入（process的标准输出流）。2表示向文件写入错误信息（process的标准错误流）
 * @param void* buf: 指向某一段内存的指针，通过该指针可以访问相应的内存片段
 * @param int n: 需要读取的 最大 长度
 * @returns 读取到的字节数。如果出现错误，返回-1.
 */
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
/**
 * 创建消息队列
 * @param uint qid: 消息队列的id
 * @return 创建成功返回0，否则返回-1
 */
int createmsq(uint qid);
/**
 * 发送消息
 * @param uint qid:      消息队列的id
 * @param const_void *msg:  指向用户空间消息的指针
 * @param long mtype:    消息的类型
 * @param int msize: 消息的大小
 * @return 发送成功返回0，否则返回-1
 */
int msgsnd(uint qid, const void *msg, long mtype, int msize);
/**
 * 接收消息
 * @param uint qid:      消息队列的id
 * @param const_void *msg:  指向用户空间消息的指针
 * @param long mtype:    消息的类型
 * @param int msize: 消息的大小
 * @return 接收成功返回0，否则返回-1
 */
int msgrcv(uint qid, void *msg, long mtype, int msize);
/**
 * 删除消息队列
 * @param uint qid: 消息队列的id
 * @return 删除成功返回0，否则返回-1
 */
int deletemsq(uint qid);
/**
 * 更改进程的nice值
 * @param pid: 进程号
 * @param val: 新的nice值
 * @return
 */
int updnice(int pid, int val);

// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void printf(int, const char*, ...);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);
