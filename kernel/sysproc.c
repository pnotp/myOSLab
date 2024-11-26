#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
// int
// sys_pgaccess(void)
// {
//   // lab pgtbl: your code here.
//   return 0;
// }
int
sys_pgaccess(void)
{
  // lab pgtbl: your code here.
  //pgaccess(buf, num, &abit);
  uint64 addr, abitsaddr;
  int size;
  unsigned int abits;
  struct proc *p = myproc();
    
  abits = 0;
  argaddr(0, &addr);
  argint(1, &size);
  argaddr(2, &abitsaddr);  
  for(int i = 0; i < size; i++){
    pte_t* pte = walk(p->pagetable, addr+i*PGSIZE, 0); 
    if(*pte & PTE_A){ 
     abits |= (1 << i); 
      *pte &= (~PTE_A);
    }   
  }
  copyout(p->pagetable, abitsaddr, (char*)&abits, 
                                    (uint64)sizeof(abits));
  return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
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
