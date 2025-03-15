#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"
#include "syscallcount.h"
#include "defs.h"

int syscallcount[31];

int strcmp2(const char *s1, const char *s2)
{
  while (*s1 && (*s1 == *s2))
  {
    s1++;
    s2++;
  }
  return *(unsigned char *)s1 - *(unsigned char *)s2;
}

// Function to get the length of a string
int strlen2(const char *s)
{
  int len = 0;
  while (*s++)
    len++;
  return len;
}

// extern int sys_getSysCount(void);

// Fetch the uint64 at addr from the current process.
int fetchaddr(uint64 addr, uint64 *ip)
{
  struct proc *p = myproc();
  if (addr >= p->sz || addr + sizeof(uint64) > p->sz) // both tests needed, in case of overflow
    return -1;
  if (copyin(p->pagetable, (char *)ip, addr, sizeof(*ip)) != 0)
    return -1;
  return 0;
}

// Fetch the nul-terminated string at addr from the current process.
// Returns length of string, not including nul, or -1 for error.
int fetchstr(uint64 addr, char *buf, int max)
{
  struct proc *p = myproc();
  if (copyinstr(p->pagetable, buf, addr, max) < 0)
    return -1;
  return strlen(buf);
}

static uint64
argraw(int n)
{
  struct proc *p = myproc();
  switch (n)
  {
  case 0:
    return p->trapframe->a0;
  case 1:
    return p->trapframe->a1;
  case 2:
    return p->trapframe->a2;
  case 3:
    return p->trapframe->a3;
  case 4:
    return p->trapframe->a4;
  case 5:
    return p->trapframe->a5;
  }
  panic("argraw");
  return -1;
}

// Fetch the nth 32-bit system call argument.
void argint(int n, int *ip)
{
  *ip = argraw(n);
}

// Retrieve an argument as a pointer.
// Doesn't check for legality, since
// copyin/copyout will do that.
void argaddr(int n, uint64 *ip)
{
  *ip = argraw(n);
}

// Fetch the nth word-sized system call argument as a null-terminated string.
// Copies into buf, at most max.
// Returns string length if OK (including nul), -1 if error.
int argstr(int n, char *buf, int max)
{
  uint64 addr;
  argaddr(n, &addr);
  return fetchstr(addr, buf, max);
}

// Prototypes for the functions that handle system calls.
extern uint64 sys_fork(void);
extern uint64 sys_exit(void);
extern uint64 sys_wait(void);
extern uint64 sys_pipe(void);
extern uint64 sys_read(void);
extern uint64 sys_kill(void);
extern uint64 sys_exec(void);
extern uint64 sys_fstat(void);
extern uint64 sys_chdir(void);
extern uint64 sys_dup(void);
extern uint64 sys_getpid(void);
extern uint64 sys_sbrk(void);
extern uint64 sys_sleep(void);
extern uint64 sys_uptime(void);
extern uint64 sys_open(void);
extern uint64 sys_write(void);
extern uint64 sys_mknod(void);
extern uint64 sys_unlink(void);
extern uint64 sys_link(void);
extern uint64 sys_mkdir(void);
extern uint64 sys_close(void);
extern uint64 sys_waitx(void);
extern uint64 sys_getSysCount(void);
extern uint64 sys_sigalarm(void);
extern uint64 sys_sigreturn(void);
extern uint64 sys_settickets(void);

// An array mapping syscall numbers from syscall.h
// to the function that handles the system call.
static uint64 (*syscalls[])(void) = {
    [SYS_fork] sys_fork,
    [SYS_exit] sys_exit,
    [SYS_wait] sys_wait,
    [SYS_pipe] sys_pipe,
    [SYS_read] sys_read,
    [SYS_kill] sys_kill,
    [SYS_exec] sys_exec,
    [SYS_fstat] sys_fstat,
    [SYS_chdir] sys_chdir,
    [SYS_dup] sys_dup,
    [SYS_getpid] sys_getpid,
    [SYS_sbrk] sys_sbrk,
    [SYS_sleep] sys_sleep,
    [SYS_uptime] sys_uptime,
    [SYS_open] sys_open,
    [SYS_write] sys_write,
    [SYS_mknod] sys_mknod,
    [SYS_unlink] sys_unlink,
    [SYS_link] sys_link,
    [SYS_mkdir] sys_mkdir,
    [SYS_close] sys_close,
    [SYS_waitx] sys_waitx,
    [SYS_getSysCount] sys_getSysCount,
    [SYS_sigalarm] sys_sigalarm,
    [SYS_sigreturn] sys_sigreturn,
    [SYS_settickets] sys_settickets,
};

// void
// syscall(void)
// {
//   int num;
//   struct proc *p = myproc();

//   num = p->trapframe->a7;
//   if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
//     // Use num to lookup the system call function for num, call it,
//     // and store its return value in p->trapframe->a0
//     p->syscall_count[num]++;
//     p->trapframe->a0 = syscalls[num]();
//   } else {
//     printf("%d %s: unknown sys call %d\n",
//             p->pid, p->name, num);
//     p->trapframe->a0 = -1;
//   }
// }

// void syscall(void)
// {
//   int num;
//   struct proc *p = myproc();

//   num = p->trapframe->a7; // Get the syscall number
//   if (num > 0 && num < NELEM(syscalls) && syscalls[num])
//   {

//     // Increment the syscall count for the current process
//     p->syscall_count[num]++;
//     syscallcount[num]++;
//     // if (p->syscall_count[num] < syscallcount[num])
//     // {
//     //   p->syscall_count[num] = syscallcount[num];
//     //   // syscallcount[num]=0;
//     // }
//     // If the current process was started by syscount, we need to track the system calls it makes
//     // if (p->parent && strcmp2(p->parent->name, "syscount") == 0)
//     // {
//     //   // If the parent is 'syscount', track system calls made by this process too
//     //   p->parent->syscall_count[num]++; // Add to the parent's syscall count
//     //   syscallcount[num]++;             // Increment the global syscall count as well
//     //   if (p->parent->syscall_count[num] < syscallcount[num])
//     //   {
//     //     p->parent->syscall_count[num] = syscallcount[num];
//     //     syscallcount[num] = 0; // Reset the local syscall count for this process and parent to zero after tracking it with parent
//     //   }
//     // }

//     // Call the system call and store the result in a0
//     p->trapframe->a0 = syscalls[num]();
//   }
//   else
//   {
//     printf("%d %s: unknown sys call %d\n", p->pid, p->name, num);
//     p->trapframe->a0 = -1;
//   }
// }

void syscall(void) {
    int num;
    struct proc *p = myproc(); // Get the current process

    // Read the system call number from the trap frame
    num = p->trapframe->a7; // a7 contains the syscall number for RISC-V

    // Call the corresponding syscall function
    if (num > 0 && num < NELEM(syscalls) && syscalls[num]) {
        p->syscall_count[num]++;
        syscallcount[num]++;
        p->trapframe->a0 = syscalls[num](); // Call the syscall function and store the return value in a0
    } else {
        p->trapframe->a0 = -1; // Invalid syscall
    }
}

