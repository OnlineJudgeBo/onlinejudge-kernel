#include "utils.h"
#include "shared.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>


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

char from_hex(char ch) {
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/* Converts an integer value to its hex character*/
char to_hex(char code) {
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
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

int fix_python_mis_judge(char *work_dir, int &ACflg, int &topmemory,
                         int mem_lmt) {
  int comp_res = OJ_AC;
  comp_res = execute_cmd("/bin/grep 'MemoryError' %s/error.out", work_dir);

  if (!comp_res) {
    write_log("Python need more Memory!");
    ACflg = OJ_ML;
    topmemory = mem_lmt * STD_MB;
  }

  return comp_res;
}

int fix_java_mis_judge(char *work_dir, int &ACflg, int &topmemory,
                       int mem_lmt) {
  int comp_res = OJ_AC;
  if (DEBUG) {
    execute_cmd("cat %s/error.out", work_dir);
  }

  comp_res = execute_cmd("/bin/grep 'Exception' %s/error.out", work_dir);
  if (!comp_res) {
    write_log("Exception reported\n");
    ACflg = OJ_RE;
  }

  comp_res = execute_cmd("/bin/grep 'java.lang.OutOfMemoryError' %s/error.out",
                         work_dir);

  if (!comp_res) {
    ACflg = OJ_ML;
    topmemory = mem_lmt * STD_MB;
    write_log("JVM need more Memory!");
  }

  comp_res = execute_cmd("/bin/grep 'java.lang.OutOfMemoryError' %s/user.out",
                         work_dir);

  if (!comp_res) {
    ACflg = OJ_ML;
    topmemory = mem_lmt * STD_MB;
    write_log("JVM need more Memory or Threads!");
  }

  comp_res = execute_cmd("/bin/grep 'Could not create' %s/error.out", work_dir);
  if (!comp_res) {
    ACflg = OJ_RE;
    write_log("jvm need more resource, tweak -Xmx(OJ_JAVA_BONUS) Settings");
  }
  return comp_res;
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

void get_sim(int solution_id, int lang, int pid, int &sim_s_id) {
  printf("Creating AC code solution id = %d, lang=%d, pid=%d \n", solution_id,
         lang, pid);
  char src_pth[BUFFER_SIZE];
  sprintf(src_pth, "Main.%s", lang_ext[lang]);
  execute_cmd("/bin/mkdir ../data/%d/ac/", pid);
  execute_cmd("/bin/cp %s ../data/%d/ac/%d.%s", src_pth, pid, solution_id,
              lang_ext[lang]);
}

int count_in_files(char *dirpath) {
  const char *cmd = "ls -l %s/*.in|wc -l";
  int ret = 0;
  FILE *fjobs = read_cmd_output(cmd, dirpath);
  fscanf(fjobs, "%d", &ret);
  pclose(fjobs);

  return ret;
}

