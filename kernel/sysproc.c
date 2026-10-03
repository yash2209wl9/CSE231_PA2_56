#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"
#include "procinfo.h"

extern struct proc proc[NPROC];
extern struct spinlock wait_lock;

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if (t == SBRK_EAGER || n < 0) {
    if (growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if (addr + n < addr)
      return -1;
    if (addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep_prepare(&ticks);
    release(&tickslock);
    sleep();
    acquire(&tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// ===== Q1: getuptime (owner: Yash Vardhan) =====
// Returns the number of timer ticks since boot.
uint64
sys_getuptime(void)
{
  uint xticks;

  acquire(&tickslock);   // same locking the kernel already does in sys_uptime
  xticks = ticks;        // read the shared counter while protected
  release(&tickslock);   // always release before returning
  return xticks;
}

// ===== Q2: activecount (owner: Yash Vardhan) =====
// Counts the entries in the process table whose state is not UNUSED.
uint64
sys_activecount(void)
{
  struct proc *p;
  int n = 0;

  for(p = proc; p < &proc[NPROC]; p++){   // visit every slot (NPROC = 64)
    acquire(&p->lock);                    // p->state is protected by p->lock
    if(p->state != UNUSED)                // USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE all count
      n++;
    release(&p->lock);                    // release before moving to the next slot
  }
  return n;
}

// ===== Q3: lineage (owner: Yash Vardhan) =====
static int
lineage_snap(struct proc *p, int *pid, char *name, struct proc **parent)
{
  acquire(&wait_lock);        // guards p->parent
  acquire(&p->lock);          // guards p->state, p->pid, p->name
  if(p->state == UNUSED){     
    release(&p->lock);
    release(&wait_lock);
    return 0;
  }
  *pid = p->pid;                    
  safestrcpy(name, p->name, 16);    // copy the name safely (max 16 bytes, always NUL-terminated)
  *parent = p->parent;              // remember who the parent is (0 for init)
  release(&p->lock);
  release(&wait_lock);
  return 1;
}

// Returns how many lines were printed, or -1 if `pid` is not an active process.
uint64
sys_lineage(void)
{
  int pid, cpid = 0, count = 0;
  char name[16];                 // fixed max name length (matches struct proc)
  struct proc *p, *cur = 0, *par = 0;

  argint(0, &pid);               // read the syscall argument (returns void here)

  // find the slot whose pid matches and which is in use.
  for(p = proc; p < &proc[NPROC]; p++){
    if(lineage_snap(p, &cpid, name, &par) && cpid == pid){
      cur = p;                   // remember the slot we found
      break;
    }
  }
  if(cur == 0)
    return -1;                   // no active process has this pid

  // climbing the parent chain one process at a time.
  // No locks are held inside this loop body except while snapshotting.
  while(1){
    printk("PID %d: %s\n", cpid, name);   // kernel printing uses printk, not printf
    count++;

    // Stop at init (pid 1), at a missing parent, or after NPROC steps
    if(cpid == 1 || par == 0 || count >= NPROC)
      break;

    cur = par;                                    // move up to the parent
    if(!lineage_snap(cur, &cpid, name, &par))    
      break;
  }
  return count;
}

// ===== Q4: getprocsize (owner: Parag Prasun) =====
uint64
sys_getprocsize(void)
{
  return 0;
}

// ===== Q5: familyheadcount (owner: Parag Prasun) =====
uint64
sys_familyheadcount(void)
{
  return 0;
}

// ===== Bonus: getprocs (owner: Parag Prasun) =====
uint64
sys_getprocs(void)
{
  return 0;
}
