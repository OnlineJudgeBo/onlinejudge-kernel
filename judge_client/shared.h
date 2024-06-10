#ifndef SHARED_H
#define SHARED_H

#include <mysql/mysql.h>
#include <stdio.h> 

#define STD_MB 1048576
#define STD_T_LIM 2
#define STD_F_LIM (STD_MB << 5)
#define STD_M_LIM (STD_MB << 7)
#define BUFFER_SIZE 512
#define BUFFER_CODE_SIZE 5000

#define LOCKFILE "/var/run/judged.pid"
#define CONFIGFILE "/home/judge/etc/judge.conf"
#define JUDGEHOME "/home/judge/"
#define JUDGELOG "/home/judge/log/client.log"

#define OJ_WT0 0
#define OJ_WT1 1
#define OJ_CI 2
#define OJ_RI 3
#define OJ_AC 4
#define OJ_PE 5
#define OJ_WA 6
#define OJ_TL 7
#define OJ_ML 8
#define OJ_OL 9
#define OJ_RE 10
#define OJ_CE 11
#define OJ_CO 12
#define OJ_TR 13

#ifdef __i386
#define REG_SYSCALL orig_eax
#define REG_RET eax
#define REG_ARG0 ebx
#define REG_ARG1 ecx
#else
#define REG_SYSCALL orig_rax
#define REG_RET rax
#define REG_ARG0 rdi
#define REG_ARG1 rsi
#endif

extern int DEBUG;
extern char host_name[BUFFER_SIZE];
extern char user_name[BUFFER_SIZE];
extern char password[BUFFER_SIZE];
extern char db_name[BUFFER_SIZE];
extern char oj_home[BUFFER_SIZE];
extern char data_list[BUFFER_SIZE][BUFFER_SIZE];
extern int data_list_len;

extern int port_number;
extern int max_running;
extern int sleep_time;
extern int java_time_bonus;
extern int java_memory_bonus;
extern char java_xms[BUFFER_SIZE];
extern char java_xmx[BUFFER_SIZE];
extern int sim_enable;
extern int oi_mode;
extern int use_max_time;
extern int http_judge;
extern int shm_run;
extern char record_call;
extern double cpu_compensation;
extern MYSQL *conn;

extern char lang_ext[21][8];

void write_log(const char *_fmt, ...);
int execute_cmd(const char *fmt, ...);
void print_runtimeerror(char *err);
FILE *read_cmd_output(const char *fmt, ...);

#endif
