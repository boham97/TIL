# Oracle ASM (Automatic Storage Management)

## 개요

Oracle Database 전용 볼륨 매니저 겸 파일 시스템.
OS 레벨 파일 시스템(ext4, NTFS 등) 대신 Oracle이 직접 디스크를 관리.

Synology NAS와 개념적으로 유사하나 **Oracle DB 전용**으로 최적화된 스토리지.

---

## 구성 요소

| 구성 요소 | 설명 |
|---|---|
| ASM Instance | ASM 전용 Oracle 인스턴스 (DB 인스턴스와 별도) |
| Disk Group | 물리 디스크들의 논리적 묶음 |
| ASM Disk | ASM이 관리하는 개별 디스크 또는 파티션 |
| ASM File | DB 데이터파일, 리두로그 등 실제 파일 |

---

## Redundancy 종류

| 종류 | 설명 |
|---|---|
| External | RAID 장비가 이중화 담당, ASM 미러링 없음 |
| Normal | 2-way 미러링 (디스크 2개 이상) |
| High | 3-way 미러링 (디스크 3개 이상) |

---

## 주요 프로세스

ASM 인스턴스 프로세스는 `ora_` 대신 `asm_` prefix 사용.

| 프로세스 | 역할 |
|---|---|
| asm_pmon_+ASM1 | 프로세스 모니터링 |
| asm_diag_+ASM1 | alert log, trace 파일 작성 담당 |
| asm_rbal_+ASM1 | 리밸런싱 총괄 |
| asm_gmon_+ASM1 | Disk Group 상태 모니터링 |
| asm_asmb_+ASM1 | DB 인스턴스 ↔ ASM 인스턴스 통신 |
| asm_mmon_+ASM1 | AWR, ASH 스냅샷 수집 |
| asm_lmon_+ASM1 | RAC 글로벌 lock 모니터링 |
| asm_vktm_+ASM1 | 가상 타임 관리 |

### 프로세스 부모

```
PID 1 (systemd)
└── asm_pmon_+ASM1
└── asm_diag_+ASM1
└── asm_rbal_+ASM1
└── ...  (전부 PPID=1)
```

### DB ↔ ASM 연결 프로세스

```
ora_asmb_racdb1                   ← DB 인스턴스의 ASMB
oracle+ASM1_asmb_racdb1           ← ASM 쪽 서버 프로세스
```

---

## 환경변수

### ASM vs DB 인스턴스

```bash
# ASM 인스턴스
ORACLE_SID=+ASM1
ORACLE_HOME=/u01/app/19c/grid      # Grid Home (별도!)

# DB 인스턴스
ORACLE_SID=ORCL
ORACLE_HOME=/u01/app/oracle/product/19c/db
```

### ASM 접속

```bash
export ORACLE_SID=+ASM1
sqlplus / as sysasm    # sysdba 아닌 sysasm 사용
```

---

## ADR (Automatic Diagnostic Repository)

### 구조

```
$ORACLE_BASE/diag/          ← ADR Base (ASM, DB 공통)
├── asm/                    ← ASM 로그
│   └── +asm/
│       └── +ASM1/
│           └── trace/
├── rdbms/                  ← DB 로그
│   └── orcl/
│       └── ORCL/
│           └── trace/
└── crs/                    ← Clusterware 로그
```

- **ADR Base** : ASM, DB 동일 (`$ORACLE_BASE`)
- **ADR Home** : ASM, DB 각각 다름

### 로그 종류별 작성 주체

| 로그 종류 | 작성 주체 |
|---|---|
| alert log | asm_diag_+ASM1 |
| 각 프로세스 trace | 해당 프로세스 직접 |
| incident 패키징 | asm_diag_+ASM1 |

### adrci 사용

```bash
export ORACLE_SID=+ASM1
adrci

adrci> show homes
adrci> set homepath diag/asm/+asm/+ASM1
adrci> show incident -mode detail -p "incident_id=75788"
adrci> purge -age 43200        # 30일 이상 purge
adrci> purge -age 43200 -type INCIDENT
```

---

## 로그 인코딩

- alert log / trace 파일 인코딩은 **NLS_CHARACTERSET** 기준
- 실제 프로세스 인코딩은 `/proc/[asm_diag PID]/environ` 으로 확인

```bash
# asm_diag PID 확인
ps -ef | grep asm_diag

# 환경변수 확인
cat /proc/[PID]/environ | tr '\0' '\n' | grep -i nls
```

```sql
-- ASM 인스턴스에서 캐릭터셋 확인
select value from v$nls_parameters
where parameter='NLS_CHARACTERSET';
```

---

## RAC 환경 주요 에러

### ORA-27300 / No buffer space available

```
ORA-00603: ORACLE server session terminated by fatal error
ORA-27504: IPC error creating OSD context
ORA-27300: OS system dependent operation:sendmsg failed with status: 105
ORA-27301: OS failure message: No buffer space available
ORA-27302: failure occurred at: sskgxpsnd2
```

- **원인** : RAC 노드간 interconnect UDP 버퍼 부족 (ENOBUFS)
- **확인** : `169.254.x.x` Link-Local 주소 → 가상 인터페이스가 interconnect로 잡힌 경우

```bash
# 소켓 버퍼 확인
sysctl net.core.wmem_max
sysctl net.core.rmem_max

# UDP 에러 통계
netstat -su | grep -i error

# 임시 조치
sysctl -w net.core.wmem_max=4194304
sysctl -w net.core.rmem_max=4194304
```

- **근본 해결** : interconnect 전용 물리 NIC 분리

---

## 유용한 명령어

```bash
# ASM 프로세스 확인
ps -ef | grep asm

# trace 디렉토리 용량 확인
du -sh $ORACLE_BASE/diag/asm/+asm/+ASM1/trace

# 특정 에러 패턴 검색
grep -E "ORA-|error|sendmsg" /u01/app/oracle/diag/asm/+asm/+ASM1/trace/alert_+ASM1.log

# NLS 파라미터 확인
sqlplus / as sysasm
SQL> select * from v$nls_parameters where parameter like '%CHARACTERSET%';
```