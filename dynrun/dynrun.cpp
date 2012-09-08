/**********************************************************************
          DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
                    Version 2, December 2004

 Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>

 Everyone is permitted to copy and distribute verbatim or modified
 copies of this license document, and changing it is allowed as long
 as the name is changed.

            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

  0. You just DO WHAT THE FUCK YOU WANT TO.

**********************************************************************/

#include <iomanip>
#include <iostream>	          // std::cout
#include <getopt.h>	          // getopt()
#include <boost/foreach.hpp>  // FOREACH
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adj_list_serialize.hpp>
#include <boost/graph/graphviz.hpp>
#include "instruction/cfg.h"
#include "util.h"

#include <unistd.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/reg.h>

/*
 * When running 32bit binaries, we need to use
 * this struct (which is in sys/user.h as well, but
 * only gets enabled if __WORDSIZE==32, which isn't
 * the case on 64bit machines.
 */
struct user_regs_struct32
{
  long int ebx;
  long int ecx;
  long int edx;
  long int esi;
  long int edi;
  long int ebp;
  long int eax;
  long int xds;
  long int xes;
  long int xfs;
  long int xgs;
  long int orig_eax;
  long int eip;
  long int xcs;
  long int eflags;
  long int esp;
  long int xss;
};


struct user_regs_struct64
{
  unsigned long r15;
  unsigned long r14;
  unsigned long r13;
  unsigned long r12;
  unsigned long rbp;
  unsigned long rbx;
  unsigned long r11;
  unsigned long r10;
  unsigned long r9;
  unsigned long r8;
  unsigned long rax;
  unsigned long rcx;
  unsigned long rdx;
  unsigned long rsi;
  unsigned long rdi;
  unsigned long orig_rax;
  unsigned long rip;
  unsigned long cs;
  unsigned long eflags;
  unsigned long rsp;
  unsigned long ss;
  unsigned long fs_base;
  unsigned long gs_base;
  unsigned long ds;
  unsigned long es;
  unsigned long fs;
  unsigned long gs;
};


struct DynRunConfig : public Configuration
{
	std::string input_filename;
	std::string binary;

	DynRunConfig()
		: Configuration(), input_filename("output.cfg"), binary("input.bin")
	{ }
};

static DynRunConfig config;

static void
usage(char const *prog)
{
	std::cout << "\033[32mUsage:\033[0m" << std::endl << std::endl;
	std::cout << prog << " [-h] [-f <CFG file>] [-v] [-d] -- <binary> <arguments>"
	          << std::endl << std::endl << "\033[32mOptions\033[0m" << std::endl;
	std::cout << "\t-f <file>          Set input file [output.cfg]" << std::endl;
	std::cout << "\t-d                 Debug output [off]" << std::endl;
	std::cout << "\t-h                 Display help" << std::endl;
	std::cout << "\t-v                 Verbose output [off]" << std::endl;
}


static void
banner()
{
	Version version = Configuration::get()->globalProgramVersion;
	std::cout << "\033[34m" << "********************************************"
	          << "\033[0m" << std::endl;
	std::cout << "\033[33m" << "        DynRun version " << version.major
	          << "." << version.minor << "\033[0m" << std::endl;
	std::cout << "\033[34m" << "********************************************"
	          << "\033[0m" << std::endl;
}


static int
parseInputFromOptions(int argc, char **argv)
{
	int opt;

	while ((opt = getopt(argc, argv, "b:df:hv")) != -1) {

		if (config.parse_option(opt))
			continue;

		switch(opt) {

			case 'b':
				config.binary = optarg;
				break;

			case 'f':
				config.input_filename = optarg;
				break;

			case 'h':
				usage(argv[0]);
				return -1;
		}
		printf("%d\n", optind);
	}
	return optind;
}


static void
readCFG(ControlFlowGraph& cfg)
{
	try {
		CFGFromFile(cfg, config.input_filename);
	} catch (FileNotFoundException fne) {
		std::cout << "\033[31m" << fne.message << " not found.\033[0m" << std::endl;
		return;
	} catch (boost::archive::archive_exception ae) {
		std::cout << "\033[31marchive exception:\033[0m " << ae.what() << std::endl;
		return;
	}
}


static std::map<int,std::string> syscallnames32;
static std::map<int,std::string> syscallnames64;

static void init32()
{
    syscallnames32[0]="restart_syscall";
    syscallnames32[1]="exit";
    syscallnames32[2]="fork";
    syscallnames32[3]="read";
    syscallnames32[4]="write";
    syscallnames32[5]="open";
    syscallnames32[6]="close";
    syscallnames32[7]="waitpid";
    syscallnames32[8]="creat";
    syscallnames32[9]="link";
    syscallnames32[10]="unlink";
    syscallnames32[11]="execve";
    syscallnames32[12]="chdir";
    syscallnames32[13]="time";
    syscallnames32[14]="mknod";
    syscallnames32[15]="chmod";
    syscallnames32[16]="lchown";
    syscallnames32[17]="break";
    syscallnames32[18]="oldstat";
    syscallnames32[19]="lseek";
    syscallnames32[20]="getpid";
    syscallnames32[21]="mount";
    syscallnames32[22]="umount";
    syscallnames32[23]="setuid";
    syscallnames32[24]="getuid";
    syscallnames32[25]="stime";
    syscallnames32[26]="ptrace";
    syscallnames32[27]="alarm";
    syscallnames32[28]="oldfstat";
    syscallnames32[29]="pause";
    syscallnames32[30]="utime";
    syscallnames32[31]="stty";
    syscallnames32[32]="gtty";
    syscallnames32[33]="access";
    syscallnames32[34]="nice";
    syscallnames32[35]="ftime";
    syscallnames32[36]="sync";
    syscallnames32[37]="kill";
    syscallnames32[38]="rename";
    syscallnames32[39]="mkdir";
    syscallnames32[40]="rmdir";
    syscallnames32[41]="dup";
    syscallnames32[42]="pipe";
    syscallnames32[43]="times";
    syscallnames32[44]="prof";
    syscallnames32[45]="brk";
    syscallnames32[46]="setgid";
    syscallnames32[47]="getgid";
    syscallnames32[48]="signal";
    syscallnames32[49]="geteuid";
    syscallnames32[50]="getegid";
    syscallnames32[51]="acct";
    syscallnames32[52]="umount2";
    syscallnames32[53]="lock";
    syscallnames32[54]="ioctl";
    syscallnames32[55]="fcntl";
    syscallnames32[56]="mpx";
    syscallnames32[57]="setpgid";
    syscallnames32[58]="ulimit";
    syscallnames32[59]="oldolduname";
    syscallnames32[60]="umask";
    syscallnames32[61]="chroot";
    syscallnames32[62]="ustat";
    syscallnames32[63]="dup2";
    syscallnames32[64]="getppid";
    syscallnames32[65]="getpgrp";
    syscallnames32[66]="setsid";
    syscallnames32[67]="sigaction";
    syscallnames32[68]="sgetmask";
    syscallnames32[69]="ssetmask";
    syscallnames32[70]="setreuid";
    syscallnames32[71]="setregid";
    syscallnames32[72]="sigsuspend";
    syscallnames32[73]="sigpending";
    syscallnames32[74]="sethostname";
    syscallnames32[75]="setrlimit";
    syscallnames32[76]="getrlimit";
    syscallnames32[77]="getrusage";
    syscallnames32[78]="gettimeofday";
    syscallnames32[79]="settimeofday";
    syscallnames32[80]="getgroups";
    syscallnames32[81]="setgroups";
    syscallnames32[82]="select";
    syscallnames32[83]="symlink";
    syscallnames32[84]="oldlstat";
    syscallnames32[85]="readlink";
    syscallnames32[86]="uselib";
    syscallnames32[87]="swapon";
    syscallnames32[88]="reboot";
    syscallnames32[89]="readdir";
    syscallnames32[90]="mmap";
    syscallnames32[91]="munmap";
    syscallnames32[92]="truncate";
    syscallnames32[93]="ftruncate";
    syscallnames32[94]="fchmod";
    syscallnames32[95]="fchown";
    syscallnames32[96]="getpriority";
    syscallnames32[97]="setpriority";
    syscallnames32[98]="profil";
    syscallnames32[99]="statfs";
    syscallnames32[100]="fstatfs";
    syscallnames32[101]="ioperm";
    syscallnames32[102]="socketcall";
    syscallnames32[103]="syslog";
    syscallnames32[104]="setitimer";
    syscallnames32[105]="getitimer";
    syscallnames32[106]="stat";
    syscallnames32[107]="lstat";
    syscallnames32[108]="fstat";
    syscallnames32[109]="olduname";
    syscallnames32[110]="iopl";
    syscallnames32[111]="vhangup";
    syscallnames32[112]="idle";
    syscallnames32[113]="vm86old";
    syscallnames32[114]="wait4";
    syscallnames32[115]="swapoff";
    syscallnames32[116]="sysinfo";
    syscallnames32[117]="ipc";
    syscallnames32[118]="fsync";
    syscallnames32[119]="sigreturn";
    syscallnames32[120]="clone";
    syscallnames32[121]="setdomainname";
    syscallnames32[122]="uname";
    syscallnames32[123]="modify_ldt";
    syscallnames32[124]="adjtimex";
    syscallnames32[125]="mprotect";
    syscallnames32[126]="sigprocmask";
    syscallnames32[127]="create_module";
    syscallnames32[128]="init_module";
    syscallnames32[129]="delete_module";
    syscallnames32[130]="get_kernel_syms";
    syscallnames32[131]="quotactl";
    syscallnames32[132]="getpgid";
    syscallnames32[133]="fchdir";
    syscallnames32[134]="bdflush";
    syscallnames32[135]="sysfs";
    syscallnames32[136]="personality";
    syscallnames32[137]="afs_syscall";
    syscallnames32[138]="setfsuid";
    syscallnames32[139]="setfsgid";
    syscallnames32[140]="_llseek";
    syscallnames32[141]="getdents";
    syscallnames32[142]="_newselect";
    syscallnames32[143]="flock";
    syscallnames32[144]="msync";
    syscallnames32[145]="readv";
    syscallnames32[146]="writev";
    syscallnames32[147]="getsid";
    syscallnames32[148]="fdatasync";
    syscallnames32[149]="_sysctl";
    syscallnames32[150]="mlock";
    syscallnames32[151]="munlock";
    syscallnames32[152]="mlockall";
    syscallnames32[153]="munlockall";
    syscallnames32[154]="sched_setparam";
    syscallnames32[155]="sched_getparam";
    syscallnames32[156]="sched_setscheduler";
    syscallnames32[157]="sched_getscheduler";
    syscallnames32[158]="sched_yield";
    syscallnames32[159]="sched_get_priority_max";
    syscallnames32[160]="sched_get_priority_min";
    syscallnames32[161]="sched_rr_get_interval";
    syscallnames32[162]="nanosleep";
    syscallnames32[163]="mremap";
    syscallnames32[164]="setresuid";
    syscallnames32[165]="getresuid";
    syscallnames32[166]="vm86";
    syscallnames32[167]="query_module";
    syscallnames32[168]="poll";
    syscallnames32[169]="nfsservctl";
    syscallnames32[170]="setresgid";
    syscallnames32[171]="getresgid";
    syscallnames32[172]="prctl";
    syscallnames32[173]="rt_sigreturn";
    syscallnames32[174]="rt_sigaction";
    syscallnames32[175]="rt_sigprocmask";
    syscallnames32[176]="rt_sigpending";
    syscallnames32[177]="rt_sigtimedwait";
    syscallnames32[178]="rt_sigqueueinfo";
    syscallnames32[179]="rt_sigsuspend";
    syscallnames32[180]="pread64";
    syscallnames32[181]="pwrite64";
    syscallnames32[182]="chown";
    syscallnames32[183]="getcwd";
    syscallnames32[184]="capget";
    syscallnames32[185]="capset";
    syscallnames32[186]="sigaltstack";
    syscallnames32[187]="sendfile";
    syscallnames32[188]="getpmsg";
    syscallnames32[189]="putpmsg";
    syscallnames32[190]="vfork";
    syscallnames32[191]="ugetrlimit";
    syscallnames32[192]="mmap2";
    syscallnames32[193]="truncate64";
    syscallnames32[194]="ftruncate64";
    syscallnames32[195]="stat64";
    syscallnames32[196]="lstat64";
    syscallnames32[197]="fstat64";
    syscallnames32[198]="lchown32";
    syscallnames32[199]="getuid32";
    syscallnames32[200]="getgid32";
    syscallnames32[201]="geteuid32";
    syscallnames32[202]="getegid32";
    syscallnames32[203]="setreuid32";
    syscallnames32[204]="setregid32";
    syscallnames32[205]="getgroups32";
    syscallnames32[206]="setgroups32";
    syscallnames32[207]="fchown32";
    syscallnames32[208]="setresuid32";
    syscallnames32[209]="getresuid32";
    syscallnames32[210]="setresgid32";
    syscallnames32[211]="getresgid32";
    syscallnames32[212]="chown32";
    syscallnames32[213]="setuid32";
    syscallnames32[214]="setgid32";
    syscallnames32[215]="setfsuid32";
    syscallnames32[216]="setfsgid32";
    syscallnames32[217]="pivot_root";
    syscallnames32[218]="mincore";
    syscallnames32[219]="madvise";
    syscallnames32[219]="madvise1";
    syscallnames32[220]="getdents64";
    syscallnames32[221]="fcntl64";
    syscallnames32[224]="gettid";
    syscallnames32[225]="readahead";
    syscallnames32[226]="setxattr";
    syscallnames32[227]="lsetxattr";
    syscallnames32[228]="fsetxattr";
    syscallnames32[229]="getxattr";
    syscallnames32[230]="lgetxattr";
    syscallnames32[231]="fgetxattr";
    syscallnames32[232]="listxattr";
    syscallnames32[233]="llistxattr";
    syscallnames32[234]="flistxattr";
    syscallnames32[235]="removexattr";
    syscallnames32[236]="lremovexattr";
    syscallnames32[237]="fremovexattr";
    syscallnames32[238]="tkill";
    syscallnames32[239]="sendfile64";
    syscallnames32[240]="futex";
    syscallnames32[241]="sched_setaffinity";
    syscallnames32[242]="sched_getaffinity";
    syscallnames32[243]="set_thread_area";
    syscallnames32[244]="get_thread_area";
    syscallnames32[245]="io_setup";
    syscallnames32[246]="io_destroy";
    syscallnames32[247]="io_getevents";
    syscallnames32[248]="io_submit";
    syscallnames32[249]="io_cancel";
    syscallnames32[250]="fadvise64";
    syscallnames32[252]="exit_group";
    syscallnames32[253]="lookup_dcookie";
    syscallnames32[254]="epoll_create";
    syscallnames32[255]="epoll_ctl";
    syscallnames32[256]="epoll_wait";
    syscallnames32[257]="remap_file_pages";
    syscallnames32[258]="set_tid_address";
    syscallnames32[259]="timer_create";
    syscallnames32[260]="timer_settime";
    syscallnames32[261]="timer_gettime";
    syscallnames32[262]="timer_getoverrun";
    syscallnames32[263]="timer_delete";
    syscallnames32[264]="clock_settime";
    syscallnames32[265]="clock_gettime";
    syscallnames32[266]="clock_getres";
    syscallnames32[267]="clock_nanosleep";
    syscallnames32[268]="statfs64";
    syscallnames32[269]="fstatfs64";
    syscallnames32[270]="tgkill";
    syscallnames32[271]="utimes";
    syscallnames32[272]="fadvise64_64";
    syscallnames32[273]="vserver";
    syscallnames32[274]="mbind";
    syscallnames32[275]="get_mempolicy";
    syscallnames32[276]="set_mempolicy";
    syscallnames32[277]="mq_open";
    syscallnames32[278]="mq_unlink";
    syscallnames32[279]="mq_timedsend";
    syscallnames32[280]="mq_timedreceive";
    syscallnames32[281]="mq_notify";
    syscallnames32[282]="mq_getsetattr";
    syscallnames32[283]="kexec_load";
    syscallnames32[284]="waitid";
    syscallnames32[285]="sys_setaltroot";
    syscallnames32[286]="add_key";
    syscallnames32[287]="request_key";
    syscallnames32[288]="keyctl";
    syscallnames32[289]="ioprio_set";
    syscallnames32[290]="ioprio_get";
    syscallnames32[291]="inotify_init";
    syscallnames32[292]="inotify_add_watch";
    syscallnames32[293]="inotify_rm_watch";
    syscallnames32[294]="migrate_pages";
    syscallnames32[295]="openat";
    syscallnames32[296]="mkdirat";
    syscallnames32[297]="mknodat";
    syscallnames32[298]="fchownat";
    syscallnames32[299]="futimesat";
    syscallnames32[300]="fstatat64";
    syscallnames32[301]="unlinkat";
    syscallnames32[302]="renameat";
    syscallnames32[303]="linkat";
    syscallnames32[304]="symlinkat";
    syscallnames32[305]="readlinkat";
    syscallnames32[306]="fchmodat";
    syscallnames32[307]="faccessat";
    syscallnames32[308]="pselect6";
    syscallnames32[309]="ppoll";
    syscallnames32[310]="unshare";
    syscallnames32[311]="set_robust_list";
    syscallnames32[312]="get_robust_list";
    syscallnames32[313]="splice";
    syscallnames32[314]="sync_file_range";
    syscallnames32[315]="tee";
    syscallnames32[316]="vmsplice";
    syscallnames32[317]="move_pages";
    syscallnames32[318]="getcpu";
    syscallnames32[319]="epoll_pwait";
    syscallnames32[320]="utimensat";
    syscallnames32[321]="signalfd";
    syscallnames32[322]="timerfd_create";
    syscallnames32[323]="eventfd";
    syscallnames32[324]="fallocate";
    syscallnames32[325]="timerfd_settime";
    syscallnames32[326]="timerfd_gettime";
    syscallnames32[327]="signalfd4";
    syscallnames32[328]="eventfd2";
    syscallnames32[329]="epoll_create1";
    syscallnames32[330]="dup3";
    syscallnames32[331]="pipe2";
    syscallnames32[332]="inotify_init1";
    syscallnames32[333]="preadv";
    syscallnames32[334]="pwritev";
    syscallnames32[335]="rt_tgsigqueueinfo";
    syscallnames32[336]="perf_event_open";
    syscallnames32[337]="recvmmsg";
    syscallnames32[338]="fanotify_init";
    syscallnames32[339]="fanotify_mark";
    syscallnames32[340]="prlimit64";
    syscallnames32[341]="name_to_handle_at";
    syscallnames32[342]="open_by_handle_at";
    syscallnames32[343]="clock_adjtime";
    syscallnames32[344]="syncfs";
    syscallnames32[345]="sendmmsg";
    syscallnames32[346]="setns";
    syscallnames32[347]="process_vm_readv";
    syscallnames32[348]="process_vm_writev";
}


static void init64()
{
	syscallnames64[0]="read";
	syscallnames64[1]="write";
	syscallnames64[2]="open";
	syscallnames64[3]="close";
	syscallnames64[4]="stat";
	syscallnames64[5]="fstat";
	syscallnames64[6]="lstat";
	syscallnames64[7]="poll";
	syscallnames64[8]="lseek";
	syscallnames64[9]="mmap";
	syscallnames64[10]="mprotect";
	syscallnames64[11]="munmap";
	syscallnames64[12]="brk";
	syscallnames64[13]="rt_sigaction";
	syscallnames64[14]="rt_sigprocmask";
	syscallnames64[15]="rt_sigreturn";
	syscallnames64[16]="ioctl";
	syscallnames64[17]="pread64";
	syscallnames64[18]="pwrite64";
	syscallnames64[19]="readv";
	syscallnames64[20]="writev";
	syscallnames64[21]="access";
	syscallnames64[22]="pipe";
	syscallnames64[23]="select";
	syscallnames64[24]="sched_yield";
	syscallnames64[25]="mremap";
	syscallnames64[26]="msync";
	syscallnames64[27]="mincore";
	syscallnames64[28]="madvise";
	syscallnames64[29]="shmget";
	syscallnames64[30]="shmat";
	syscallnames64[31]="shmctl";
	syscallnames64[32]="dup";
	syscallnames64[33]="dup2";
	syscallnames64[34]="pause";
	syscallnames64[35]="nanosleep";
	syscallnames64[36]="getitimer";
	syscallnames64[37]="alarm";
	syscallnames64[38]="setitimer";
	syscallnames64[39]="getpid";
	syscallnames64[40]="sendfile";
	syscallnames64[41]="socket";
	syscallnames64[42]="connect";
	syscallnames64[43]="accept";
	syscallnames64[44]="sendto";
	syscallnames64[45]="recvfrom";
	syscallnames64[46]="sendmsg";
	syscallnames64[47]="recvmsg";
	syscallnames64[48]="shutdown";
	syscallnames64[49]="bind";
	syscallnames64[50]="listen";
	syscallnames64[51]="getsockname";
	syscallnames64[52]="getpeername";
	syscallnames64[53]="socketpair";
	syscallnames64[54]="setsockopt";
	syscallnames64[55]="getsockopt";
	syscallnames64[56]="clone";
	syscallnames64[57]="fork";
	syscallnames64[58]="vfork";
	syscallnames64[59]="execve";
	syscallnames64[60]="exit";
	syscallnames64[61]="wait4";
	syscallnames64[62]="kill";
	syscallnames64[63]="uname";
	syscallnames64[64]="semget";
	syscallnames64[65]="semop";
	syscallnames64[66]="semctl";
	syscallnames64[67]="shmdt";
	syscallnames64[68]="msgget";
	syscallnames64[69]="msgsnd";
	syscallnames64[70]="msgrcv";
	syscallnames64[71]="msgctl";
	syscallnames64[72]="fcntl";
	syscallnames64[73]="flock";
	syscallnames64[74]="fsync";
	syscallnames64[75]="fdatasync";
	syscallnames64[76]="truncate";
	syscallnames64[77]="ftruncate";
	syscallnames64[78]="getdents";
	syscallnames64[79]="getcwd";
	syscallnames64[80]="chdir";
	syscallnames64[81]="fchdir";
	syscallnames64[82]="rename";
	syscallnames64[83]="mkdir";
	syscallnames64[84]="rmdir";
	syscallnames64[85]="creat";
	syscallnames64[86]="link";
	syscallnames64[87]="unlink";
	syscallnames64[88]="symlink";
	syscallnames64[89]="readlink";
	syscallnames64[90]="chmod";
	syscallnames64[91]="fchmod";
	syscallnames64[92]="chown";
	syscallnames64[93]="fchown";
	syscallnames64[94]="lchown";
	syscallnames64[95]="umask";
	syscallnames64[96]="gettimeofday";
	syscallnames64[97]="getrlimit";
	syscallnames64[98]="getrusage";
	syscallnames64[99]="sysinfo";
	syscallnames64[100]="times";
	syscallnames64[101]="ptrace";
	syscallnames64[102]="getuid";
	syscallnames64[103]="syslog";
	syscallnames64[104]="getgid";
	syscallnames64[105]="setuid";
	syscallnames64[106]="setgid";
	syscallnames64[107]="geteuid";
	syscallnames64[108]="getegid";
	syscallnames64[109]="setpgid";
	syscallnames64[110]="getppid";
	syscallnames64[111]="getpgrp";
	syscallnames64[112]="setsid";
	syscallnames64[113]="setreuid";
	syscallnames64[114]="setregid";
	syscallnames64[115]="getgroups";
	syscallnames64[116]="setgroups";
	syscallnames64[117]="setresuid";
	syscallnames64[118]="getresuid";
	syscallnames64[119]="setresgid";
	syscallnames64[120]="getresgid";
	syscallnames64[121]="getpgid";
	syscallnames64[122]="setfsuid";
	syscallnames64[123]="setfsgid";
	syscallnames64[124]="getsid";
	syscallnames64[125]="capget";
	syscallnames64[126]="capset";
	syscallnames64[127]="rt_sigpending";
	syscallnames64[128]="rt_sigtimedwait";
	syscallnames64[129]="rt_sigqueueinfo";
	syscallnames64[130]="rt_sigsuspend";
	syscallnames64[131]="sigaltstack";
	syscallnames64[132]="utime";
	syscallnames64[133]="mknod";
	syscallnames64[134]="uselib";
	syscallnames64[135]="personality";
	syscallnames64[136]="ustat";
	syscallnames64[137]="statfs";
	syscallnames64[138]="fstatfs";
	syscallnames64[139]="sysfs";
	syscallnames64[140]="getpriority";
	syscallnames64[141]="setpriority";
	syscallnames64[142]="sched_setparam";
	syscallnames64[143]="sched_getparam";
	syscallnames64[144]="sched_setscheduler";
	syscallnames64[145]="sched_getscheduler";
	syscallnames64[146]="sched_get_priority_max";
	syscallnames64[147]="sched_get_priority_min";
	syscallnames64[148]="sched_rr_get_interval";
	syscallnames64[149]="mlock";
	syscallnames64[150]="munlock";
	syscallnames64[151]="mlockall";
	syscallnames64[152]="munlockall";
	syscallnames64[153]="vhangup";
	syscallnames64[154]="modify_ldt";
	syscallnames64[155]="pivot_root";
	syscallnames64[156]="_sysctl";
	syscallnames64[157]="prctl";
	syscallnames64[158]="arch_prctl";
	syscallnames64[159]="adjtimex";
	syscallnames64[160]="setrlimit";
	syscallnames64[161]="chroot";
	syscallnames64[162]="sync";
	syscallnames64[163]="acct";
	syscallnames64[164]="settimeofday";
	syscallnames64[165]="mount";
	syscallnames64[166]="umount2";
	syscallnames64[167]="swapon";
	syscallnames64[168]="swapoff";
	syscallnames64[169]="reboot";
	syscallnames64[170]="sethostname";
	syscallnames64[171]="setdomainname";
	syscallnames64[172]="iopl";
	syscallnames64[173]="ioperm";
	syscallnames64[174]="create_module";
	syscallnames64[175]="init_module";
	syscallnames64[176]="delete_module";
	syscallnames64[177]="get_kernel_syms";
	syscallnames64[178]="query_module";
	syscallnames64[179]="quotactl";
	syscallnames64[180]="nfsservctl";
	syscallnames64[181]="getpmsg";
	syscallnames64[182]="putpmsg";
	syscallnames64[183]="afs_syscall";
	syscallnames64[184]="tuxcall";
	syscallnames64[185]="security";
	syscallnames64[186]="gettid";
	syscallnames64[187]="readahead";
	syscallnames64[188]="setxattr";
	syscallnames64[189]="lsetxattr";
	syscallnames64[190]="fsetxattr";
	syscallnames64[191]="getxattr";
	syscallnames64[192]="lgetxattr";
	syscallnames64[193]="fgetxattr";
	syscallnames64[194]="listxattr";
	syscallnames64[195]="llistxattr";
	syscallnames64[196]="flistxattr";
	syscallnames64[197]="removexattr";
	syscallnames64[198]="lremovexattr";
	syscallnames64[199]="fremovexattr";
	syscallnames64[200]="tkill";
	syscallnames64[201]="time";
	syscallnames64[202]="futex";
	syscallnames64[203]="sched_setaffinity";
	syscallnames64[204]="sched_getaffinity";
	syscallnames64[205]="set_thread_area";
	syscallnames64[206]="io_setup";
	syscallnames64[207]="io_destroy";
	syscallnames64[208]="io_getevents";
	syscallnames64[209]="io_submit";
	syscallnames64[210]="io_cancel";
	syscallnames64[211]="get_thread_area";
	syscallnames64[212]="lookup_dcookie";
	syscallnames64[213]="epoll_create";
	syscallnames64[214]="epoll_ctl_old";
	syscallnames64[215]="epoll_wait_old";
	syscallnames64[216]="remap_file_pages";
	syscallnames64[217]="getdents64";
	syscallnames64[218]="set_tid_address";
	syscallnames64[219]="restart_syscall";
	syscallnames64[220]="semtimedop";
	syscallnames64[221]="fadvise64";
	syscallnames64[222]="timer_create";
	syscallnames64[223]="timer_settime";
	syscallnames64[224]="timer_gettime";
	syscallnames64[225]="timer_getoverrun";
	syscallnames64[226]="timer_delete";
	syscallnames64[227]="clock_settime";
	syscallnames64[228]="clock_gettime";
	syscallnames64[229]="clock_getres";
	syscallnames64[230]="clock_nanosleep";
	syscallnames64[231]="exit_group";
	syscallnames64[232]="epoll_wait";
	syscallnames64[233]="epoll_ctl";
	syscallnames64[234]="tgkill";
	syscallnames64[235]="utimes";
	syscallnames64[236]="vserver";
	syscallnames64[237]="mbind";
	syscallnames64[238]="set_mempolicy";
	syscallnames64[239]="get_mempolicy";
	syscallnames64[240]="mq_open";
	syscallnames64[241]="mq_unlink";
	syscallnames64[242]="mq_timedsend";
	syscallnames64[243]="mq_timedreceive";
	syscallnames64[244]="mq_notify";
	syscallnames64[245]="mq_getsetattr";
	syscallnames64[246]="kexec_load";
	syscallnames64[247]="waitid";
	syscallnames64[248]="add_key";
	syscallnames64[249]="request_key";
	syscallnames64[250]="keyctl";
	syscallnames64[251]="ioprio_set";
	syscallnames64[252]="ioprio_get";
	syscallnames64[253]="inotify_init";
	syscallnames64[254]="inotify_add_watch";
	syscallnames64[255]="inotify_rm_watch";
	syscallnames64[256]="migrate_pages";
	syscallnames64[257]="openat";
	syscallnames64[258]="mkdirat";
	syscallnames64[259]="mknodat";
	syscallnames64[260]="fchownat";
	syscallnames64[261]="futimesat";
	syscallnames64[262]="newfstatat";
	syscallnames64[263]="unlinkat";
	syscallnames64[264]="renameat";
	syscallnames64[265]="linkat";
	syscallnames64[266]="symlinkat";
	syscallnames64[267]="readlinkat";
	syscallnames64[268]="fchmodat";
	syscallnames64[269]="faccessat";
	syscallnames64[270]="pselect6";
	syscallnames64[271]="ppoll";
	syscallnames64[272]="unshare";
	syscallnames64[273]="set_robust_list";
	syscallnames64[274]="get_robust_list";
	syscallnames64[275]="splice";
	syscallnames64[276]="tee";
	syscallnames64[277]="sync_file_range";
	syscallnames64[278]="vmsplice";
	syscallnames64[279]="move_pages";
	syscallnames64[280]="utimensat";
	syscallnames64[281]="epoll_pwait";
	syscallnames64[282]="signalfd";
	syscallnames64[283]="timerfd_create";
	syscallnames64[284]="eventfd";
	syscallnames64[285]="fallocate";
	syscallnames64[286]="timerfd_settime";
	syscallnames64[287]="timerfd_gettime";
	syscallnames64[288]="accept4";
	syscallnames64[289]="signalfd4";
	syscallnames64[290]="eventfd2";
	syscallnames64[291]="epoll_create1";
	syscallnames64[292]="dup3";
	syscallnames64[293]="pipe2";
	syscallnames64[294]="inotify_init1";
	syscallnames64[295]="preadv";
	syscallnames64[296]="pwritev";
	syscallnames64[297]="rt_tgsigqueueinfo";
	syscallnames64[298]="perf_event_open";
	syscallnames64[299]="recvmmsg";
	syscallnames64[300]="fanotify_init";
	syscallnames64[301]="fanotify_mark";
	syscallnames64[302]="prlimit64";
	syscallnames64[303]="name_to_handle_at";
	syscallnames64[304]="open_by_handle_at";
	syscallnames64[305]="clock_adjtime";
	syscallnames64[306]="syncfs";
	syscallnames64[307]="sendmmsg";
	syscallnames64[308]="setns";
	syscallnames64[309]="getcpu";
	syscallnames64[310]="process_vm_readv";
	syscallnames64[311]="process_vm_writev";
}


std::string syscall2Name(int syscall, int mode)
{
	static bool init = false;
	if (!init) {
		init32();
		init64();
	}

	switch(mode) {
		case 32:
			return syscallnames32[syscall];
		case 64:
			return syscallnames64[syscall];
		default:
			return "???";
	}
}


int getUnresolvedAddresses(ControlFlowGraph const &cfg, std::list<Address>& unresolved)
{
	int count = 0;
	CFGVertexIterator v, v_end;

	for (boost::tie(v, v_end) = boost::vertices(cfg); v != v_end; ++v)
	{
		//std::cout << *v;
		CFGNodeInfo const& n = cfg[*v];
		switch(n.bb->branchType) {
			case Instruction::BT_CALL_RESOLVE:
				//std::cout << " UNRES " << std::hex << n.bb->lastInstruction().v;
				unresolved.push_back(n.bb->lastInstruction());
				count++;
				break;
			default:
				break;
		}
		//std::cout << std::endl;
	}

	return count;
}


class PTracer
{
	/**
	 * @brief Child we are tracing
	 **/
	pid_t    _child;
	unsigned _emulationMode;
	bool     _inSyscall;
	unsigned _curSyscall;

	std::map<Address, unsigned> _breakpointData;

	/**
	 * @brief Run ptrace() with arguments and print error if necessary
	 *
	 * @param req ...
	 * @param chld ...
	 * @param addr ...
	 * @param data ...
	 * @return int
	 **/
	int ptrace_checked(enum __ptrace_request req, pid_t chld, unsigned long addr, void *data)
	{
		int r = ptrace(req, chld, addr, data);
		//DEBUG(std::cout << "ptraced: " << r << std::endl;);

		if (r and errno) {
			perror("ptrace");
		}

		return r;
	}

	/**
	 * @brief Wait for child
	 *
	 * @param chld ...
	 * @param status ...
	 * @param signal ...
	 * @return :WaitRet
	 **/
	int wait_checked(pid_t chld, int *status)
	{
		int res = waitpid(chld, status, __WALL);
		//DEBUG(std::cout << "WAIT(): res " << res << ", status " << *status << std::endl;);
		if (res == -1) {
			perror("waitpid");
		}
		return res;
	}

#if __WORDSIZE == 64
	void dumpReg(unsigned long reg)
	{
		std::cout << std::setw(16) << std::setfill('0') << std::hex << reg;
	}
#else
	void dumpReg(unsigned reg)
	{
		std::cout << std::setw(8) << std::setfill('0') << std::hex << reg;
	}
#endif

	void dumpRegs(void* arg)
	{
		std::cout << "----------------------------------------------------------------------" << std::endl;
		std::cout << "REGS" << std::endl;
#if __WORDSIZE == 64
		struct user_regs_struct64* regs = reinterpret_cast<struct user_regs_struct64*>(arg);
		std::cout << "R15 "; dumpReg(regs->r15);
		std::cout << " R14 "; dumpReg(regs->r14);
		std::cout << " R13 "; dumpReg(regs->r13);
		std::cout << " R12 "; dumpReg(regs->r12); std::cout << std::endl;
		std::cout << "R11 "; dumpReg(regs->r11);
		std::cout << " R10 "; dumpReg(regs->r10);
		std::cout << " R09 "; dumpReg(regs->r9);
		std::cout << " R08 "; dumpReg(regs->r8); std::cout << std::endl;
		std::cout << "RAX "; dumpReg(regs->rax);
		std::cout << " RBX "; dumpReg(regs->rbx);
		std::cout << " RCX "; dumpReg(regs->rcx);
		std::cout << " RDX "; dumpReg(regs->rdx); std::cout << std::endl;
		std::cout << "RSP "; dumpReg(regs->rsp);
		std::cout << " RBP "; dumpReg(regs->rbp);
		std::cout << " RIP "; dumpReg(regs->rip);
		std::cout << " FLG "; dumpReg(regs->eflags); std::cout << std::endl;
		std::cout << "ORA "; dumpReg(regs->orig_rax); std::cout << std::endl;
#else
		struct user_regs_struct32* regs = reinterpret_cast<struct user_regs_struct32*>(arg);
		std::cout << "EAX "; dumpReg(regs->eax);
		std::cout << " EBX "; dumpReg(regs->ebx);
		std::cout << " ECX "; dumpReg(regs->ecx);
		std::cout << " EDX "; dumpReg(regs->edx); std::cout << std::endl;
		std::cout << "ESI "; dumpReg(regs->esi);
		std::cout << " EDI "; dumpReg(regs->edi);
		std::cout << " EBP "; dumpReg(regs->ebp);
		std::cout << " ESP "; dumpReg(regs->esp); std::cout << std::endl;
		std::cout << "FLG "; dumpReg(regs->eflags);
		std::cout << " EIP "; dumpReg(regs->eip);
		std::cout << " ORA "; dumpReg(regs->orig_eax); std::cout << std::endl;
#endif
		std::cout << "----------------------------------------------------------------------" << std::endl;
	}


	/**
	 * @brief Perform system call handling
	 *
	 * @return int
	 **/
	int handleSyscall()
	{
		struct user_regs_struct data;

		ptrace_checked(PTRACE_GETREGS, _child, 0, &data);
		if (Configuration::get()->debug) {
			//dumpRegs(&data);
		}

		if (!_inSyscall) {
#if __WORDSIZE == 64
			_curSyscall = data.orig_rax;
#else
			_curSyscall = data.orig_eax;
#endif
			std::cout << "   System call: " << std::dec << _curSyscall
					  << " \033[33m(" << syscall2Name(_curSyscall, _emulationMode)
					  << ")\033[0m" << std::endl;
			_inSyscall = true;
		} else {
#if __WORDSIZE == 64
			int sysret = data.rax;
#else
			int sysret = data.eax;
#endif
			std::cout << "   System call return: 0x" << std::hex
			          << sysret << std::endl;
			_inSyscall = false;

#if __WORDSIZE == 64
			if (_curSyscall == 59) { // == SYS_execve on 64bit
				_emulationMode = 32;
				_curSyscall    = 11; // == SYS_execve on 32bit
				_inSyscall     = true;
			}
#endif
		}

		return 0;
	}

public:
	PTracer(pid_t child)
		: _child(child), _emulationMode(__WORDSIZE),
		  _inSyscall(false), _curSyscall(~0U),
		  _breakpointData()
	{ }

	/**
	 * @brief Perform initial handshake with child
	 *
	 * e.g., wait for the child to raise a SIGSTOP after
	 * calling ptrace(TRACEME...)
	 *
	 * @return bool
	 **/
	bool handshake()
	{
		int status;
		int r = wait_checked(_child, &status);
		if (r == _child) {
			return WIFSTOPPED(status) and WSTOPSIG(status) == SIGSTOP;
		}
		return false;
	}

	/**
	 * @brief Continue child execution
	 *
	 * @return void
	 **/
	void continueChild()
	{
		ptrace_checked(PTRACE_SYSCALL, _child, 0, 0);
	}

	/**
	 * @brief Run child and monitor ptrace signals
	 *
	 * @return void
	 **/
	void run()
	{
		int status = 0;
		while (1) {
			int r = wait_checked(_child, &status);

			if (r == -1) {
				return;
			}

			if (WIFSTOPPED(status)) {
				DEBUG(std::cout << "Stopped with signal " << WSTOPSIG(status) << std::endl;);
				if (WSTOPSIG(status) == SIGTRAP) {
					if (handleSyscall())
						return;
				}
			}

			if (WIFEXITED(status)) {
				DEBUG(std::cout << "Terminated with status " << WEXITSTATUS(status) << std::endl;);
				return;
			}

			continueChild();
		}
	}

	void addBreakpoint(Address a)
	{
		unsigned data = ptrace_checked(PTRACE_PEEKTEXT, _child, a.v, 0);
		DEBUG(std::cout << "BP @ " << std::hex << a.v << " data: " << std::hex << data << std::endl;);	
	}
};


static void
runInstrumented(std::list<Address> iPoints, int argc, char** argv)
{
	pid_t child = fork();
	if (child == 0) {

		/*
		 * Child's part of the ptrace handshake
		 */
		ptrace(PTRACE_TRACEME, 0, 0, 0);
		raise(SIGSTOP);

		for (int i = 0; i < argc; ++i)
			std::cout << argv[i] << " ";
		std::cout << std::endl;

		/*
		 * Execute the binary including parameters and
		 * inheriting parent's environment
		 */
		execve(argv[0], argv, environ);
		perror("execve");
		throw ThisShouldNeverHappenException("execve failed");
	} else if (child > 0) {

		PTracer pt(child);

		if (!pt.handshake()) {
			return;
		}

		BOOST_FOREACH(Address a, iPoints) {
			pt.addBreakpoint(a);
		}
		exit(1);

		pt.continueChild();

		pt.run();

	} else {
		perror("fork");
		exit(1);
	}
}


int main(int argc, char **argv)
{
	Configuration::setConfig(&config);

	int newArg;
	if ((newArg = parseInputFromOptions(argc, argv)) < 0)
		exit(2);

	DEBUG(std::cout << "Newarg: " << newArg << std::endl;);
	DEBUG(std::cout << "        " << argv[newArg] << std::endl;)

	banner();

	ControlFlowGraph cfg;
	readCFG(cfg);

	std::list<Address> unresolved;
	int numUnres = getUnresolvedAddresses(cfg, unresolved);
	std::cout << std::dec << numUnres << " unresolved jumps." << std::endl;

	runInstrumented(unresolved, argc - newArg + 1, &argv[newArg]);

	return 0;
}


#include "instruction/instruction_udis86.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
BOOST_CLASS_EXPORT_GUID(Udis86Instruction, "Udis86Instruction");
#pragma GCC diagnostic pop
