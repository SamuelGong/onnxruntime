// ~/ort_ohos_compat.h
#pragma once

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

/* 仅在 Linux + musl（非 glibc）下启用；避免和其他平台冲突 */
#if defined(__linux__) && (!defined(__GLIBC__)) && (defined(__MUSL__) || 1)

#  include <sched.h>
#  include <pthread.h>
#  include <unistd.h>
#  include <errno.h>
#  include <sys/syscall.h>

#  ifdef __cplusplus
extern "C" {
#  endif

/* 获取当前线程 TID（无 SYS_gettid 时退化到进程级；够用） */
static inline int ort_gettid(void) {
#    ifdef SYS_gettid
  return (int)syscall(SYS_gettid);
#    else
  return (int)getpid();
#    endif
}

/* 缺省环境可能没有 CPU_COUNT，给一个兜底实现 */
#ifndef CPU_COUNT
static inline int CPU_COUNT(const cpu_set_t* set) {
  int cnt = 0;
  const unsigned long* p = (const unsigned long*)set;
  for (size_t i = 0; i < sizeof(*set) / sizeof(unsigned long); ++i) {
    unsigned long v = p[i];
    while (v) { cnt += (int)(v & 1UL); v >>= 1; }
  }
  return cnt;
}
#endif

/* 亲和力 shim：用 sched_*affinity 实现 pthread_*affinity_np */
#ifndef HAVE_PTHREAD_SETAFFINITY_NP
static inline int pthread_setaffinity_np(pthread_t /*thr*/,
                                         size_t cpusetsize,
                                         const cpu_set_t* mask) {
  return sched_setaffinity(ort_gettid(), cpusetsize, mask);
}
#endif

#ifndef HAVE_PTHREAD_GETAFFINITY_NP
static inline int pthread_getaffinity_np(pthread_t /*thr*/,
                                         size_t cpusetsize,
                                         cpu_set_t* mask) {
  return sched_getaffinity(ort_gettid(), cpusetsize, mask);
}
#endif

#  ifdef __cplusplus
} /* extern "C" */
#  endif
#endif /* linux+musl */

