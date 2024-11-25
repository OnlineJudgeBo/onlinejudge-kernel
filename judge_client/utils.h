#include <stdio.h>

int data_list_has(char *file);

int data_list_add(char *file);

long get_file_size(const char *filename);

void find_next_nonspace(int &c1, int &c2, FILE *&f1, FILE *&f2, int &ret);

void make_diff_out(const char *file1, const char *file2, int c1, int c2,
                   const char *path);

char from_hex(char ch);

char to_hex(char code);


void copy_shell_runtime(char *work_dir);

void copy_python_runtime(char *work_dir);

void copy_python3_runtime(char *work_dir);

int fix_python_mis_judge(char *work_dir, int &ACflg, int &topmemory,
                         int mem_lmt);
int fix_java_mis_judge(char *work_dir, int &ACflg, int &topmemory,
                       int mem_lmt);

void clean_workdir(char *work_dir);
void get_sim(int solution_id, int lang, int pid, int &sim_s_id);
int count_in_files(char *dirpath);

void print_call_array();