# 베어메탈 vs VM vs 도커 지연 시간 실험 기록

간단한 C 프로그램으로 **베어메탈, VM(버추얼박스), 도커 컨테이너**에서 지연 시간 분포(p50/p90/p99/p99.9/max)를 측정한 기록.
처음 가설은 "도커와 베어메탈은 p50은 비슷해도 p90, p99는 다를 것"이었다.

---

## 1. 결론 요약

| 비교 | 결과 |
|---|---|
| 베어메탈 vs 도커 (같은 서버, 제한 없음) | **거의 같다** |
| 베어메탈 vs 버추얼박스 VM | **확연히 다르다.** 타이머 깨어남 p50 약 14배, p99 약 200배 |
| VM에서 실시간 우선순위(`chrt`) | **효과 없음** → 지연 원인이 게스트 바깥(호스트)에 있음 |
| 휴지페이지(THP) 효과 | 256MB 구간 개선 폭이 베어메탈 약 8%, VM 약 28% → VM에서 TLB 미스가 더 비쌈 |

> 진지한 성능 테스트는 **베어메탈**에서, 비교는 **같은 서버·같은 시간대**에서 해야 한다.

---

## 2. 실험 환경

| 구분 | 베어메탈 | VM | 도커 서버 |
|---|---|---|---|
| CPU | AMD Ryzen 5 7430U (Zen 3, 6코어) | 호스트: Intel Core Ultra 5 125H (P4 + E8 + LP-E2 하이브리드) | 별도 서버 (부하 있음) |
| OS | Rocky Linux 10 | 게스트: Rocky Linux 8 | 컨테이너: Rocky 이미지 |
| L1d / L2 / L3 | 32KB·코어 / 512KB·코어 / 16MB | 48KB / 2MB / 18MB (호스트 CPU 값) | - |
| 비고 | - | 버추얼박스, vCPU 5개 | 베어메탈과 다른 서버 |

VM 설정 (버추얼박스 상태 표시줄 툴팁 기준):

| 항목 | 값 |
|---|---|
| 실행 엔진 | VT-x/AMD-V (거북이 모드 아님) |
| 네스티드 페이징 | 활성 |
| 실행 제한 | 100% |
| 반가상화 인터페이스 | KVM |
| clocksource / clockevent | `tsc` / `lapic` |

---

## 3. 측정 도구

| 파일 | 모드 | 측정 대상 |
|---|---|---|
| `lat.c` | `sleep` | 1ms 주기 절대시각 타이머의 **늦게 깨어난 시간** (cyclictest 방식) |
| | `syscall` | `getpid` 시스템콜 왕복 |
| | `work` | 고정량 CPU 연산 소요 시간 |
| `yield.c` | - | `sched_yield()` 왕복 |
| `cpuid.c` | - | `cpuid` 명령 지연 (VM에서는 매번 VM exit) |
| `memwalk.c` | `4KB` / `huge` | 무작위 포인터 체이싱으로 메모리 접근 1회 지연 (16KB ~ 1GB) |

빌드: `gcc -O2 -static -o <이름> <이름>.c`
정적 빌드라 같은 바이너리를 호스트와 컨테이너에서 그대로 실행할 수 있다.

---

## 4. 결과

### 4.1 타이머 깨어남 지연 (`lat sleep 10000`, 단위: us)

| 환경 | p50 | p90 | p99 | p99.9 | max |
|---|---|---|---|---|---|
| 베어메탈 (1회) | 56.1 | 57.2 | 57.7 | 59.2 | 63.9 |
| 베어메탈 (2회) | 56.0 | 57.2 | 57.6 | 58.3 | 61.0 |
| 베어메탈 + `chrt -f 50` | **4.6** | 4.8 | 5.7 | 7.6 | 8.6 |
| VM (1회) | 804 | 3,399 | 11,912 | 23,817 | 57,232 |
| VM (2회) | 855 | 3,755 | 9,833 | 19,706 | 38,782 |
| VM + `chrt -f 50` | 970 | 4,255 | 11,472 | 19,737 | 37,592 |
| 도커 (다른 서버, 부하 있음) | 60.4 | 84~90 | 127 | 181~190 | 609~2,171 |

**해석**

- **베어메탈 p50 56us 중 약 50us는 타이머 슬랙**이다. 일반(SCHED_OTHER) 스레드는 전력 절약을 위해 타이머 만료를 기본 50us까지 늦춘다.
  `chrt`로 실시간 우선순위를 주면 슬랙이 적용되지 않아 **실제 깨어남 지연 약 4.6us**만 남는다.
- 베어메탈은 p50과 max 차이가 10us 이내로 **매우 조용한 시스템**이다.
- **VM은 p50부터 약 0.8ms**, p99는 10ms 안팎, max는 수십 ms.
- **VM에서 `chrt`가 효과 없음**: 실시간 우선순위는 게스트 안의 모든 작업보다 먼저 실행되게 해주는데도 그대로라는 건,
  지연이 게스트 내부가 아니라 **호스트가 vCPU를 늦게 실행해주거나 타이머 인터럽트를 늦게 넣어주는 데서** 생긴다는 뜻.
- **도커 결과는 비교 불가**: 다른 서버에 부하까지 있어서, 차이가 도커 때문인지 서버·부하 때문인지 구분할 수 없다.
  이후 같은 서버에서 비교한 결과 **거의 같다**고 판단.

### 4.2 네트워크 왕복 (`ping` 같은 LAN 대상, 약 300회, 단위: ms)

| 환경 | p50 | p90 | p99 | max |
|---|---|---|---|---|
| VM | 1.01 | 8.13 | 14.8 | 16.0 |
| 베어메탈 | 0.54 | 7.73 | 15.3 | 17.8 |

**해석**

- **p50 차이 약 0.47ms가 VM이 왕복마다 추가하는 비용.** 가상 NIC 전달과 잠든 vCPU를 깨우는 비용으로, 타이머 실험의 VM 지연과 같은 성격.
- **p90 이상은 양쪽이 거의 같다.** 응답이 약 0.5ms 무리와 약 10ms 무리로 **두 갈래**로 나뉘는 현상이 양쪽 모두에서 보였고,
  이건 서버가 아니라 **대상 장비나 경로**의 특성. 이 잡음이 커서 VM의 꼬리 차이는 가려졌다.
- ping은 상대 쪽 응답을 커널이 처리해서 애플리케이션 경로가 빠지고, ICMP는 장비에서 낮은 우선순위로 처리되기도 해서
  **p99 측정용으로는 한계**가 있다. 정확히 하려면 `netperf TCP_RR` 같은 요청/응답 테스트가 필요.

### 4.3 메모리 접근 지연 (`memwalk`, 접근 1회당 ns)

| 배열 크기 | 베어메탈 4KB | 베어메탈 huge | VM 4KB | VM huge |
|---|---|---|---|---|
| 16 KB | 0.9 | 0.9 | 3.2 | 2.9 |
| 256 KB | 2.8 | 2.8 | 14.0 | 6.7 |
| 4 MB | 11.9 | 10.6 | 29.3 | 24.6 |
| 32 MB | 87.7 | 81.9 | 250.2 | 218.8 |
| 256 MB | 136.8 | 126.1 | 377.4 | 272.0 |
| 1 GB | 139.9 | 127.7 | 386.2 | 353.9 |

**해석**

- **베어메탈은 캐시 구조와 정확히 일치**: 16KB(L1 32KB 안) → 256KB(L2 512KB 안) → 4MB(L3 16MB 안) → 32MB 이상(메인 메모리)으로 계단식 증가.
- **두 기계의 절대값 비교는 무의미.** 16KB는 캐시·TLB 미스가 없어 가상화 비용이 끼어들 곳이 없는데도 3배 이상 차이 나는 건,
  **CPU 자체가 다르기 때문**(AMD Zen 3 vs Intel 하이브리드).
- **대신 각 기계 안에서 휴지페이지 개선 폭을 비교**:

  | 256MB 구간 | 4KB → huge | 개선 |
  |---|---|---|
  | 베어메탈 | 136.8 → 126.1 | 약 8% |
  | VM | 377.4 → 272.0 | **약 28%** |

  VM은 네스티드 페이징 때문에 TLB 미스 한 번에 주소 변환을 **두 겹**으로 해야 해서, TLB 미스를 줄여주는 휴지페이지 효과가 훨씬 크다.
- VM의 1GB 구간은 개선이 작은데(약 8%), VM 메모리가 넉넉하지 않아 2MB 페이지를 다 못 받았을 가능성이 있다.
  실행 중 `grep AnonHugePages /proc/meminfo`로 확인 가능.

---

## 5. VM이 느린 이유 정리

측정과 확인을 통해 좁혀간 순서:

1. `tsc` clocksource 확인 → **시간 읽기는 문제 없음**
2. 버추얼박스 설정 확인 (VT-x, 네스티드 페이징, KVM 반가상화) → **설정은 이미 최적**
3. `chrt`로 게스트 내부 우선순위 최대화 → **효과 없음** → 원인은 게스트 바깥
4. 호스트 CPU가 **Intel 하이브리드(P/E/LP-E 코어)** 로 확인 → vCPU 스레드가 느린 E코어에 배치되거나,
   VM 창이 포커스를 잃었을 때 전원 스로틀링을 받을 수 있음

결론: **"VM이라서"** 가 맞고, 특히 **노트북 위 데스크톱 하이퍼바이저**라 더 심하다.
서버용 KVM에 CPU 고정 등 튜닝을 하면 이보다 훨씬 작아진다.

미적용 개선책 (호스트 관리자 명령 프롬프트):

```bat
powercfg /powerthrottling disable /path "C:\Program Files\Oracle\VirtualBox\VirtualBoxVM.exe"
```

---

## 6. 개념 정리

| | 무엇을 나누나 | 커널 | 성능 영향 |
|---|---|---|---|
| **VM** | 하드웨어 전체 | 게스트 커널 따로 | 큼. 타이머·인터럽트·특권 명령·TLB 미스마다 하이퍼바이저 개입 |
| **도커 (리눅스)** | 보이는 범위(네임스페이스), 쓸 수 있는 양(cgroup) | 호스트 커널 공유 | 거의 없음. 컨테이너 프로세스도 호스트의 일반 프로세스 |
| **도커 (윈도우/맥)** | 위와 같음 | WSL2/경량 VM 안의 리눅스 커널 | VM 비용이 붙음 |
| **파이썬 venv** | 패키지 경로만 | 호스트 그대로 | 없음. 가상화가 아님 |

- 컨테이너는 실행 경로에 끼어드는 **층이 아니라 이름표**에 가깝다. 리눅스는 원래 모든 프로세스를 cgroup에 넣어 관리한다.
- VM도 **순수 계산은 거의 베어메탈 속도**다. 비용은 "항상 조금씩"이 아니라 **"특정 순간에 크게"** 생겨서 평균보다 **꼬리(p99)** 에서 드러난다.
- 도커가 베어메탈과 동급인 건 **제한 없음 + `--network host` + 볼륨** 조건에서다. `--cpus` 제한, NAT, 컨테이너 내부 파일시스템 쓰기는 비용이 있다.

---

## 7. 삽질 기록

| 증상 | 원인 | 해결 |
|---|---|---|
| `ld: cannot find lat` | `gcc`에 `-o` 누락 → `lat`을 입력 파일로 인식 | `gcc -O2 -o lat lat.c` |
| `ld: cannot find -lc` | 정적 glibc(`libc.a`) 없음 | `dnf --enablerepo=crb install glibc-static` (Rocky 8은 `powertools`) |
| `No such command: config-manager` | 컨테이너 최소 이미지에 `dnf-plugins-core` 없음 | `--enablerepo` 옵션 사용 또는 `dnf install dnf-plugins-core` |
| `lspci: command not found` | 최소 설치에 `pciutils` 없음 | `dnf install pciutils` |
| `./lat sleep 100000`이 안 끝남 | 샘플당 1ms → 약 100초 이상 소요 (정상) | 샘플 수를 10000으로 |
| root인데 `chrt: failed to set pid 0's policy` | `su -`로 root가 돼도 셸이 `user.slice` cgroup에 남음, RT 실행 시간이 0 | `echo $$ > /sys/fs/cgroup/cpu,cpuacct/tasks` 후 실행 |
| ping 백분위가 빈칸 | ping 출력이 한국어(`시간=`)라 `grep 'time='` 실패 | 앞에 `LC_ALL=C` |

---

## 8. 측정 원칙 (이번에 배운 것)

1. **같은 서버, 같은 시간대**에서 비교 대상을 **번갈아** 실행한다. 다른 서버·다른 부하와 비교하면 원인을 구분할 수 없다.
2. **절대값보다 같은 기계 안의 상대 변화**를 본다 (예: 4KB → huge 개선 폭).
3. 결과가 이상하면 **한 변수씩 제거**한다 (`chrt`로 스케줄링 배제, 슬랙 배제 등).
4. 꼬리 지연을 보려면 **샘플을 충분히** (p99는 수천 개, p99.9는 수만 개 이상).
5. 측정 대상 외의 잡음(ping 대상 장비, 호스트 하이브리드 코어 등)이 결과를 좌우하지 않는지 확인한다.

---

## 9. 다음에 해볼 것

- [ ] 같은 서버에서 호스트 vs 도커 번갈아 측정 결과 수치로 기록
- [ ] 도커 `--cpus` 제한 시 꼬리 변화 (`lat work`, `lat sleep`)
- [ ] 도커 `--cap-add=SYS_NICE` + `chrt`로 CFS 그룹 스케줄링 영향 분리
- [ ] `cpuid`, `yield` 베어메탈 vs VM 비교
- [ ] 버추얼박스 `powercfg` 스로틀링 해제 전후 비교
- [ ] `netperf TCP_RR`로 네트워크 p99 측정
- [ ] DPDK: 베어메탈 NIC가 **RTL8168**(`10ec:8168`) → 최신 DPDK의 r8169 PMD 필요, 소스 빌드 필요. 유선 NIC가 하나뿐이라 바인딩 시 SSH 끊김 주의

---

## 참고 자료

- `clock_nanosleep`: https://man7.org/linux/man-pages/man2/clock_nanosleep.2.html
- 타이머 슬랙 `PR_SET_TIMERSLACK`: https://man7.org/linux/man-pages/man2/PR_SET_TIMERSLACK.2const.html
- 스케줄링 정책 `sched(7)`: https://man7.org/linux/man-pages/man7/sched.7.html
- CFS 대역폭 제어(throttling): https://docs.kernel.org/scheduler/sched-bwc.html
- RT 그룹 스케줄링: https://docs.kernel.org/scheduler/sched-rt-group.html
- THP(투명 휴지페이지): https://docs.kernel.org/admin-guide/mm/transhuge.html
- 네임스페이스 / cgroup: https://man7.org/linux/man-pages/man7/namespaces.7.html , https://man7.org/linux/man-pages/man7/cgroups.7.html
- 버추얼박스 반가상화: https://www.virtualbox.org/manual/ch10.html#gimproviders
- 도커 리소스 제한: https://docs.docker.com/engine/containers/resource_constraints/
- IBM VM vs 컨테이너 논문: https://people.computing.clemson.edu/~jmarty/projects/lowLatencyNetworking/papers/NFVandContainers/AnupdatedPerfAnalysisofContainersandVMs.pdf
- DPDK R8169 PMD: https://doc.dpdk.org/guides/nics/r8169.html
