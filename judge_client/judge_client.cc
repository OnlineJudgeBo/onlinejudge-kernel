//
// File:   main.cc
// Author: sempr
// refacted by zhblue
/*
 * Copyright 2008 sempr <iamsempr@gmail.com>
 *
 * Refacted and modified by Samuel Loza<starsaminf@gmail.com>
 * Bug report email starsaminf@gmail.com
 *
 * Refacted and modified by zhblue<newsclan@gmail.com>
 * Bug report email newsclan@gmail.com
 *
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HUSTOJ. if not, see <http://www.gnu.org/licenses/>.
 */
#include "cJSON.h"
#include "okcalls.h"
#include "shared.h"
#include <assert.h>
#include <ctype.h>
#include <curl/curl.h>
#include <dirent.h>
#include <mysql/mysql.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

void write_log(const char *_fmt, ...) {
  va_list ap;
  char fmt[4096];
  strncpy(fmt, _fmt, 4096);
  char buffer[4096];
  // time_t          t = time(NULL);
  // int l;
  sprintf(buffer, "%s/log/client.log", oj_home);
  FILE *fp = fopen(buffer, "ae+");
  if (fp == NULL) {
    fprintf(stderr, "openfile error!\n");
    system("pwd");
  }
  va_start(ap, _fmt);
  vsprintf(buffer, fmt, ap);
  fprintf(fp, "%s\n", buffer);
  if (DEBUG)
    if (DEBUG)
      if (DEBUG)
        printf("%s\n", buffer);

  va_end(ap);
  fclose(fp);
}

int execute_cmd(const char *fmt, ...) {
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

const int call_array_size = 512;
int call_counter[call_array_size] = {0};
static char LANG_NAME[BUFFER_SIZE];

typedef struct {
  int similar_s_id;
  int percentage;
} Similar_Code;

int data_list_has(char *file) {
  for (int i = 0; i < data_list_len; i++) {
    if (strcmp(data_list[i], file) == 0) {
      return 1;
    }
  }
  return 0;
}

int data_list_add(char *file) {
  if (data_list_len < BUFFER_SIZE - 1) {
    strcpy(data_list[data_list_len], file);
    data_list_len++;
    return 0;
  } else {
    return 1;
  }
}

long get_file_size(const char *filename) {
  struct stat f_stat;
  if (stat(filename, &f_stat) == -1) {
    return 0;
  }
  return (long)f_stat.st_size;
}

void init_syscalls_limits(int lang) {
  memset(call_counter, 0, sizeof(call_counter));
  int i;
  if (DEBUG) {
    write_log("init_call_counter:%d", lang);
  }
  if (record_call) {
    for (i = 0; i < call_array_size; i++) {
      call_counter[i] = 0;
    }
  }

  if (lang <= 1 || lang == 13 || lang == 14 || lang == 16 ||
      lang == 20) { // C & C++
    for (i = 0; i == 0 || LANG_CV[i]; i++)
      call_counter[LANG_CV[i]] = HOJ_MAX_LIMIT;
  } else if (lang == 3) { // Java
    for (i = 0; i == 0 || LANG_JV[i]; i++)
      call_counter[LANG_JV[i]] = HOJ_MAX_LIMIT;
  } else if (lang == 6) { // Python
    for (i = 0; i == 0 || LANG_YV[i]; i++)
      call_counter[LANG_YV[i]] = HOJ_MAX_LIMIT;
  } else if (lang == 15 || lang == 17 ||
             lang == 19) { // python3 or python3.7 or python3.12
    for (i = 0; i == 0 || LANG_PY3[i]; i++)
      call_counter[LANG_PY3[i]] = HOJ_MAX_LIMIT;
  }
}

int after_equal(const char *c) {
  if (!c) {
    return 0;
  }

  int i = 0;
  while (c[i] != '\0') {
    if (c[i] == '=') {
      return i + 1;
    }
    i++;
  }
  return 0;
}

void trim(char *c) {
  char buf[BUFFER_SIZE];
  char *start, *end;
  strcpy(buf, c);
  start = buf;
  while (isspace(*start)) {
    start++;
  }
  end = start;
  while (!isspace(*end)) {
    end++;
  }
  *end = '\0';

  strcpy(c, start);
}

bool read_buf(char *buf, const char *key, char *value) {
  if (strncmp(buf, key, strlen(key)) == 0) {
    strcpy(value, buf + after_equal(buf));
    trim(value);
    if (DEBUG) {
      write_log("%s\n", value)
    }
    return 1;
  }
  return 0;
}

void read_double(char *buf, const char *key, double *value) {
  char buf2[BUFFER_SIZE];
  if (read_buf(buf, key, buf2)) {
    sscanf(buf2, "%lf", value);
  }
}

void read_int(char *buf, const char *key, int *value) {
  char buf2[BUFFER_SIZE];
  if (read_buf(buf, key, buf2)) {
    sscanf(buf2, "%d", value);
  }
}

void init_mysql_conf() {
  FILE *fp = NULL;
  char buf[BUFFER_SIZE];
  host_name[0] = 0;
  user_name[0] = 0;
  password[0] = 0;
  db_name[0] = 0;
  port_number = 3306;
  max_running = 3;
  sleep_time = 3;
  strcpy(java_xms, "-Xms32m");
  strcpy(java_xmx, "-Xmx256m");
  fp = fopen(CONFIGFILE, "r");
  if (fp != NULL) {
    while (fgets(buf, BUFFER_SIZE - 1, fp)) {
      read_buf(buf, "OJ_HOST_NAME", host_name);
      read_buf(buf, "OJ_USER_NAME", user_name);
      read_buf(buf, "OJ_PASSWORD", password);
      read_buf(buf, "OJ_DB_NAME", db_name);
      read_int(buf, "OJ_PORT_NUMBER", &port_number);
      read_int(buf, "OJ_JAVA_TIME_BONUS", &java_time_bonus);
      read_int(buf, "OJ_JAVA_MEMORY_BONUS", &java_memory_bonus);
      read_buf(buf, "OJ_JAVA_XMS", java_xms);
      read_buf(buf, "OJ_JAVA_XMX", java_xmx);
      read_int(buf, "OJ_USE_MAX_TIME", &use_max_time);
    }
  }
}

int isInFile(const char fname[]) {
  int l = strlen(fname);
  if (l <= 3 || strcmp(fname + l - 3, ".in") != 0) {
    return 0;
  } else {
    return l - 3;
  }
}

void find_next_nonspace(int &c1, int &c2, FILE *&f1, FILE *&f2, int &ret) {
  // Find the next non-space character or \n.
  while ((isspace(c1)) || (isspace(c2))) {
    if (c1 != c2) {
      if (c2 == EOF) {
        do {
          c1 = fgetc(f1);
        } while (isspace(c1));
        continue;
      } else if (c1 == EOF) {
        do {
          c2 = fgetc(f2);
        } while (isspace(c2));
        continue;
      } else if ((c1 == '\r' && c2 == '\n')) {
        c1 = fgetc(f1);
      } else if ((c2 == '\r' && c1 == '\n')) {
        c2 = fgetc(f2);
      } else {
        if (DEBUG) {
          write_log("%d=%c\t%d=%c", c1, c1, c2, c2);
        }

        ret = OJ_PE;
      }
    }

    if (isspace(c1)) {
      c1 = fgetc(f1);
    }

    if (isspace(c2)) {
      c2 = fgetc(f2);
    }
  }
}

const char *getFileNameFromPath(const char *path) {
  for (int i = strlen(path); i >= 0; i--) {
    if (path[i] == '/') {
      return &path[i];
    }
  }
  return path;
}

// edit by sam show different fails 2016
void make_diff_out(const char *file1, const char *file2, int c1, int c2,
                   const char *path) {
  FILE *f1, *f2;
  f1 = fopen(file1, "r+");
  f2 = fopen(file2, "r+");

  FILE *out;
  out = fopen("diff.out", "a+");

  fprintf(out, "Entrada \n");
  fprintf(out, "=================\n");
  FILE *fin_;
  fin_ = fopen("data.in", "r+");
  char character;
  int limit = 512;

  while (feof(fin_) == 0 && limit--) {
    character = fgetc(fin_);
    fprintf(out, "%c", character);
  }

  if (limit < 0) {
    fprintf(out, "%s", "\n ... \n");
  }

  fclose(fin_);
  fprintf(out, "\n=================\n");
  fprintf(out, "Respuesta Correcta:\n");

  limit = 900;
  while (feof(f1) == 0 && limit--) {
    character = fgetc(f1);
    fprintf(out, "%c", character);
  }

  if (limit < 0) {
    fprintf(out, "\n...\n");
  }

  fprintf(out, "\n-----------------\n");
  fprintf(out, "Tu respuesta:\n");

  limit = 900;
  while (feof(f2) == 0 && limit--) {
    character = fgetc(f2);
    fprintf(out, "%c", character);
  }

  if (limit < 0) {
    fprintf(out, "%s", "\n...\n");
  }

  fprintf(out, "\n=================\n");
  fprintf(out, "\nDato esperado '%c', Tu salida '%c'. \n", c1, c2);
  fclose(out);
}

/*
 * translated from ZOJ judger r367
 * http://code.google.com/p/zoj/source/browse/trunk/judge_client/client/text_checker.cc#25
 *
 */
int compare_zoj(const char *file1, const char *file2) {
  int ret = OJ_AC;
  int c1, c2;
  FILE *f1, *f2;
  f1 = fopen(file1, "r+");
  f2 = fopen(file2, "r+");
  if (!f1 || !f2) {
    ret = OJ_RE;
    write_log("Error opening files: %s %s\n", file1, file2);
  } else {
    while (true) {
      // Find the first non-space character at the beginning of line.
      // Blank lines are skipped.
      c1 = fgetc(f1);
      c2 = fgetc(f2);
      find_next_nonspace(c1, c2, f1, f2, ret);
      // Compare the current line.
      while (true) {
        // Read until 2 files return a space or 0 together.
        while ((!isspace(c1) && c1) || (!isspace(c2) && c2)) {
          if (c1 == EOF && c2 == EOF) {
            goto end;
          }

          if (c1 == EOF || c2 == EOF) {
            break;
          }

          if (c1 != c2) {
            // Consecutive non-space characters should be all exactly the same
            ret = OJ_WA;
            goto end;
          }
          c1 = fgetc(f1);
          c2 = fgetc(f2);
        }

        find_next_nonspace(c1, c2, f1, f2, ret);
        if (c1 == EOF && c2 == EOF) {
          goto end;
        }
        if (c1 == EOF || c2 == EOF) {
          ret = OJ_WA;
          goto end;
        }
        if ((c1 == '\n' || !c1) && (c2 == '\n' || !c2)) {
          break;
        }
      }
    }
  }
end:
  if (ret == OJ_WA) {
    make_diff_out(file1, file2, c1, c2, file1);
  }
  if (ret == OJ_PE) {
    make_diff_out(file1, file2, c1, c2, file1);
  }
  if (f1) {
    fclose(f1);
  }
  if (f2) {
    fclose(f2);
  }
  return ret;
}

void delnextline(char s[]) {
  int L;
  L = strlen(s);
  while (L > 0 && (s[L - 1] == '\n' || s[L - 1] == '\r')) {
    s[--L] = 0;
  }
}

FILE *read_cmd_output(const char *fmt, ...) {
  char cmd[BUFFER_SIZE];

  FILE *ret = 1024;
  va_list ap;

  va_start(ap, fmt);
  vsprintf(cmd, fmt, ap);
  va_end(ap);
  if (DEBUG)
    printf("%s\n", cmd);
  ret = popen(cmd, "r");

  return ret;
}

/* write result back to database */
void _update_solution_mysql(int solution_id, int result, int time, int memory,
                            int sim, int sim_s_id, double pass_rate) {
  char sql[BUFFER_SIZE];
  int sql_len;

  if (oi_mode) {
    sql_len = snprintf(
        sql, BUFFER_SIZE,
        "UPDATE solution SET result=%d, time=%d, memory=%d, pass_rate=%f "
        "WHERE solution_id=%d LIMIT 1",
        result, time, memory, pass_rate, solution_id);
  } else {
    sql_len = snprintf(sql, BUFFER_SIZE,
                       "UPDATE solution SET result=%d, time=%d, memory=%d "
                       "WHERE solution_id=%d LIMIT 1",
                       result, time, memory, solution_id);
  }

  if (sql_len < 0 || sql_len >= BUFFER_SIZE) {
    write_log("Error: SQL buffer overflow detected.\n");
    return;
  }

  if (mysql_real_query(conn, sql, sql_len)) {
    write_log("MySQL Error: %s\n", mysql_error(conn));
    return;
  }

  if (solution_id > 0 && sim_s_id > 0 && sim > 0) {
    sql_len = snprintf(
        sql, BUFFER_SIZE,
        "INSERT INTO similar_code (solution_id, similar_s_id, percentage) "
        "VALUES (%d, %d, %d) "
        "ON DUPLICATE KEY UPDATE similar_s_id = %d, percentage = %d",
        solution_id, sim_s_id, sim, sim_s_id, sim);

    if (sql_len < 0 || sql_len >= BUFFER_SIZE) {
      write_log("Error: SQL buffer overflow detected.\n");
      return;
    }

    if (mysql_real_query(conn, sql, sql_len)) {
      write_log("MySQL Error: %s\n", mysql_error(conn));
    }
  }
}

void update_solution(int solution_id, int result, int time, int memory, int sim,
                     int sim_s_id, double pass_rate) {
  if (result == OJ_TL && memory == 0) {
    result = OJ_ML;
  }
  _update_solution_mysql(solution_id, result, time, memory, sim, sim_s_id,
                         pass_rate);
}

void _addceinfo_mysql(int solution_id) {
  char sql[(1 << 16)], *end;
  char ceinfo[(1 << 16)], *cend;
  FILE *fp = fopen("ce.txt", "r");

  snprintf(sql, (1 << 16) - 1, "DELETE FROM compileinfo WHERE solution_id=%d",
           solution_id);
  mysql_real_query(conn, sql, strlen(sql));

  cend = ceinfo;
  while (fgets(cend, 1024, fp)) {
    cend += strlen(cend);
    if (cend - ceinfo > 40000) {
      break;
    }
  }

  cend = 0;
  end = sql;
  strcpy(end, "INSERT INTO compileinfo VALUES(");
  end += strlen(sql);
  *end++ = '\'';
  end += sprintf(end, "%d", solution_id);
  *end++ = '\'';
  *end++ = ',';
  *end++ = '\'';
  end += mysql_real_escape_string(conn, end, ceinfo, strlen(ceinfo));
  *end++ = '\'';
  *end++ = ')';
  *end = 0;

  if (mysql_real_query(conn, sql, end - sql)) {
    write_log("%s\n", mysql_error(conn));
  }
  fclose(fp);
}

char from_hex(char ch) {
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/* Converts an integer value to its hex character*/
char to_hex(char code) {
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

/* write runtime error message back to database */
void _addreinfo_mysql(int solution_id, const char *filename) {
  if (!filename) {
    write_log("Error: filename is NULL.\n") return;
  }

  write_log("Deleting reinfo solution_id=%d, filename=%s\n", solution_id,
            filename);

  char sql[65536];
  char reinfo[65536];
  char *rend = reinfo;

  FILE *fp = fopen(filename, "r");
  if (!fp) {
    perror("Error opening file");
    return;
  }

  snprintf(sql, sizeof(sql), "DELETE FROM runtimeinfo WHERE solution_id=%d",
           solution_id);

  if (mysql_real_query(conn, sql, strlen(sql))) {
    write_log("MySQL Error (DELETE): %s\n", mysql_error(conn));
    fclose(fp);
    return;
  }

  int limit = 1024;
  int ch;
  while ((ch = fgetc(fp)) != EOF && limit--) {
    if (ch > 0) {
      *rend++ = (char)ch;
    }
  }
  if (limit == 0) {
    rend += snprintf(rend, sizeof(reinfo) - (rend - reinfo), "\n ... \n");
  }
  *rend = '\0';

  fclose(fp);

  write_log("Adding reinfo solution_id=%d\n", solution_id);

  char escaped_reinfo[65536];
  size_t escaped_len =
      mysql_real_escape_string(conn, escaped_reinfo, reinfo, strlen(reinfo));

  snprintf(sql, sizeof(sql),
           "INSERT INTO runtimeinfo (solution_id, info) VALUES ('%d', '%.*s')",
           solution_id, (int)escaped_len, escaped_reinfo);

  if (mysql_real_query(conn, sql, strlen(sql))) {
    write_log("MySQL Error (INSERT): %s\n", mysql_error(conn));
  }

  write_log("End reinfo solution_id=%d\n", solution_id);
}

void addreinfo(int solution_id) { _addreinfo_mysql(solution_id, "error.out"); }

void adddiffinfo(int solution_id) { _addreinfo_mysql(solution_id, "diff.out"); }

void addcustomout(int solution_id) {
  _addreinfo_mysql(solution_id, "user.out");
}

void _update_user_mysql(const char *user_id) {
  if (!user_id || strlen(user_id) == 0) {
    write_log("Error: user_id is NULL or empty.");
    return;
  }

  char sql[BUFFER_SIZE];
  char escaped_user_id[BUFFER_SIZE];

  size_t escaped_len =
      mysql_real_escape_string(conn, escaped_user_id, user_id, strlen(user_id));

  snprintf(sql, sizeof(sql),
           "UPDATE user_activity "
           "SET solved = (SELECT COUNT(DISTINCT problem_id) "
           "FROM solution "
           "WHERE user_id='%.*s' AND result='4') "
           "WHERE user_id='%.*s'",
           (int)escaped_len, escaped_user_id, (int)escaped_len,
           escaped_user_id);

  if (mysql_real_query(conn, sql, strlen(sql))) {
    write_log(mysql_error(conn));
    return;
  }

  snprintf(sql, sizeof(sql),
           "UPDATE user_activity "
           "SET submit = (SELECT COUNT(*) "
           "FROM solution "
           "WHERE user_id='%.*s') "
           "WHERE user_id='%.*s'",
           (int)escaped_len, escaped_user_id, (int)escaped_len,
           escaped_user_id);

  if (mysql_real_query(conn, sql, strlen(sql))) {
    write_log(mysql_error(conn));
  }
}

void update_user(char *user_id) { _update_user_mysql(user_id); }

void _update_problem_mysql(int p_id) {
  if (p_id <= 0) {
    write_log("Error: Invalid problem ID.");
    return;
  }

  char sql[BUFFER_SIZE];

  int sql_len = snprintf(sql, sizeof(sql),
                         "UPDATE problem "
                         "SET accepted = ("
                         "SELECT COUNT(*) FROM solution "
                         "WHERE problem_id = %d AND result = 4) "
                         "WHERE problem_id = %d",
                         p_id, p_id);

  if (sql_len < 0 || static_cast<size_t>(sql_len) >= sizeof(sql)) {
    write_log("Error: SQL buffer overflow detected for accepted update.");
    return;
  }

  if (mysql_real_query(conn, sql, strlen(sql))) {
    write_log(mysql_error(conn));
    return;
  }

  sql_len = snprintf(sql, sizeof(sql),
                     "UPDATE problem "
                     "SET submit = ("
                     "SELECT COUNT(*) FROM solution "
                     "WHERE problem_id = %d) "
                     "WHERE problem_id = %d",
                     p_id, p_id);

  if (sql_len < 0 || sql_len >= (int)sizeof(sql)) {
    write_log("Error: SQL buffer overflow detected for submit update.");
    return;
  }

  if (mysql_real_query(conn, sql, strlen(sql))) {
    write_log(mysql_error(conn));
  }
}

void update_problem(int pid) { _update_problem_mysql(pid); }

int compile(int lang, char *work_dir) {
  int pid;

  const char *CP_C[] = {"gcc",        "Main.c",         "-o",  "Main",
                        "-fno-asm",   "-Wall",          "-lm", "--static",
                        "-std=c++0x", "-DONLINE_JUDGE", NULL};

  const char *CP_X[] = {
      "g++",   "Main.cc", "-o",       "Main",           "-fno-asm",
      "-Wall", "-lm",     "--static", "-DONLINE_JUDGE", NULL};

  const char *CP_CLANG[] = {"clang",    "Main.c",         "-o",  "Main",
                            "-fno-asm", "-Wall",          "-lm", "--static",
                            "-std=c99", "-DONLINE_JUDGE", NULL};

  const char *CP_CLANG_CPP[] = {
      "clang++",    "Main.cc",        "-o",  "Main",
      "-fno-asm",   "-Wall",          "-lm", "--static",
      "-std=c++0x", "-DONLINE_JUDGE", NULL};

  const char *CP_X11[] = {"g++",        "Main.cc",        "-o",  "Main",
                          "-fno-asm",   "-Wall",          "-lm", "--static",
                          "-std=c++11", "-DONLINE_JUDGE", NULL};

  const char *CP_PY[] = {"/usr/bin/python3.7", "-m", "pyflakes", "Main.py",
                         NULL};

  const char *CP_PY12[] = {"/usr/bin/python3.12", "-c",
                           "import py_compile; py_compile.compile(r'Main.py')",
                           NULL};

  const char *CP_CPP20[] = {
      "/bin/sh", "-c",
      "chown judge:judge Main.psc && /usr/bin/dos2unix -b Main.psc  && "
      "/usr/bin/pseint Main.psc --draw Main.psd --fixwincharset --norun "
      "pseint.txt && /usr/bin/psexport --lang=cpp Main.psd Main.cc && g++ "
      "Main.cc -o Main -fno-asm -Wall -lm --static -DONLINE_JUDGE",
      NULL};

  char javac_buf[7][16];
  char *CP_J[7];

  for (int i = 0; i < 7; i++) {
    CP_J[i] = javac_buf[i];
  }

  strncpy(CP_J[0], "javac", BUFFER_SIZE - 1);
  CP_J[0][BUFFER_SIZE - 1] = '\0';

  int written = snprintf(CP_J[1], BUFFER_SIZE, "-J%s", java_xms);
  if (written < 0 || written >= BUFFER_SIZE) {
    write_log("Warning: java_xms truncated to fit into CP_J[1]\n");
  }

  written = snprintf(CP_J[2], BUFFER_SIZE, "-J%s", java_xmx);
  if (written < 0 || written >= BUFFER_SIZE) {
    write_log("Warning: java_xmx truncated to fit into CP_J[2]\n");
  }

  strncpy(CP_J[3], "-encoding", BUFFER_SIZE - 1);
  CP_J[3][BUFFER_SIZE - 1] = '\0';

  strncpy(CP_J[4], "UTF-8", BUFFER_SIZE - 1);
  CP_J[4][BUFFER_SIZE - 1] = '\0';

  strncpy(CP_J[5], "Main.java", BUFFER_SIZE - 1);
  CP_J[5][BUFFER_SIZE - 1] = '\0';
  CP_J[6] = NULL;

  pid = fork();
  if (pid == 0) {
    struct rlimit LIM;
    LIM.rlim_max = 60;
    LIM.rlim_cur = 60;
    setrlimit(RLIMIT_CPU, &LIM);
    alarm(60);
    LIM.rlim_max = 100 * STD_MB;
    LIM.rlim_cur = 100 * STD_MB;
    setrlimit(RLIMIT_FSIZE, &LIM);

    if (lang == 3) {
      LIM.rlim_max = STD_MB << 11;
      LIM.rlim_cur = STD_MB << 11;
    } else {
      LIM.rlim_max = STD_MB << 10;
      LIM.rlim_cur = STD_MB << 10;
    }
    setrlimit(RLIMIT_AS, &LIM);

    if (lang != 2 && lang != 11 && lang != 17) {
      freopen("ce.txt", "w", stderr);
    } else {
      freopen("ce.txt", "w", stdout);
    }

    execute_cmd("chown judge *");
    while (setgid(1536) != 0) {
      sleep(1);
    }

    while (setuid(1536) != 0) {
      sleep(1);
    }

    while (setresuid(1536, 1536, 1536) != 0) {
      sleep(1);
    }

    switch (lang) {
    case 0:
      execvp(CP_C[0], (char *const *)CP_C);
      break;
    case 1:
      execvp(CP_X[0], (char *const *)CP_X);
      break;
    case 3:
      execvp(CP_J[0], (char *const *)CP_J);
      break;
    case 13:
      execvp(CP_CLANG[0], (char *const *)CP_CLANG);
      break;
    case 14:
      execvp(CP_CLANG_CPP[0], (char *const *)CP_CLANG_CPP);
      break;
    case 16:
      execvp(CP_X11[0], (char *const *)CP_X11);
      break;
    case 17:
      execvp(CP_PY[0], (char *const *)CP_PY);
      break;
    case 19:
      execvp(CP_PY12[0], (char *const *)CP_PY12);
      break;
    case 20:
      execvp(CP_CPP20[0], (char *const *)CP_CPP20);
      break;
    default:
      write_log("nothing to do!\n");
    }
    if (DEBUG) {
      write_log("compile end Line 882!\n");
    }
    exit(0);
  } else {
    int status = 0;
    waitpid(pid, &status, 0);
    if (lang > 3 && lang < 7) {
      status = get_file_size("ce.txt");
    }
    if (DEBUG) {
      write_log("status=%d Line 832\n", status);
    }
    return status;
  }
}

int get_proc_status(int pid, const char *mark) {
  FILE *pf;
  char fn[BUFFER_SIZE], buf[BUFFER_SIZE];
  int ret = 0;
  sprintf(fn, "/proc/%d/status", pid);
  pf = fopen(fn, "r");
  int m = strlen(mark);

  while (pf && fgets(buf, BUFFER_SIZE - 1, pf)) {
    buf[strlen(buf) - 1] = 0;
    if (strncmp(buf, mark, m) == 0) {
      sscanf(buf + m + 1, "%d", &ret);
    }
  }

  if (pf) {
    fclose(pf);
  }
  return ret;
}

int init_mysql_conn() {
  conn = mysql_init(NULL);
  const char timeout = 30;
  mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

  if (!mysql_real_connect(conn, host_name, user_name, password, db_name,
                          port_number, 0, 0)) {
    write_log("%s", mysql_error(conn));
    return 0;
  }
  const char *utf8sql = "set names utf8";
  if (mysql_real_query(conn, utf8sql, strlen(utf8sql))) {
    write_log("%s", mysql_error(conn));
    return 0;
  }
  return 1;
}

void _get_remote_solution_mysql(int solution_id, char *code, int lang) {
  char sql[BUFFER_SIZE];

  MYSQL_RES *res;
  MYSQL_ROW row;
  sprintf(sql, "SELECT source FROM source_code WHERE solution_id=%d",
          solution_id);
  mysql_real_query(conn, sql, strlen(sql));
  res = mysql_store_result(conn);
  row = mysql_fetch_row(res);
  sprintf(code, "%s", row[0]);
}

void _get_solution_mysql(int solution_id, char *work_dir, int lang) {
  char sql[BUFFER_SIZE], src_pth[BUFFER_SIZE];
  MYSQL_RES *res;
  MYSQL_ROW row;
  sprintf(sql, "SELECT source FROM source_code WHERE solution_id=%d",
          solution_id);
  mysql_real_query(conn, sql, strlen(sql));
  res = mysql_store_result(conn);
  row = mysql_fetch_row(res);

  sprintf(src_pth, "Main.%s", lang_ext[lang]);
  if (DEBUG) {
    write_log("Main=%s\n", src_pth);
  }
  FILE *fp_src = fopen(src_pth, "w");
  fprintf(fp_src, "%s", row[0]);
  mysql_free_result(res);
  fclose(fp_src);
}

void get_solution(int solution_id, char *work_dir, int lang) {
  _get_solution_mysql(solution_id, work_dir, lang);
}

void _get_solution_info_mysql(int solution_id, int &p_id, char *user_id,
                              int &lang, bool &is_remote_id, int &contest_id) {

  MYSQL_RES *res;
  MYSQL_ROW row;

  char sql[BUFFER_SIZE];
  sprintf(sql,
          "SELECT problem_id, user_id, language, is_remote_oj, contest_id FROM "
          "solution WHERE solution_id=%d",
          solution_id);
  mysql_real_query(conn, sql, strlen(sql));
  res = mysql_store_result(conn);
  row = mysql_fetch_row(res);
  p_id = atoi(row[0]);
  strcpy(user_id, row[1]);
  lang = atoi(row[2]);
  is_remote_id = atoi(row[3]) == 1;
  if (row[4] == NULL) {
    contest_id = -1;
  } else {
    contest_id = atoi(row[4]);
  }
  mysql_free_result(res);
}

void get_solution_info(int solution_id, int &p_id, char *user_id, int &lang,
                       bool &is_remote_id, int &contest_id) {
  _get_solution_info_mysql(solution_id, p_id, user_id, lang, is_remote_id,
                           contest_id);
}

void _get_problem_info_mysql(int p_id, int &time_lmt, int &mem_lmt,
                             int &isspj) {
  char sql[BUFFER_SIZE];
  MYSQL_RES *res;
  MYSQL_ROW row;
  sprintf(
      sql,
      "SELECT time_limit, memory_limit, spj FROM problem where problem_id=%d",
      p_id);
  mysql_real_query(conn, sql, strlen(sql));
  res = mysql_store_result(conn);
  row = mysql_fetch_row(res);
  time_lmt = atoi(row[0]);
  mem_lmt = atoi(row[1]);
  isspj = (row[2][0] == '1');
  mysql_free_result(res);
}

void get_problem_info(int p_id, int &time_lmt, int &mem_lmt, int &isspj) {
  _get_problem_info_mysql(p_id, time_lmt, mem_lmt, isspj);
  if (time_lmt <= 0)
    time_lmt = 1;
}

void prepare_files(char *filename, int namelen, char *infile, int &p_id,
                   char *work_dir, char *outfile, char *userfile,
                   int runner_id) {

  char fname[BUFFER_SIZE];
  strncpy(fname, filename, namelen);
  fname[namelen] = 0;
  sprintf(infile, "%s/data/%d/%s.in", oj_home, p_id, fname);
  execute_cmd("/bin/cp %s %s/data.in", infile, work_dir);
  execute_cmd("/bin/cp %s/data/%d/*.dic %s/", oj_home, p_id, work_dir);

  sprintf(outfile, "%s/data/%d/%s.out", oj_home, p_id, fname);
  sprintf(userfile, "%s/run%d/user.out", oj_home, runner_id);
}

void copy_shell_runtime(char *work_dir) {
  execute_cmd("/bin/mkdir %s/lib", work_dir);
  execute_cmd("/bin/mkdir %s/lib64", work_dir);
  execute_cmd("/bin/mkdir %s/bin", work_dir);
  execute_cmd("/bin/cp /lib/* %s/lib/", work_dir);
  execute_cmd("/bin/cp -a /lib/i386-linux-gnu %s/lib/", work_dir);
  execute_cmd("/bin/cp -a /lib/x86_64-linux-gnu %s/lib/", work_dir);
  execute_cmd("/bin/cp /lib64/* %s/lib64/", work_dir);
  execute_cmd("/bin/cp -a /lib32 %s/", work_dir);
  execute_cmd("/bin/cp /bin/busybox %s/bin/", work_dir);
  execute_cmd("/bin/ln -s /bin/busybox %s/bin/sh", work_dir);
  execute_cmd("/bin/cp /bin/bash %s/bin/bash", work_dir);
}

void copy_python_runtime(char *work_dir) {
  copy_shell_runtime(work_dir);
  execute_cmd("mkdir -p %s/usr/include", work_dir);
  execute_cmd("mkdir -p %s/dev", work_dir);
  execute_cmd("mkdir -p %s/usr/lib", work_dir);
  execute_cmd("mkdir -p %s/usr/lib64", work_dir);
  execute_cmd("mkdir -p %s/usr/local/lib", work_dir);

  execute_cmd("mkdir -p %s/etc/abrt", work_dir);
  execute_cmd("mkdir -p %s/etc/abrt/plugins", work_dir);
  execute_cmd(
      "cp -a /etc/abrt/plugins/python.conf %s/etc/abrt/plugins/python.conf",
      work_dir);

  execute_cmd("mkdir -p %s/usr/share", work_dir);
  execute_cmd("mkdir -p %s/usr/share/abrt/", work_dir);
  execute_cmd("mkdir -p %s/usr/share/abrt/conf.d", work_dir);
  execute_cmd("mkdir -p %s/usr/share/abrt/conf.d/plugins", work_dir);
  execute_cmd("cp -a /usr/share/abrt/conf.d/plugins/python.conf "
              "%s/usr/share/abrt/conf.d/plugins/python.conf",
              work_dir);

  execute_cmd("cp /usr/bin/python* %s/", work_dir);
  execute_cmd("cp -a /usr/lib/python* %s/usr/lib/", work_dir);
  execute_cmd("cp -a /usr/lib64/python* %s/usr/lib64/", work_dir);
  execute_cmd("cp -a /usr/local/lib/python* %s/usr/local/lib/", work_dir);
  execute_cmd("cp -a /usr/include/python* %s/usr/include/", work_dir);
  execute_cmd("cp -a /usr/lib/libpython* %s/usr/lib/", work_dir);
  execute_cmd("/bin/mkdir -p %s/home/judge", work_dir);
  execute_cmd("/bin/chown judge %s", work_dir);
  execute_cmd("/bin/mkdir -p %s/etc", work_dir);
  execute_cmd("/bin/grep judge /etc/passwd>%s/etc/passwd", work_dir);
  execute_cmd("/bin/mount -o bind /dev %s/dev", work_dir);
  execute_cmd("/bin/mount -o remount, ro %s/dev", work_dir);
}

void copy_python3_runtime(char *work_dir) {
  copy_shell_runtime(work_dir);
  execute_cmd("mkdir -p %s/usr/include", work_dir);
  execute_cmd("mkdir -p %s/dev", work_dir);
  execute_cmd("mkdir -p %s/usr/lib", work_dir);
  execute_cmd("mkdir -p %s/usr/lib64", work_dir);
  execute_cmd("mkdir -p %s/usr/local/lib", work_dir);
  execute_cmd("cp /usr/bin/python* %s/", work_dir);
  execute_cmd("cp -a /usr/lib/python* %s/usr/lib/", work_dir);
  execute_cmd("cp -a /usr/lib64/python* %s/usr/lib64/", work_dir);
  execute_cmd("cp -a /usr/local/lib/python* %s/usr/local/lib/", work_dir);
  execute_cmd("cp -a /usr/include/python* %s/usr/include/", work_dir);
  execute_cmd("cp -a /usr/lib/libpython* %s/usr/lib/", work_dir);
  execute_cmd("/bin/mkdir -p %s/home/judge", work_dir);
  execute_cmd("/bin/chown judge %s", work_dir);
  execute_cmd("/bin/mkdir -p %s/etc", work_dir);
  execute_cmd("/bin/grep judge /etc/passwd>%s/etc/passwd", work_dir);
  execute_cmd("/bin/mount -o bind /dev %s/dev", work_dir);
  execute_cmd("/bin/mount -o remount, ro %s/dev", work_dir);
}

void run_solution(int &lang, char *work_dir, int &time_lmt, int &usedtime,
                  int &mem_lmt) {
  nice(19);
  // now the user is "judger"
  chdir(work_dir);
  // open the files
  freopen("data.in", "r", stdin);
  freopen("user.out", "w", stdout);
  freopen("error.out", "a+", stderr);
  // trace me
  ptrace(PTRACE_TRACEME, 0, NULL, NULL);
  // run me
  if (lang != 3 && lang != 6 && lang != 15 && lang != 17 && lang != 19) {
    chroot(work_dir);
  }

  while (setgid(1536) != 0) {
    sleep(1);
  }

  while (setuid(1536) != 0) {
    sleep(1);
  }

  while (setresuid(1536, 1536, 1536) != 0) {
    sleep(1);
  }

  struct rlimit LIM;
  if (oi_mode) {
    LIM.rlim_cur = time_lmt / cpu_compensation + 1;
  } else {
    LIM.rlim_cur = (time_lmt / cpu_compensation - usedtime / 1000) + 1;
  }
  LIM.rlim_max = LIM.rlim_cur;
  setrlimit(RLIMIT_CPU, &LIM);
  alarm(0);
  alarm(time_lmt * 5 / cpu_compensation);

  LIM.rlim_max = STD_F_LIM + STD_MB;
  LIM.rlim_cur = STD_F_LIM;
  setrlimit(RLIMIT_FSIZE, &LIM);
  switch (lang) {
  case 3: // java
  default:
    LIM.rlim_cur = LIM.rlim_max = 1;
  }

  setrlimit(RLIMIT_NPROC, &LIM);

  // set the stack

  LIM.rlim_cur = STD_MB << 6;
  LIM.rlim_max = STD_MB << 6;
  setrlimit(RLIMIT_STACK, &LIM);
  // set the memory
  LIM.rlim_cur = STD_MB * mem_lmt / 2 * 3;
  LIM.rlim_max = STD_MB * mem_lmt * 2;
  if (lang < 3 || lang == 16) {
    // c,++ y c++11
    setrlimit(RLIMIT_AS, &LIM);
  }
  switch (lang) {
  case 0:
  case 1:
  case 13:
  case 14:
  case 20:
  case 16:
    execl("./Main", "./Main", (char *)NULL);
    break;
  case 3:
    execl("/usr/bin/java", "/usr/bin/java", java_xms, java_xmx,
          "-Djava.security.manager", "-Djava.security.policy=./java.policy",
          "Main", (char *)NULL);
    break;
  case 6: // Python
    execl("/usr/bin/python2.7", "-m", "/usr/bin/python2.7", "Main.py",
          (char *)NULL);
    break;
  case 15: // PYTHON3
    execl("/usr/bin/python3", "/usr/bin/python3", "Main.py", (char *)NULL);
    break;
  case 17: // PYTHON3.7
    execl("/usr/bin/python3.7", "/usr/bin/python3.7", "Main.py", (char *)NULL);
    break;
  case 19: // PYTHON3.12
    execl("/usr/bin/python3.12", "/usr/bin/python3.12", "Main.py",
          (char *)NULL);
    break;
  }
  exit(0);
}

int fix_python_mis_judge(char *work_dir, int &ACflg, int &topmemory,
                         int mem_lmt) {
  int comp_res = OJ_AC;

  comp_res = execute_cmd("/bin/grep 'MemoryError' %s/error.out", work_dir);

  if (!comp_res) {
    printf("Python need more Memory!");
    ACflg = OJ_ML;
    topmemory = mem_lmt * STD_MB;
  }

  return comp_res;
}

int fix_java_mis_judge(char *work_dir, int &ACflg, int &topmemory,
                       int mem_lmt) {
  int comp_res = OJ_AC;
  if (DEBUG)
    execute_cmd("cat %s/error.out", work_dir);
  comp_res = execute_cmd("/bin/grep 'Exception' %s/error.out", work_dir);
  if (!comp_res) {
    printf("Exception reported\n");
    ACflg = OJ_RE;
  }

  comp_res = execute_cmd("/bin/grep 'java.lang.OutOfMemoryError' %s/error.out",
                         work_dir);

  if (!comp_res) {
    printf("JVM need more Memory!");
    ACflg = OJ_ML;
    topmemory = mem_lmt * STD_MB;
  }
  comp_res = execute_cmd("/bin/grep 'java.lang.OutOfMemoryError' %s/user.out",
                         work_dir);

  if (!comp_res) {
    printf("JVM need more Memory or Threads!");
    ACflg = OJ_ML;
    topmemory = mem_lmt * STD_MB;
  }
  comp_res = execute_cmd("/bin/grep 'Could not create' %s/error.out", work_dir);
  if (!comp_res) {
    printf("jvm need more resource, tweak -Xmx(OJ_JAVA_BONUS) Settings");
    ACflg = OJ_RE;
  }
  return comp_res;
}

int special_judge(char *oj_home, int problem_id, char *infile, char *outfile,
                  char *userfile) {
  pid_t pid;
  write_log("special judge pid=%d\n", problem_id);
  pid = fork();
  int ret = 0;
  if (pid == 0) {

    while (setgid(1536) != 0) {
      sleep(1);
    }
    while (setuid(1536) != 0) {
      sleep(1);
    }
    while (setresuid(1536, 1536, 1536) != 0) {
      sleep(1);
    }

    struct rlimit LIM; // time limit, file limit& memory limit

    LIM.rlim_cur = 5;
    LIM.rlim_max = LIM.rlim_cur;
    setrlimit(RLIMIT_CPU, &LIM);
    alarm(0);
    alarm(10);

    LIM.rlim_max = STD_F_LIM + STD_MB;
    LIM.rlim_cur = STD_F_LIM;
    setrlimit(RLIMIT_FSIZE, &LIM);

    ret = execute_cmd("%s/data/%d/spj %s %s %s", oj_home, problem_id, infile,
                      outfile, userfile);
    if (DEBUG) {
      printf("spj1=%d\n", ret);
    }
    if (ret) {
      exit(1);
    } else {
      exit(0);
    }
  } else {
    int status;

    waitpid(pid, &status, 0);
    ret = WEXITSTATUS(status);
    if (DEBUG) {
      write_log("spj2=%d\n", ret);
    }
  }
  return ret;
}

void judge_solution(int &ACflg, int &usedtime, int time_lmt, int isspj,
                    int p_id, char *infile, char *outfile, char *userfile,
                    int &PEflg, int lang, char *work_dir, int &topmemory,
                    int mem_lmt, int solution_id, double num_of_test) {
  int comp_res;
  if (!oi_mode) {
    num_of_test = 1.0;
  }
  if (ACflg == OJ_AC &&
      usedtime > time_lmt * 1000 * (use_max_time ? 1 : num_of_test)) {
    ACflg = OJ_TL;
  }
  if (topmemory > mem_lmt * STD_MB) {
    ACflg = OJ_ML;
  }
  if (ACflg == OJ_AC) {
    if (isspj) {
      comp_res = special_judge(oj_home, p_id, infile, outfile, userfile);

      if (comp_res == 0) {
        comp_res = OJ_AC;
      } else {
        if (DEBUG) {
          write_log("fail test %s\n", infile);
        }
        comp_res = OJ_WA;
      }
    } else {
      comp_res = compare_zoj(outfile, userfile);
    }
    if (comp_res == OJ_WA) {
      ACflg = OJ_WA;
      if (DEBUG) {
        write_log("fail test %s\n", infile);
      }
    } else if (comp_res == OJ_PE) {
      PEflg = OJ_PE;
    }
    ACflg = comp_res;
  }
  // jvm popup messages, if don't consider them will get miss-WrongAnswer
  if (lang == 3) {
    comp_res = fix_java_mis_judge(work_dir, ACflg, topmemory, mem_lmt);
  }

  if (lang == 6 || lang == 15 || lang == 17 || lang == 19) {
    comp_res = fix_python_mis_judge(work_dir, ACflg, topmemory, mem_lmt);
  }
}

int get_page_fault_mem(struct rusage &ruse, pid_t &pidApp) {
  // java use pagefault
  int m_vmpeak, m_vmdata, m_minflt;
  m_minflt = ruse.ru_minflt * getpagesize();
  if (0 && DEBUG) {
    m_vmpeak = get_proc_status(pidApp, "VmPeak:");
    m_vmdata = get_proc_status(pidApp, "VmData:");
    printf("VmPeak:%d KB VmData:%d KB minflt:%d KB\n", m_vmpeak, m_vmdata,
           m_minflt >> 10);
  }
  return m_minflt;
}
void print_runtimeerror(char *err) {
  FILE *ferr = fopen("error.out", "a+");
  fprintf(ferr, "Runtime Error:%s\n", err);
  fclose(ferr);
}

void watch_solution(pid_t pidApp, char *infile, int &ACflg, int isspj,
                    char *userfile, char *outfile, int solution_id, int lang,
                    int &topmemory, int mem_lmt, int &usedtime, int time_lmt,
                    int &p_id, int &PEflg, char *work_dir) {
  int tempmemory;

  if (DEBUG) {
    write_log("watch_solution pid=%d judging %s\n", pidApp, infile);
  }

  int status, sig, exitcode;
  struct user_regs_struct reg;
  struct rusage ruse;
  while (1) {
    wait4(pidApp, &status, 0, &ruse);

    // jvm gc ask VM before need,so used kernel page fault times and page size
    if (lang == 3) {
      tempmemory = get_page_fault_mem(ruse, pidApp);
    } else { // other use VmPeak
      tempmemory = get_proc_status(pidApp, "VmPeak:") << 10;
    }
    if (tempmemory > topmemory) {
      topmemory = tempmemory;
    }
    if (topmemory > mem_lmt * STD_MB) {
      if (DEBUG) {
        write_log("out of memory %d\n", topmemory);
      }
      if (ACflg == OJ_AC) {
        ACflg = OJ_ML;
      }
      ptrace(PTRACE_KILL, pidApp, NULL, NULL);
      break;
    }
    // sig = status >> 8;/*status >> 8 */
    if (WIFEXITED(status)) {
      break;
    }
    if ((lang < 4 || lang == 9 || lang == 17 || lang == 19) &&
        get_file_size("error.out") && !oi_mode) {
      ACflg = OJ_RE;
      ptrace(PTRACE_KILL, pidApp, NULL, NULL);
      break;
    }

    if (!isspj && get_file_size(userfile) > get_file_size(outfile) * 4 + 1024) {
      if (DEBUG) {
        write_log("get_file_size(%s)=%ld > get_file_size(%s) %ld\n", userfile,
                  get_file_size(userfile), outfile,
                  get_file_size(outfile) * 4 + 1024);
      }
      ACflg = OJ_OL;
      ptrace(PTRACE_KILL, pidApp, NULL, NULL);
      break;
    }

    exitcode = WEXITSTATUS(status);
    if ((lang >= 3 && exitcode == 17) || exitcode == 0x05 || exitcode == 0) {
      ;
    } else {
      if (DEBUG) {
        write_log("status>>8=%d\n", exitcode);
      }

      if (ACflg == OJ_AC) {
        switch (exitcode) {
        case SIGCHLD:
        case SIGALRM:
          alarm(0);
        case SIGKILL:
        case SIGXCPU:
          ACflg = OJ_TL;
          break;
        case SIGXFSZ:
          ACflg = OJ_OL;
          break;
        default:
          ACflg = OJ_RE;
        }
        print_runtimeerror(strsignal(exitcode));
      }
      ptrace(PTRACE_KILL, pidApp, NULL, NULL);

      break;
    }
    if (WIFSIGNALED(status)) {
      /*  WIFSIGNALED: if the process is terminated by signal
       *
       *  psignal(int sig, char *s)，like perror(char *s)，print out s, with
       * error msg from system of sig sig = 5 means Trace/breakpoint trap sig =
       * 11 means Segmentation fault sig = 25 means File size limit exceeded
       */
      sig = WTERMSIG(status);

      if (DEBUG) {
        write_log("WTERMSIG=%d\n", sig);
        psignal(sig, NULL);
      }
      if (ACflg == OJ_AC) {
        switch (sig) {
        case SIGCHLD:
        case SIGALRM:
          alarm(0);
        case SIGKILL:
        case SIGXCPU:
          ACflg = OJ_TL;
          break;
        case SIGXFSZ:
          ACflg = OJ_OL;
          break;

        default:
          ACflg = OJ_RE;
        }
        print_runtimeerror(strsignal(sig));
      }
      break;
    }
    /*     comment from http://www.felix021.com/blog/read.php?1662

     WIFSTOPPED: return true if the process is paused or stopped while ptrace is
     watching on it WSTOPSIG: get the signal if it was stopped by signal
     */

    // check the system calls
    ptrace(PTRACE_GETREGS, pidApp, NULL, &reg);
    if (call_counter[reg.REG_SYSCALL]) {
    } else if (record_call) {
      call_counter[reg.REG_SYSCALL] = 1;
    } else { // do not limit JVM syscall for using different JVM
      ACflg = OJ_RE;
      char error[BUFFER_SIZE];
      sprintf(
          error,
          "[ERROR] A Not allowed system call: runid:%d callid:%ld\n TO FIX "
          "THIS , ask admin to add the CALLID into corresponding LANG_XXV[] "
          "located at okcalls32/64.h ,and recompile judge_client",
          solution_id, (long)reg.REG_SYSCALL);
      write_log(error);
      print_runtimeerror(error);
      ptrace(PTRACE_KILL, pidApp, NULL, NULL);
    }

    ptrace(PTRACE_SYSCALL, pidApp, NULL, NULL);
  }
  usedtime += (ruse.ru_utime.tv_sec * 1000 + ruse.ru_utime.tv_usec / 1000) *
              cpu_compensation;
  usedtime += (ruse.ru_stime.tv_sec * 1000 + ruse.ru_stime.tv_usec / 1000) *
              cpu_compensation;
}

void clean_workdir(char *work_dir) {
  if (DEBUG) {
    execute_cmd("/bin/rm -rf %s/log/* 2>/dev/null", work_dir);
    execute_cmd("mkdir %s/log/ 2>/dev/null", work_dir);
    execute_cmd("/bin/mv %s/* %s/log/ 2>/dev/null", work_dir, work_dir);
  } else {
    execute_cmd("mkdir %s/log/ 2>/dev/null", work_dir);
    execute_cmd("/bin/mv %s/* %s/log/ 2>/dev/null", work_dir, work_dir);
    execute_cmd("/bin/rm -rf %s/log/* 2>/dev/null", work_dir);
  }
}

void init_parameters(int argc, char **argv, int &solution_id, int &runner_id) {
  if (argc < 3) {
    fprintf(stderr, "Usage:%s solution_id runner_id.\n", argv[0]);
    fprintf(stderr, "Multi:%s solution_id runner_id judge_base_path.\n",
            argv[0]);
    fprintf(stderr, "Debug:%s solution_id runner_id judge_base_path debug.\n",
            argv[0]);
    exit(1);
  }
  DEBUG = (argc > 4);
  record_call = (argc > 5);
  if (argc > 5) {
    strcpy(LANG_NAME, argv[5]);
  }
  if (argc > 3) {
    strcpy(oj_home, argv[3]);
  } else {
    strcpy(oj_home, JUDGEHOME);
  }

  chdir(oj_home); // change the dir// init our work

  solution_id = atoi(argv[1]);
  runner_id = atoi(argv[2]);
}

void get_sim(int solution_id, int lang, int pid, int &sim_s_id) {
  printf("Creating AC code solution id = %d, lang=%d, pid=%d \n", solution_id,
         lang, pid);
  char src_pth[BUFFER_SIZE];
  sprintf(src_pth, "Main.%s", lang_ext[lang]);
  execute_cmd("/bin/mkdir ../data/%d/ac/", pid);
  execute_cmd("/bin/cp %s ../data/%d/ac/%d.%s", src_pth, pid, solution_id,
              lang_ext[lang]);
}

void save_contest_solution(int solution_id, int lang, int pid, int contest_id) {
  printf("The solution of the contest is AC, constest_id= %d, solution_id = "
         "%d, problem_id=%d \n",
         contest_id, solution_id, pid);
  char src_pth[BUFFER_SIZE];
  sprintf(src_pth, "Main.%s", lang_ext[lang]);
  execute_cmd("/bin/mkdir -p ../data/contests/%d/problem/%d", contest_id, pid);
  execute_cmd("/bin/cp %s ../data/contests/%d/problem/%d/%d.%s", src_pth,
              contest_id, pid, solution_id, lang_ext[lang]);
}
/**
 * @brief Get percent of similar code
 *
 * @param solution_id
 * @param lang
 * @param p_id
 * @param contest_id
 * @param work_dir
 * @return percentage of Similar_Code
 */
Similar_Code get_similar_code(int solution_id, int lang, int p_id,
                              int contest_id, char *work_dir) {
  char cmd[BUFFER_SIZE];
  int written =
      snprintf(cmd, sizeof(cmd), "/usr/bin/anti_cheating.sh %s %d %d .%s %d",
               oj_home, solution_id, contest_id, lang_ext[lang], p_id);

  if (written < 0) {
    fprintf(stderr, "Error generating work_dir with snprintf.\n");
    exit(EXIT_FAILURE);
  } else if (written >= (int)sizeof(work_dir)) {
    fprintf(stderr, "Error: Buffer overflow detected. work_dir truncated.\n");
    exit(EXIT_FAILURE);
  }

  int first_number;
  int second_number = 0;
  double third_number = 0.0;
  char buffer[BUFFER_SIZE] = "";
  char output[BUFFER_SIZE] = "";

  FILE *fjobs = read_cmd_output("%s", cmd);

  while (fgets(buffer, BUFFER_SIZE, fjobs) != NULL) {
    strcat(output, buffer);
  }
  pclose(fjobs);

  if (sscanf(output, "%d%*[^,],%d%*[^,],%lf", &first_number, &second_number,
             &third_number) != 3) {
    printf("Error reading command output\n");
    printf("%s", output);
  }

  Similar_Code similar;
  similar.similar_s_id = second_number;
  similar.percentage = (int)(third_number * 100);

  if (similar.percentage < 70) {
    similar.percentage = 0;
  }
  return similar;
}

int count_in_files(char *dirpath) {
  const char *cmd = "ls -l %s/*.in|wc -l";
  int ret = 0;
  FILE *fjobs = read_cmd_output(cmd, dirpath);
  fscanf(fjobs, "%d", &ret);
  pclose(fjobs);

  return ret;
}

void print_call_array() {
  printf("int LANG_%sV[256]={", LANG_NAME);
  int i = 0;
  for (i = 0; i < call_array_size; i++) {
    if (call_counter[i]) {
      printf("%d, ", i);
    }
  }
  printf("0};\n");

  printf("int LANG_%sC[256]={", LANG_NAME);
  for (i = 0; i < call_array_size; i++) {
    if (call_counter[i]) {
      printf("HOJ_MAX_LIMIT, ");
    }
  }
  printf("0};\n");
}

static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                            void *userp) {
  return size * nmemb;
}

void send_request_to_patito_judge(const char *json_data,
                                  const char *callback_url, const char *token) {
  CURL *curl;
  CURLcode res;
  struct curl_slist *headers = NULL;

  char auth_header[2024];
  snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", token);

  curl = curl_easy_init();
  if (curl) {
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth_header);

    curl_easy_setopt(curl, CURLOPT_URL, callback_url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      write_log("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
  }
}

void create_json_response(int memory, int code_length, const char *in_date,
                          int result, int time, const char *judgetime,
                          int remote_id, const char *callback_url,
                          const char *token) {

  cJSON *root = cJSON_CreateObject();
  cJSON_AddNumberToObject(root, "time", time);
  cJSON_AddNumberToObject(root, "code_length", code_length);
  cJSON_AddNumberToObject(root, "memory", memory);
  cJSON_AddStringToObject(root, "in_date", in_date);
  cJSON_AddNumberToObject(root, "result", result);
  cJSON_AddStringToObject(root, "judgetime", judgetime ? judgetime : "NULL");
  cJSON_AddNumberToObject(root, "remote_id", remote_id);

  char *json_data = cJSON_Print(root);
  if (json_data) {
    send_request_to_patito_judge(json_data, callback_url, token);
    free(json_data);
  }

  cJSON_Delete(root);
}

void get_solution_json(int solution_id) {
  MYSQL_RES *res;
  MYSQL_ROW row;

  char query[512];
  snprintf(
      query, sizeof(query),
      "SELECT problem_id, time, memory, in_date, result, language, "
      "code_length, judgetime, remote_id, remote_clients.callback_url, token "
      "FROM solution, solution_client, remote_clients "
      "WHERE solution.solution_id = %d "
      "AND solution_client.solution_id = solution.solution_id "
      "AND remote_clients.client_id = solution_client.client_id "
      "AND remote_clients.is_available = 1",
      solution_id);

  if (mysql_query(conn, query)) {
    write_log("%s\n", mysql_error(conn));
    mysql_close(conn);
    return;
  }

  res = mysql_store_result(conn);

  if (res == NULL) {
    write_log("%s\n", mysql_error(conn));
    mysql_close(conn);
    return;
  }

  row = mysql_fetch_row(res);

  if (row) {
    int time = atoi(row[1]);
    int memory = atoi(row[2]);
    const char *in_date = row[3];
    int result = atoi(row[4]);
    int code_length = atoi(row[6]);
    const char *judgetime = row[7] ? row[7] : NULL;
    int remote_id = atoi(row[8]);
    const char *callback_url = row[9];
    const char *token = row[10];

    create_json_response(memory, code_length, in_date, result, time, judgetime,
                         remote_id, callback_url, token);
  }

  mysql_free_result(res);
}

int main(int argc, char **argv) {
  char work_dir[BUFFER_SIZE];
  char user_id[BUFFER_SIZE];
  int solution_id = 1000;
  int runner_id = 0;
  int p_id, time_lmt, mem_lmt, lang, isspj, sim, sim_s_id, max_case_time = 0;
  int contest_id = 0;
  bool is_remote_id = false;

  init_parameters(argc, argv, solution_id, runner_id);
  init_mysql_conf();

  if (!init_mysql_conn()) {
    exit(0);
  }

  int written =
      snprintf(work_dir, sizeof(work_dir), "%s/run%s/", oj_home, argv[2]);

  if (written < 0) {
    write_log("Error generating work_dir with snprintf.\n");
    exit(EXIT_FAILURE);
  } else if (written >= (int)sizeof(work_dir)) {
    write_log("Error: Buffer overflow detected. work_dir truncated.\n");
    exit(EXIT_FAILURE);
  }

  chdir(work_dir);
  if (!DEBUG) {
    clean_workdir(work_dir);
  }

  get_solution_info(solution_id, p_id, user_id, lang, is_remote_id, contest_id);

  if (p_id > 0) {
    get_problem_info(p_id, time_lmt, mem_lmt, isspj);
  }
  write_log("solution_id=%d, work_dir=%s, lang=%d\n", solution_id, work_dir,
            lang);

  get_solution(solution_id, work_dir, lang);
  if (lang >= 3) {
    time_lmt = time_lmt + java_time_bonus;
    mem_lmt = mem_lmt + java_memory_bonus;
    execute_cmd("/bin/cp %s/etc/java0.policy %s/java.policy", oj_home,
                work_dir);
  }

  if (time_lmt > 300 || time_lmt < 1) {
    time_lmt = 300;
  }
  if (mem_lmt > 1024 || mem_lmt < 1) {
    mem_lmt = 1024;
  }

  if (DEBUG) {
    write_log("time: %d mem: %d\n", time_lmt, mem_lmt);
  }

  int Compile_OK;

  Compile_OK = compile(lang, work_dir);
  if (Compile_OK != 0) {
    _addceinfo_mysql(solution_id);
    update_solution(solution_id, OJ_CE, 0, 0, 0, 0, 0.0);
    update_user(user_id);
    update_problem(p_id);
    mysql_close(conn);

    if (!DEBUG) {
      clean_workdir(work_dir);
    } else {
      write_log("compile error");
    }
    exit(0);
  } else {
    update_solution(solution_id, OJ_RI, 0, 0, 0, 0, 0.0);
  }
  char fullpath[BUFFER_SIZE];
  char infile[BUFFER_SIZE];
  char outfile[BUFFER_SIZE];
  char userfile[BUFFER_SIZE];

  written = snprintf(fullpath, sizeof(fullpath), "%s/data/%d", oj_home, p_id);

  if (written < 0 || written >= (int)sizeof(fullpath)) {
    write_log(
        "Error: Buffer overflow or snprintf failed in fullpath generation.\n");
  }

  DIR *dp;
  dirent *dirp;

  if (p_id > 0 && (dp = opendir(fullpath)) == NULL) {

    write_log("No such dir:%s!\n", fullpath);
    mysql_close(conn);
    exit(-1);
  }

  int ACflg, PEflg;
  ACflg = PEflg = OJ_AC;
  int namelen;
  int usedtime = 0, topmemory = 0;

  if (lang == 6)
    copy_python_runtime(work_dir);

  double pass_rate = 0.0;
  int num_of_test = 0;
  int finalACflg = ACflg;

  for (; (oi_mode || ACflg == OJ_AC) && (dirp = readdir(dp)) != NULL;) {

    namelen = isInFile(dirp->d_name);
    if (namelen == 0) {
      continue;
    }

    prepare_files(dirp->d_name, namelen, infile, p_id, work_dir, outfile,
                  userfile, runner_id);

    init_syscalls_limits(lang);

    pid_t pidApp = fork();

    if (pidApp == 0) {
      run_solution(lang, work_dir, time_lmt, usedtime, mem_lmt);
      exit(0);
    } else {
      num_of_test++;
      watch_solution(pidApp, infile, ACflg, isspj, userfile, outfile,
                     solution_id, lang, topmemory, mem_lmt, usedtime, time_lmt,
                     p_id, PEflg, work_dir);

      judge_solution(ACflg, usedtime, time_lmt, isspj, p_id, infile, outfile,
                     userfile, PEflg, lang, work_dir, topmemory, mem_lmt,
                     solution_id, num_of_test);
      if (use_max_time) {
        max_case_time = usedtime > max_case_time ? usedtime : max_case_time;
        usedtime = 0;
      }
    }
    if (oi_mode) {
      if (ACflg == OJ_AC) {
        ++pass_rate;
      }
      if (finalACflg < ACflg) {
        finalACflg = ACflg;
      }

      ACflg = OJ_AC;
    }
  }
  if (ACflg == OJ_AC && PEflg == OJ_PE) {
    ACflg = OJ_PE;
  }

  if (sim_enable && ACflg == OJ_AC && (!oi_mode || finalACflg == OJ_AC)) {
    get_sim(solution_id, lang, p_id, sim_s_id);
    if (contest_id > 0) {
      save_contest_solution(solution_id, lang, p_id, contest_id);
      Similar_Code resul_sim =
          get_similar_code(solution_id, lang, p_id, contest_id, work_dir);
      sim = resul_sim.percentage;
      sim_s_id = resul_sim.similar_s_id;
      printf("Sim percent %d", sim);
      printf("is similar to solution_id=%d\n", sim_s_id);
    }
  } else {
    sim = 0;
  }

  if ((oi_mode && finalACflg == OJ_RE) || ACflg == OJ_RE) {
    if (DEBUG) {
      printf("add RE info of %d..... \n", solution_id);
    }
    addreinfo(solution_id);
  }

  if (use_max_time) {
    usedtime = max_case_time;
  }

  if (ACflg == OJ_TL) {
    usedtime = time_lmt * 1000;
  }

  if (oi_mode) {
    if (num_of_test > 0) {
      pass_rate /= num_of_test;
    }
    update_solution(solution_id, finalACflg, usedtime, topmemory >> 10, sim,
                    sim_s_id, pass_rate);
  } else {
    update_solution(solution_id, ACflg, usedtime, topmemory >> 10, sim,
                    sim_s_id, 0);
  }

  if ((oi_mode && finalACflg == OJ_WA) || ACflg == OJ_WA) {
    if (DEBUG) {
      printf("add diff info of %d..... \n", solution_id);
    }
    if (!isspj) {
      adddiffinfo(solution_id);
    }
  }

  if (ACflg == OJ_PE) {
    adddiffinfo(solution_id);
  }

  printf("Adding user information user_id=%s\n", user_id);
  update_user(user_id);

  printf("Updating problem information p_id=%d\n", p_id);
  update_problem(p_id);
  clean_workdir(work_dir);

  if (is_remote_id) {
    get_solution_json(solution_id);
    printf("Yes is remote code %d\n", solution_id);
  } else {
    printf("Yes is local code %d \n", solution_id);
  }

  if (DEBUG) {
    write_log("result=%d\n", oi_mode ? finalACflg : ACflg);
  }

  mysql_close(conn);
  if (record_call) {
    print_call_array();
  }
  closedir(dp);
  return 0;
}
