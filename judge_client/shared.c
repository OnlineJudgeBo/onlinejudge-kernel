#include "shared.h"
#include "cJSON.h"
#include <stdio.h> 
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

int DEBUG = 0;
char host_name[BUFFER_SIZE] = "localhost";
char user_name[BUFFER_SIZE] = "root";
char password[BUFFER_SIZE] = "password";
char db_name[BUFFER_SIZE] = "judge_db";
char oj_home[BUFFER_SIZE] = "/home/judge/";
char data_list[BUFFER_SIZE][BUFFER_SIZE];
int data_list_len = 0;

int port_number = 3306;
int max_running = 10;
int sleep_time = 1;
int java_time_bonus = 5;
int java_memory_bonus = 512;
char java_xms[BUFFER_SIZE] = "128m";
char java_xmx[BUFFER_SIZE] = "512m";
int sim_enable = 1;
int use_max_time = 0;
int http_judge = 0;
bool oi_mode = false;
char record_call = 0;
double cpu_compensation = 1.0;
MYSQL *conn = NULL;

char lang_ext[21][8] = {
    "c",    // 0
    "cc",   // 1
    "pas",  // 2
    "java", // 3
    "rb",   // 4
    "sh",   // 5
    "py",   // 6
    "php",  // 7
    "pl",   // 8
    "cs",   // 9
    "m",    // 10
    "bas",  // 11
    "scm",  // 12
    "c",    // 13
    "cc",   // 14
    "py",   // 15
    "cc",   // 16
    "py",   // 17
    "go",   // 18
    "py",   // 19
    "psc",  // 20
};

FILE *read_cmd_output(const char *fmt, ...)
{
    char cmd[BUFFER_SIZE];

    FILE *ret = NULL;
    va_list ap;

    va_start(ap, fmt);
    vsprintf(cmd, fmt, ap);
    va_end(ap);
    if (DEBUG)
        printf("%s\n", cmd);
    ret = popen(cmd, "r");

    return ret;
}

int execute_cmd(const char *fmt, ...)
{
    char cmd[BUFFER_SIZE];

    int ret = 0;
    va_list ap;

    va_start(ap, fmt);
    vsprintf(cmd, fmt, ap);
    if (DEBUG)
        printf("%s\n", cmd);

    ret = system(cmd);
    va_end(ap);
    return ret;
}

void write_log(const char *_fmt, ...)
{
    va_list ap;
    char fmt[4096];
    strncpy(fmt, _fmt, 4096);
    char buffer[4096];
    sprintf(buffer, "%s/log/client.log", oj_home);
    FILE *fp = fopen(buffer, "ae+");
    if (fp == NULL)
    {
        fprintf(stderr, "openfile error!\n");
        system("pwd");
    }
    va_start(ap, _fmt);
    vsprintf(buffer, fmt, ap);
    fprintf(fp, "%s\n", buffer);
    if (DEBUG)
        printf("%s\n", buffer);

    va_end(ap);
    fclose(fp);
}

void print_runtimeerror(char *err)
{
    FILE *ferr = fopen("error.out", "a+");
    fprintf(ferr, "Runtime Error: %s\n", err);
    fclose(ferr);
}
