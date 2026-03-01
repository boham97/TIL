```
[root@localhost mutex_cas]# strace -c ./bench mutex
생산자 8개 / 소비자 8개 / 생산량 1000000개씩
[MUTEX]  소비 합계: 8000000  시간: 7682.25 ms
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 99.94    7.228830     1807207         4           futex
  0.02    0.001787         111        16           clone
  0.02    0.001089          49        22           mprotect
  0.01    0.000818          32        25           mmap
  0.01    0.000697          53        13           munmap
  0.00    0.000056          56         1         1 access
  0.00    0.000043          14         3           openat
  0.00    0.000028          14         2           write
  0.00    0.000028           7         4           fstat
  0.00    0.000021           5         4           brk
  0.00    0.000017           8         2           rt_sigaction
  0.00    0.000009           3         3           read
  0.00    0.000009           3         3           close
  0.00    0.000005           5         1           lseek
  0.00    0.000005           5         1           rt_sigprocmask
  0.00    0.000005           5         1           prlimit64
  0.00    0.000005           5         1           getrandom
  0.00    0.000004           2         2         1 arch_prctl
  0.00    0.000004           4         1           set_tid_address
  0.00    0.000004           4         1           set_robust_list
  0.00    0.000000           0         1           execve
------ ----------- ----------- --------- --------- ----------------
100.00    7.233464       65166       111         2 total
[root@localhost mutex_cas]# strace -c ./bench cas
생산자 8개 / 소비자 8개 / 생산량 1000000개씩
[CAS]    소비 합계: 8000000  시간: 3547.90 ms
         push: 8000000  pop: 8000000
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 93.95    0.151932       37983         4           futex
  4.30    0.006958         278        25           mmap
  0.84    0.001354          61        22           mprotect
  0.65    0.001059          81        13           munmap
  0.26    0.000418          26        16           clone
  0.00    0.000001           0         3           write
  0.00    0.000000           0         3           read
  0.00    0.000000           0         3           close
  0.00    0.000000           0         4           fstat
  0.00    0.000000           0         1           lseek
  0.00    0.000000           0         4           brk
  0.00    0.000000           0         2           rt_sigaction
  0.00    0.000000           0         1           rt_sigprocmask
  0.00    0.000000           0         1         1 access
  0.00    0.000000           0         1           execve
  0.00    0.000000           0         2         1 arch_prctl
  0.00    0.000000           0         1           set_tid_address
  0.00    0.000000           0         3           openat
  0.00    0.000000           0         1           set_robust_list
  0.00    0.000000           0         1           prlimit64
  0.00    0.000000           0         1           getrandom
------ ----------- ----------- --------- --------- ----------------
100.00    0.161722        1443       112         2 total
```

