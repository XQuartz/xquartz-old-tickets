#!/usr/sbin/dtrace -C -s
 
#pragma D option quiet
 
proc::posix_spawn:exec-success,proc::__mac_execve:exec-success
{
this->isx64=(curproc->p_flag & P_LP64)!=0;
#define SELECT_64_86(x64, x86) (this->isx64 ? (x64) : (x86))
#define GET_POINTER(base, offset) (user_addr_t)SELECT_64_86(*(uint64_t*)((base)+sizeof(uint64_t)*(offset)), *(uint32_t*)((base)+sizeof(uint32_t)*(offset)))
 
this->ptrsize=SELECT_64_86(sizeof(uint64_t),sizeof(uint32_t));
this->argc=curproc->p_argc;
 
// I havn't recognized whether the x64 occurs tha same problem (argv\[0\] points invalid area)
this->isClean=SELECT_64_86(1, (curproc->p_dtrace_argv==(uregs[R_SP],sizeof(uint32_t),sizeof(uint32_t))));
this->argv=(uint64_t)copyin(curproc->p_dtrace_argv,this->ptrsize*this->argc);
 
/* printf("%s with args:%d (%p, %p)\n",execname, this->argc, curproc->pdtraceargv, uregs\[R_SP\]); */
 
printf("%s ", (0 < this->argc && this->isClean) ? copyinstr(GET_POINTER(this->argv,0)) : "");
printf("%s ", (1 < this->argc && this->isClean) ? copyinstr(GET_POINTER(this->argv,1)) : "");
printf("%s ", (2 < this->argc && this->isClean) ? copyinstr(GET_POINTER(this->argv,2)) : "");
printf("%s ", (3 < this->argc && this->isClean) ? copyinstr(GET_POINTER(this->argv,3)) : "");
printf("%s ", (4 < this->argc && this->isClean) ? copyinstr(GET_POINTER(this->argv,4)) : "");
printf("%s ", (5 < this->argc && this->isClean) ? copyinstr(GET_POINTER(this->argv,5)) : "");
printf("%s ", (6 < this->argc && this->isClean) ? copyinstr(GET_POINTER(this->argv,6)) : "");
printf("%s ", (7 < this->argc && this->isClean) ? copyinstr(GET_POINTER(this->argv,7)) : "");
printf("%s ", (8 < this->argc && this->isClean) ? copyinstr(GET_POINTER(this->argv,8)) : "");
printf("%s ", (9 < this->argc && this->isClean) ? copyinstr(GET_POINTER(this->argv,9)) : "");
printf("\n");
 
#undef GET_POINTER
#undef SELECT_64_86
}
