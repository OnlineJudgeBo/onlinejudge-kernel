/*
 * Copyright 2008 sempr <iamsempr@gmail.com>
 *
 * Refacted and modified by starsaminf<starsaminf@gmail.com>
 * Bug report email starsaminf@gmail.com
 *
 * Refacted and modified by zhblue<newsclan@gmail.com>
 * Bug report email newsclan@gmail.com
 *
 * This file is part of HUSTOJ.
 *
 * HUSTOJ is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * HUSTOJ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HUSTOJ. if not, see <http://www.gnu.org/licenses/>.
 */
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <mysql/mysql.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/resource.h>
static int DEBUG = 0;
#define BUFFER_SIZE 1024
#define LOCKFILE "/var/run/judged.pid"
#define CONFIGFILE "/home/judge/etc/judge.conf"
#define JUDGEHOME "/home/judge/"
#define JUDGELOG "/home/judge/log/client.log"
#define LOCKMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define STD_MB 1048576
#define OJ_CI 2

static char host_name[BUFFER_SIZE];
static char user_name[BUFFER_SIZE];
static char password[BUFFER_SIZE];
static char db_name[BUFFER_SIZE];
static char oj_home[BUFFER_SIZE];
static char oj_lang_set[BUFFER_SIZE];
static int port_number;
static int max_running;
static int sleep_time;
static int sleep_tmp;
static bool STOP = false;
static MYSQL *conn;
static MYSQL_RES *res;
static MYSQL_ROW row;

static char query[BUFFER_SIZE];

void call_for_exit(int s)
{
    STOP = true;
    printf("Stopping judged...\n");
}

void write_log(const char *fmt, ...)
{
    va_list ap;
    char buffer[4096];
    FILE *fp = fopen(JUDGELOG, "a+");
    if (fp == NULL)
    {
        fprintf(stderr, "openfile error!\n");
        system("pwd");
    }
    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap);
    fprintf(fp, "%s\n", buffer);
    if (DEBUG)
        printf("%s\n", buffer);
    va_end(ap);
    fclose(fp);
}

int after_equal(char *c)
{
    int i = 0;
    for (; c[i] != '\0' && c[i] != '='; i++)
        ;
    return ++i;
}

void trim(char *c)
{
    char buf[BUFFER_SIZE];
    char *start, *end;
    strcpy(buf, c);
    start = buf;
    while (isspace(*start))
        start++;
    end = start;
    while (!isspace(*end))
        end++;
    *end = '\0';
    strcpy(c, start);
}

bool read_buf(char *buf, const char *key, char *value)
{
    if (strncmp(buf, key, strlen(key)) == 0)
    {
        strcpy(value, buf + after_equal(buf));
        trim(value);
        if (DEBUG)
            printf("%s\n", value);
        return 1;
    }
    return 0;
}

void read_int(char *buf, const char *key, int *value)
{
    char buf2[BUFFER_SIZE];
    if (read_buf(buf, key, buf2))
        sscanf(buf2, "%d", value);
}

// read the configue file
void init_mysql_conf()
{
    FILE *fp = NULL;
    char buf[BUFFER_SIZE];
    host_name[0] = 0;
    user_name[0] = 0;
    password[0] = 0;
    db_name[0] = 0;
    port_number = 3306;
    max_running = 3;
    sleep_time = 1;
    fp = fopen(CONFIGFILE, "r");
    if (fp != NULL)
    {
        while (fgets(buf, BUFFER_SIZE - 1, fp))
        {
            read_buf(buf, "OJ_HOST_NAME", host_name);
            read_buf(buf, "OJ_USER_NAME", user_name);
            read_buf(buf, "OJ_PASSWORD", password);
            read_buf(buf, "OJ_DB_NAME", db_name);
            read_int(buf, "OJ_PORT_NUMBER", &port_number);
            read_int(buf, "OJ_RUNNING", &max_running);
            read_int(buf, "OJ_SLEEP_TIME", &sleep_time);
            read_buf(buf, "OJ_LANG_SET", oj_lang_set);
        }
        sleep_tmp = sleep_time;
    }
    sprintf(query, "SELECT solution_id FROM solution WHERE language in (%s) and result < 2 AND TIMESTAMPDIFF(MINUTE, in_date, NOW()) BETWEEN 0 AND 15 ORDER BY result ASC,solution_id ASC limit %d", oj_lang_set, max_running * 2);
    if (DEBUG)
    {
        write_log("%s", query);
    }
}

/*
 * Limit for child
 */
void run_client(int runid, int clientid)
{
    char buf[BUFFER_SIZE], runidstr[BUFFER_SIZE];
    struct rlimit LIM;
    LIM.rlim_max = 800;
    LIM.rlim_cur = 800;
    setrlimit(RLIMIT_CPU, &LIM);
    LIM.rlim_max = 80 * STD_MB;
    LIM.rlim_cur = 80 * STD_MB;
    setrlimit(RLIMIT_FSIZE, &LIM);
    LIM.rlim_max = STD_MB << 11;
    LIM.rlim_cur = STD_MB << 11;
    setrlimit(RLIMIT_AS, &LIM);
    LIM.rlim_cur = LIM.rlim_max = 200;
    setrlimit(RLIMIT_NPROC, &LIM);
    sprintf(runidstr, "%d", runid);
    sprintf(buf, "%d", clientid);

    if (!DEBUG)
        execl("/usr/bin/judge_client", "/usr/bin/judge_client", runidstr, buf, oj_home, (char *)NULL);
    else
        execl("/usr/bin/judge_client", "/usr/bin/judge_client", runidstr, buf, oj_home, "debug", (char *)NULL);
}

int executesql(const char *sql)
{
    if (mysql_real_query(conn, sql, strlen(sql)))
    {
        if (DEBUG)
            write_log("%s", mysql_error(conn));
        sleep(20);
        conn = NULL;
        return 1;
    }
    else
        return 0;
}

/*
 * connect the database
 */
int init_mysql()
{
    if (conn == NULL)
    {
        conn = mysql_init(NULL);
        const char timeout = 30;
        mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
        if (!mysql_real_connect(conn, host_name, user_name, password, db_name, port_number, 0, 0))
        {
            if (DEBUG)
                write_log("%s", mysql_error(conn));
            sleep(2);
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return executesql("set names utf8");
    }
}
/*
 * Obtain solutions
 */

int _get_jobs_mysql(int *jobs)
{

    if (mysql_real_query(conn, query, strlen(query)))
    {
        if (DEBUG)
            write_log("%s", mysql_error(conn));
        sleep(20);
        return 0;
    }
    res = mysql_store_result(conn);
    int i = 0;
    int ret = 0;
    while ((row = mysql_fetch_row(res)) != NULL)
    {
        jobs[i++] = atoi(row[0]);
    }
    ret = i;
    while (i <= max_running * 2)
        jobs[i++] = 0;
    return ret;
}

int get_jobs(int *jobs)
{
    return _get_jobs_mysql(jobs);
}

bool _check_out_mysql(int solution_id, int result)
{
    char sql[BUFFER_SIZE];
    sprintf(sql, "UPDATE solution SET result=%d,time=0,memory=0,judgetime=NOW() WHERE solution_id=%d and result<2 LIMIT 1", result, solution_id);
    if (mysql_real_query(conn, sql, strlen(sql)))
    {
        syslog(LOG_ERR | LOG_DAEMON, "%s", mysql_error(conn));
        return false;
    }
    else
    {
        if (mysql_affected_rows(conn) > 0ul)
            return true;
        else
            return false;
    }
}

void expire_old_solution()
{
    char sql[BUFFER_SIZE];
    sprintf(sql, "UPDATE solution SET result=13, time=0, memory=0, judgetime=NOW() "
                 "WHERE result<=3 AND TIMESTAMPDIFF(MINUTE, judgetime, NOW()) > 15");

    if (mysql_real_query(conn, sql, strlen(sql))) 
    {
        syslog(LOG_ERR | LOG_DAEMON, "MySQL Error: %s", mysql_error(conn));
    }
    else 
    {
        long affected_rows = mysql_affected_rows(conn);
        syslog(LOG_INFO | LOG_DAEMON, "Expired %ld old solutions", affected_rows);
    }
}

bool check_out(int solution_id, int result)
{
    return _check_out_mysql(solution_id, result);
}

int work()
{
    static int retcnt = 0;
    int i = 0;
    static int workcnt = 0;
    int runid = 0;
    int jobs[max_running * 2 + 1];
    static pid_t ID[100];
    pid_t tmp_pid = 0;

    if (!get_jobs(jobs))
        retcnt = 0;
    /* exec the submit */
    for (int j = 0; jobs[j] > 0; j++)
    {
        runid = jobs[j];

        if (DEBUG)
            write_log("Judging solution %d", runid);
        if (workcnt >= max_running)
        {                                   // if no more client can running
            tmp_pid = waitpid(-1, NULL, 0); // wait 4 one child exit
            workcnt--;
            retcnt++;
            for (i = 0; i < max_running; i++) // get the client id
                if (ID[i] == tmp_pid)
                    break; // got the client id
            ID[i] = 0;
        }
        else
        {                                     // have free client
            for (i = 0; i < max_running; i++) // find the client id
                if (ID[i] == 0)
                    break; // got the client id
        }
        if (workcnt < max_running && check_out(runid, OJ_CI))
        {
            workcnt++;
            ID[i] = fork(); // start to fork
            if (ID[i] == 0)
            {
                if (DEBUG)
                    write_log("<<=sid=%d===clientid=%d==>>\n", runid, i);
                run_client(runid, i); // if the process is the son, run it
                exit(0);
            }
        }
        else
        {
            ID[i] = 0;
        }
    }
    while ((tmp_pid = waitpid(-1, NULL, WNOHANG)) > 0)
    {
        workcnt--;
        retcnt++;
        for (i = 0; i < max_running; i++) // get the client id
            if (ID[i] == tmp_pid)
                break; // got the client id
        ID[i] = 0;
        printf("tmp_pid = %d\n", tmp_pid);
    }

    mysql_free_result(res); // free the memory
    executesql("commit");

    if (DEBUG && retcnt)
        write_log("<<%ddone!>>", retcnt);
    return retcnt;
}

int lockfile(int fd)
{
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    return (fcntl(fd, F_SETLK, &fl));
}

int already_running()
{
    int fd;
    char buf[16];
    fd = open(LOCKFILE, O_RDWR | O_CREAT, LOCKMODE);
    if (fd < 0)
    {
        syslog(LOG_ERR | LOG_DAEMON, "can't open %s: %s", LOCKFILE,
               strerror(errno));
        exit(1);
    }
    if (lockfile(fd) < 0)
    {
        if (errno == EACCES || errno == EAGAIN)
        {
            close(fd);
            return 1;
        }
        syslog(LOG_ERR | LOG_DAEMON, "can't lock %s: %s", LOCKFILE,
               strerror(errno));
        exit(1);
    }
    ftruncate(fd, 0);
    sprintf(buf, "%d", getpid());
    write(fd, buf, strlen(buf) + 1);
    return (0);
}

int daemon_init(void)
{
    pid_t pid;
    if ((pid = fork()) < 0)
        return (-1);
    else if (pid != 0)
        exit(0); /* parent exit */
    /* child continues */
    setsid();       /* become session leader */
    chdir(oj_home); /* change working directory */
    umask(0);       /* clear file mode creation mask */
    close(0);       /* close stdin */
    close(1);       /* close stdout */
    close(2);       /* close stderr */
    return (0);
}

int main(int argc, char **argv)
{
    DEBUG = (argc > 2);
    if (argc > 1)
        strcpy(oj_home, argv[1]);
    else
        strcpy(oj_home, JUDGEHOME);
    chdir(oj_home); // change the dir
    //if (!DEBUG)
    //    daemon_init();
    if (strcmp(oj_home, JUDGEHOME) == 0 && already_running())
    {
        syslog(LOG_ERR | LOG_DAEMON, "This daemon program is already running!\n");
        return 1;
    }
    // struct timespec final_sleep;
    // final_sleep.tv_sec=0;
    // final_sleep.tv_nsec=500000000;
    expire_old_solution();
    init_mysql_conf(); // set the database info
    signal(SIGQUIT, call_for_exit);
    signal(SIGKILL, call_for_exit);
    signal(SIGTERM, call_for_exit);
    int j = 1;
    while (1)
    { // start to run
        while (j && (!init_mysql()))
        {
            j = work();
        }
        sleep(sleep_time);
        j = 1;
    }
    return 0;
}
