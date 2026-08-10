# setuid & setgid 정리

유닉스/리눅스의 **특수 권한 비트(special permission bits)** 로, 파일 실행 시 권한 상승 및 그룹 상속과 관련된 개념입니다.

---

## 1. 개요

| 비트 | 8진수 값 | 표시 위치 | 주요 효과 |
|------|----------|-----------|-----------|
| setuid | 4000 | 소유자 실행 권한 자리 (`s`) | 실행 시 **파일 소유자** 권한으로 동작 |
| setgid | 2000 | 그룹 실행 권한 자리 (`s`) | 실행 시 **파일 소유 그룹** 권한으로 동작 / 디렉터리에서는 그룹 상속 |
| sticky bit | 1000 | 기타 실행 권한 자리 (`t`) | 디렉터리 내 파일을 소유자만 삭제 가능 (예: `/tmp`) |

---

## 2. setuid (Set User ID)

실행 파일에 setuid 비트가 설정되어 있으면, 그 프로그램을 실행하는 동안 프로세스가 **실행한 사람이 아니라 파일 소유자의 권한**으로 동작합니다.

### 대표 예시: `passwd`

```bash
$ ls -l /usr/bin/passwd
-rwsr-xr-x 1 root root 68208 ... /usr/bin/passwd
   ↑
   소유자 실행 권한 자리가 x 대신 s
```

일반 사용자는 root만 수정 가능한 `/etc/shadow`를 직접 건드릴 수 없지만,
`passwd`에 setuid가 걸려 있어 실행 순간에는 root 권한으로 동작하여 자기 비밀번호를 변경할 수 있습니다.

### 관련 UID 개념

- **RUID (Real UID)**: 프로세스를 실제로 실행한 사용자
- **EUID (Effective UID)**: 권한 검사에 실제로 사용되는 UID → setuid 프로그램에서 소유자 UID로 바뀜
- **SUID (Saved UID)**: 권한을 잠시 내렸다가 되돌릴 때 보관용

---

## 3. setgid (Set Group ID)

### (1) 실행 파일에 설정한 경우

실행 시 프로세스가 파일 소유 **그룹**의 권한으로 동작합니다.

```bash
$ ls -l /usr/bin/wall
-rwxr-sr-x 1 root tty ... /usr/bin/wall
```

### (2) 디렉터리에 설정한 경우 ← 실무에서 더 자주 사용

해당 디렉터리 안에 새로 생성되는 파일/하위 디렉터리가
생성자의 기본 그룹이 아니라 **디렉터리의 그룹을 상속**받습니다.
팀 공유 폴더를 만들 때 유용합니다.

```bash
sudo mkdir /srv/team
sudo chgrp devteam /srv/team
sudo chmod 2775 /srv/team    # setgid + rwxrwxr-x

# 이후 누가 파일을 만들든 그룹은 항상 devteam
```

---

## 4. 설정 및 확인 방법

### 설정

```bash
chmod u+s  file          # setuid 설정
chmod u-s  file          # setuid 해제
chmod g+s  file_or_dir   # setgid 설정
chmod g-s  file_or_dir   # setgid 해제

chmod 4755 file          # 숫자 방식: setuid + rwxr-xr-x
chmod 2775 dir           # 숫자 방식: setgid + rwxrwxr-x
chmod 6755 file          # setuid + setgid 동시 설정
```

### 표시 규칙 (`ls -l`)

| 상태 | 표시 |
|------|------|
| 실행 권한 O + setuid/setgid O | 소문자 `s` |
| 실행 권한 X + setuid/setgid O | 대문자 `S` |

> 대문자 `S`는 "실행 권한이 없어 사실상 의미 없는 설정"이라는 신호이므로 설정 실수를 의심해야 합니다.

### 시스템 내 setuid/setgid 파일 점검

```bash
find / -perm -4000 -type f 2>/dev/null    # setuid 파일 목록
find / -perm -2000 -type f 2>/dev/null    # setgid 파일 목록
find / -perm -6000 -type f 2>/dev/null    # 둘 다
```

---

## 5. 보안 주의사항

- setuid 프로그램은 **권한 상승(privilege escalation) 공격의 단골 표적**입니다.
  버퍼 오버플로, 환경변수 조작, 심볼릭 링크 공격 등으로 root 탈취까지 이어질 수 있습니다.
- 대부분의 현대 유닉스/리눅스는 **쉘 스크립트의 setuid를 무시**합니다 (레이스 컨디션 때문).
- 마운트 옵션 `nosuid`를 사용하면 해당 파일시스템에서 setuid/setgid가 무시됩니다.
  (외부 저장장치, `/tmp` 등에 권장)
- 최근에는 setuid 대신 **Linux capabilities**로 필요한 최소 권한만 부여하는 방식이 권장됩니다.

```bash
# 예: ping에 raw socket 권한만 부여 (전체 root 권한 대신)
sudo setcap cap_net_raw+ep /usr/bin/ping
```

### setuid 프로그램 작성 시 체크리스트

- [ ] 환경변수(`PATH`, `LD_PRELOAD`, `IFS` 등)를 신뢰하지 않고 초기화
- [ ] 작업이 끝나면 즉시 권한 반납 (`setuid(getuid())`)
- [ ] 입력값 길이/형식 검증 철저
- [ ] 절대경로로 외부 명령 호출, `system()` 대신 `execve()` 사용
- [ ] 파일 접근 시 TOCTOU(Time-of-Check to Time-of-Use) 취약점 주의

---

## 6. 참고 자료

- [Setuid — Wikipedia](https://en.wikipedia.org/wiki/Setuid) : setuid/setgid 개념 전반
- [setuid(2) — Linux manual page](https://man7.org/linux/man-pages/man2/setuid.2.html) : setuid 시스템 콜 명세
- [setgid(2) — Linux manual page](https://man7.org/linux/man-pages/man2/setgid.2.html) : setgid 시스템 콜 명세
- [chmod(1) — Linux manual page](https://man7.org/linux/man-pages/man1/chmod.1.html) : 권한 설정 명령어
- [capabilities(7) — Linux manual page](https://man7.org/linux/man-pages/man7/capabilities.7.html) : setuid의 대안인 capabilities
- [credentials(7) — Linux manual page](https://man7.org/linux/man-pages/man7/credentials.7.html) : RUID/EUID/SUID 개념 정리
- [Secure Programming HOWTO — Minimize Privileges](https://dwheeler.com/secure-programs/Secure-Programs-HOWTO/minimize-privileges.html) : setuid 프로그램 보안 작성 가이드
