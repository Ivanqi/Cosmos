/**********************************************************
    idle线程头文件krlcpuidle.h
***********************************************************/
#ifndef _KRLCPUIDLE_H
#define _KRLCPUIDLE_H

void init_krlcpuidle();
void krlcpuidle_start();
thread_t* new_cpuidle_thread();
void new_cpuidle();
void krlcpuidle_main();
thread_t* krlthread_execvl(thread_t* thread, char_t* filename);

#endif // KRLCPUIDLE_H
