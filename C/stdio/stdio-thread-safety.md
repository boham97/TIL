# stdio 파일 쓰기와 스레드 안전성 정리

## 1. 락의 단위는 "파일"이 아니라 "FILE 객체"

- stdio는 각 `FILE` 객체마다 lockcount와 소유 스레드를 두고, 라이브러리 호출마다 락을 잡고 → I/O 수행 → 해제한다.
- `flockfile()` / `funlockfile()`이 노출하는 락이 곧 stdio가 내부적으로 쓰는 그 락이다.
- 따라서 보호 범위는 **FILE 구조체 단위**다. 같은 파일이라도 FILE이 다르면 서로를 전혀 모른다.

## 2. fopen을 여러 번 하면?

같은 경로를 두 번 열면 다음이 전부 별개가 된다.

| 항목 | 상태 |
|---|---|
| FILE 객체 | 2개 |
| stdio 내부 락 | 2개 (서로 무관) |
| stdio 버퍼 | 2개 |
| 커널 open file description (파일 오프셋) | 2개 |

→ libc 자료구조가 깨지는(크래시) 의미의 unsafe는 아니지만, **파일 내용이 덮어써지거나 뒤섞이는** 의미로는 확실히 unsafe.

### 정리표

| 구성 | libc 내부 상태 | 파일 내용 무결성 |
|---|---|---|
| FILE\* 1개 공유 | 안전 | fwrite 1회는 원자적 / 여러 호출 묶음은 깨짐 |
| FILE\* 여러 개 | 안전 | **깨짐 (덮어쓰기 · interleaving)** |

## 3. "버퍼보다 작게 쓰면 안전하다"는 오해

**FILE\* 1개를 공유하는 경우** → 사실상 문제없음. 락이 직렬화하므로 레코드가 write() 두 번으로 쪼개져도 바이트 순서는 유지된다.

**FILE\*이 여러 개인 경우** → 크기와 무관하게 위험. 버퍼는 레코드 경계가 아니라 **가득 찰 때** flush되기 때문이다.

```
버퍼 4096B, 레코드 100B 단위로 fwrite
현재 버퍼에 4050B 적재
  → fwrite(100B)
  → 앞 46B로 버퍼가 참 → write(4096) 발생
  → 나머지 54B는 버퍼에 잔류
  ⇒ 레코드 하나가 syscall 2개로 분할됨
```

이 틈에 다른 FILE\*이 flush하면 그대로 섞이거나 덮어쓴다.

> `PIPE_BUF`(4096B 이하 원자적 쓰기)는 **파이프/FIFO 전용**이며 일반 파일에는 적용되지 않는다.

레코드 = write() 1회를 보장하려면:
- `setvbuf(fp, NULL, _IONBF, 0)` 로 버퍼링 해제
- 레코드마다 `fflush()`
- 또는 `write(2)` 직접 사용

## 4. 일반 파일에서의 원자성

- POSIX.1-2008 XSI **2.9.7 Thread Interactions with Regular File Operations**: `write()`, `writev()` 등은 일반 파일에 대해 서로 원자적이어야 하며, **파일 오프셋 갱신도 그 효과에 포함**된다.
- 리눅스는 **3.14 이전**까지 이를 지키지 않았다. open file description을 공유한 동시 write()의 오프셋 갱신이 비원자적이라 출력 블록이 겹칠 수 있었고, 3.14에서 수정되었다.
- `O_APPEND`는 매 write 전에 오프셋을 파일 끝으로 옮기며, **오프셋 이동 + 쓰기가 단일 원자 단계**로 수행된다.

## 5. NFS 주의

- NFS는 여러 호스트가 네트워크로 공유하는 파일시스템이다. **로컬 FS(ext4/xfs 등)를 쓴다면 아래 경고는 해당 없음.**
- NFS는 파일 append를 지원하지 않아 클라이언트 커널이 이를 흉내내며, 이 과정에서 경쟁 상태를 피할 수 없다 → 여러 프로세스가 동시에 append하면 파일이 손상될 수 있다.
- 클라이언트 캐싱(close-to-open consistency)까지 겹치면 상황은 더 나빠진다.

## 6. 권장 구성

1. **FILE\* 하나만 열어 공유** + 여러 stdio 호출을 묶어야 하면 `flockfile()` / `funlockfile()`
2. 로그 등 append-only라면 **fd 하나를 `O_APPEND`로 열고 `write(2)` 직접 호출** (레코드당 1회)
3. 또는 **전용 writer 스레드 + 큐**, 혹은 애플리케이션 레벨 뮤텍스
4. `write()`의 **부분 쓰기(short write)** 반환값 처리는 항상 필요

---

## 참고 링크

- [POSIX `flockfile()` 명세 — The Open Group](https://pubs.opengroup.org/onlinepubs/9699919799/functions/flockfile.html)
- [`flockfile(3)` — Linux man page](https://man7.org/linux/man-pages/man3/flockfile.3.html)
- [GNU C Library — Streams and Threads](https://www.gnu.org/software/libc/manual/html_node/Streams-and-Threads.html)
- [`write(2)` — 원자성 및 Linux 3.14 수정 내역](https://man7.org/linux/man-pages/man2/write.2.html)
- [Linus Torvalds, "vfs: atomic f_pos accesses as per POSIX"](https://lkml.iu.edu/hypermail/linux/kernel/1403.0/01625.html)
- [`open(2)` — open file description / O_APPEND / NFS 주의](https://man7.org/linux/man-pages/man2/open.2.html)
- [`pipe(7)` — PIPE_BUF 원자성 (파이프 한정)](https://man7.org/linux/man-pages/man7/pipe.7.html)
- [`setvbuf(3)` — 버퍼링 모드 제어](https://man7.org/linux/man-pages/man3/setvbuf.3.html)
