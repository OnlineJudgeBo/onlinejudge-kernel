/*
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

//c & c++
int LANG_CV[512] = {0,1,2,3,5,8,9,10,11,12,13,14,16,20,21,39,56,59,63,89,99,158,186,218,231,234,257,262,268,275,292,302,318,334,511,267,
	SYS_write, SYS_mprotect, SYS_munmap, SYS_brk, SYS_arch_prctl, SYS_pread64, SYS_open, SYS_writev,
        SYS_time, SYS_futex, SYS_set_thread_area, SYS_access, SYS_clock_gettime, SYS_exit_group, SYS_mq_open,
        SYS_ioprio_get, SYS_unshare, SYS_set_robust_list, SYS_splice, SYS_close, SYS_stat, SYS_fstat, SYS_execve,
        SYS_uname, SYS_lseek, SYS_readlink, SYS_mmap, SYS_sysinfo, 0 };

//java
int LANG_JV[512] = {
        0, 262,318,334,435, SYS_mprotect, SYS_getuid, SYS_getgid, SYS_geteuid, SYS_getegid, SYS_munmap, SYS_getppid, SYS_getpgrp, 267,
        SYS_brk, SYS_rt_sigaction, SYS_rt_sigprocmask, SYS_prctl, SYS_arch_prctl, SYS_ioctl, SYS_pread64, SYS_open,
        SYS_futex, SYS_set_thread_area, SYS_access, SYS_getdents64, SYS_set_tid_address, SYS_pipe, SYS_exit_group,
        SYS_openat, SYS_set_robust_list, SYS_close, SYS_prlimit64, SYS_dup2, SYS_getpid, SYS_stat, SYS_fstat, SYS_clone,
        SYS_execve, SYS_lstat, SYS_wait4, SYS_uname, SYS_fcntl, SYS_getcwd, SYS_lseek, SYS_readlink, SYS_mmap,
        SYS_getrlimit, 0 };

//python
int LANG_PY3[512] = {
        0,1,2,3,4,5,6,8,9,10,11,12,13,14,16,17,21,32,39,41,42,49,59,72,78,79,89,97,99,102,104,106,107,108,131,137,158,186,202,217,218,228,231,257,262,273,302,318,334,
	511,
        SYS_write, SYS_mprotect, SYS_getuid, SYS_getgid, SYS_geteuid, SYS_getegid, SYS_munmap, SYS_brk,
        SYS_rt_sigaction, SYS_sigaltstack, SYS_rt_sigprocmask, SYS_sched_get_priority_max, SYS_arch_prctl, SYS_ioctl,
        SYS_pread64, SYS_getxattr, SYS_open, SYS_futex, SYS_access, SYS_getdents64, SYS_set_tid_address, SYS_clock_gettime,
        SYS_exit_group, SYS_mremap, SYS_openat, SYS_unshare, SYS_set_robust_list, SYS_close, SYS_prlimit64,
        SYS_dup, SYS_getpid, SYS_stat, SYS_socket, SYS_connect, SYS_fstat, SYS_execve, SYS_lstat,
        SYS_exit, SYS_fcntl, SYS_getdents, SYS_getcwd, SYS_lseek, SYS_readlink, SYS_mmap, SYS_getrlimit,
        SYS_sysinfo, 0 };

//python
int LANG_YV[512] = {
        0,1,2,3,4,5,6,8,9,10,11,12,13,14,16,17,21,32,39,41,42,49,59,72,78,79,89,97,99,102,104,106,107,108,131,137,158,186,202,217,218,228,231,257,262,273,302,318,334,
	511,
        SYS_write, SYS_mprotect, SYS_getuid, SYS_getgid, SYS_geteuid, SYS_getegid, SYS_munmap, SYS_brk,
        SYS_rt_sigaction, SYS_sigaltstack, SYS_rt_sigprocmask, SYS_sched_get_priority_max, SYS_arch_prctl, SYS_ioctl,
        SYS_pread64, SYS_getxattr, SYS_open, SYS_futex, SYS_access, SYS_getdents64, SYS_set_tid_address, SYS_clock_gettime,
        SYS_exit_group, SYS_mremap, SYS_openat, SYS_unshare, SYS_set_robust_list, SYS_close, SYS_prlimit64,
        SYS_dup, SYS_getpid, SYS_stat, SYS_socket, SYS_connect, SYS_fstat, SYS_execve, SYS_lstat,
        SYS_exit, SYS_fcntl, SYS_getdents, SYS_getcwd, SYS_lseek, SYS_readlink, SYS_mmap, SYS_getrlimit,
        SYS_sysinfo, 0 };
