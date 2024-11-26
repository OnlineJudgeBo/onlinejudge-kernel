#include "shared.h"
#include "cJSON.h"
#include <ctype.h>
#include <curl/curl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int DEBUG = 0;
char host_name[BUFFER_SIZE] = "localhost";
char user_name[BUFFER_SIZE] = "root";
char password[BUFFER_SIZE] = "password";
char db_name[BUFFER_SIZE] = "judge_db";
char oj_home[BUFFER_SIZE] = "/home/judge/";
char data_list[BUFFER_SIZE][BUFFER_SIZE];
char LANG_NAME[BUFFER_SIZE];
int data_list_len = 0;
int call_counter[CALL_ARRAY_SIZE] = {0};

int port_number = 3306;
int max_running = 10;
int sleep_time = 1;
int java_time_bonus = 5;
int java_memory_bonus = 512;
char java_xms[16] = "128m";
char java_xmx[16] = "512m";
int sim_enable = 1;
int use_max_time = 0;
int http_judge = 0;
char record_call = 1;
double cpu_compensation = 0.5;
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

FILE *read_cmd_output(const char *fmt, ...) {
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

int execute_cmd(const char *fmt, ...) {
  char cmd[BUFFER_SIZE];

  int ret = 0;
  va_list ap;

  va_start(ap, fmt);
  vsprintf(cmd, fmt, ap);
  ret = system(cmd);
  va_end(ap);
  return ret;
}

void write_log(const char *_fmt, ...) {
  va_list ap;
  char fmt[4096];
  strncpy(fmt, _fmt, 4096);
  char buffer[4096];
  sprintf(buffer, "%s/log/client.log", oj_home);
  FILE *fp = fopen(buffer, "ae+");
  if (fp == NULL) {
    fprintf(stderr, "openfile error!\n");
    system("pwd");
  }
  va_start(ap, _fmt);
  vsprintf(buffer, fmt, ap);
  fprintf(fp, "%s\n", buffer);
  printf("%s\n", buffer);

  va_end(ap);
  fclose(fp);
}

void print_runtimeerror(char *err) {
  FILE *ferr = fopen("error.out", "a+");
  fprintf(ferr, "Runtime Error: %s\n", err);
  fclose(ferr);
}

char *escape_string(const char *input) {
  size_t input_len = strlen(input);
  size_t max_output_len = input_len * 2 + 1;

  char *output = (char *)malloc(max_output_len);
  memset(output, 0, sizeof(max_output_len));

  char *ptr = output;
  while (*input) {
    if ((unsigned char)*input == 0xFF) {
      input += 1;
      continue;
    }

    if ((unsigned char)*(input) == 0x0A) {
      input += 1;
      continue;
    }

    if (*input == '\'') {
      *ptr++ = '\\';
    }
    *ptr++ = *input++;
  }
  *ptr = '\0';

  return output;
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

bool read_buf(char *buf, const char *key, char *value) {
  if (strncmp(buf, key, strlen(key)) == 0) {
    strcpy(value, buf + after_equal(buf));
    trim(value);
    return true;
  }
  return false;
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

int isInFile(const char fname[]) {
  int l = strlen(fname);
  if (l <= 3 || strcmp(fname + l - 3, ".in") != 0) {
    return 0;
  } else {
    return l - 3;
  }
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

const char *getFileNameFromPath(const char *path) {
  for (int i = strlen(path); i >= 0; i--) {
    if (path[i] == '/') {
      return &path[i];
    }
  }
  return path;
}

void delnextline(char s[]) {
  int L;
  L = strlen(s);
  while (L > 0 && (s[L - 1] == '\n' || s[L - 1] == '\r')) {
    s[--L] = 0;
  }
}

void stabilize_cpu() {
  struct timespec req, rem;
  req.tv_sec = 0;
  req.tv_nsec = 5000000;

  nanosleep(&req, &rem);
}

void print_call_array() {
  printf("int LANG_%sV[256]={", LANG_NAME);
  int i = 0;
  for (i = 0; i < CALL_ARRAY_SIZE; i++) {
    if (call_counter[i]) {
      printf("%d, ", i);
    }
  }
  printf("0};\n");

  printf("int LANG_%sC[256]={", LANG_NAME);
  for (i = 0; i < CALL_ARRAY_SIZE; i++) {
    if (call_counter[i]) {
      printf("HOJ_MAX_LIMIT, ");
    }
  }
  printf("0};\n");
}