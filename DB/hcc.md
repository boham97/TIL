# TIL: 일반 Oracle → Exadata 마이그레이션과 HCC 압축

**2026-06-22**

오늘 정리한 핵심: **"Exadata로 옮기면 SQL을 다 바꿔야 한다"는 건 과장**이다. SQL 문법은 100% 호환되어 그대로 돈다. 다만 Exadata의 진짜 무기인 **Smart Scan**과 **HCC 압축**을 끌어내려면 '조건'을 맞춰야 한다는 게 포인트.

## 1. 무엇을 바꾸고 무엇을 안 바꾸나

| 항목 | 바꿔야 하나? |
|---|---|
| SELECT/INSERT 문법 | ❌ 그대로 동작 |
| SELECT 힌트 | ❌ Smart Scan은 힌트로 제어 **불가** (런타임 자동 결정) |
| INSERT 힌트 | ✅ HCC 노리면 `/*+ APPEND */` 등 **direct path 필수** |
| 테이블 DDL | ✅ HCC 쓸 거면 `COMPRESS FOR QUERY/ARCHIVE` 추가 |
| 인덱스/통계 전략 | ⚠️ DW성 풀스캔이 잘 풀리도록 점검 |

## 2. Smart Scan은 강제할 수 없다

- 셀 오프로딩(Smart Scan)은 **Full Scan + Direct Path Read일 때만** 작동.
- 힌트로 강제 못 한다. `_serial_direct_read`, `_small_table_threshold`, 테이블 통계 blocks 등으로 **런타임에 자동 결정**.
- 인덱스 위주 OLTP 쿼리는 안 탄다 → 풀스캔이 유리한 **DW성 쿼리**라야 자동으로 direct read → smart scan.
- 그래서 힌트가 아니라 **통계/인덱스 전략 점검**이 할 일.

## 3. HCC 압축은 'direct path load'일 때만 걸린다

가장 충격적이었던 실측 비교 (같은 데이터, 같은 압축 테이블):

```
일반 INSERT INTO ... SELECT   → 1536 MB  (압축 거의 안 됨)
INSERT /*+ APPEND */ ...       →  104 MB  (HCC 압축됨)  ≈ 15배
```

- direct path load = `APPEND` / `APPEND_VALUES` / 병렬 DML / SQL*Loader direct / CTAS.
- ⚠️ **HCC된 데이터에 UPDATE 하면 압축이 풀린다** → 거의 안 바뀌는 **이력/아카이브**에 적합, 자주 바뀌는 테이블엔 부적합.

## 4. 이미 적재된 데이터를 사후 압축하려면

```sql
ALTER TABLE tab MOVE COMPRESS FOR ARCHIVE LOW;  -- 24MB → 2MB
ALTER INDEX idx_tab REBUILD;                     -- MOVE 후 인덱스 UNUSABLE → 재생성 필수
```

- 함정: `ALTER TABLE ... COMPRESS` (MOVE 없이)는 **앞으로 들어올 데이터의 속성만** 지정. 기존 데이터는 안 건드림.
- MOVE 주의: 작업 중 **테이블 락**(조회만 가능), 일시적으로 공간 **약 2배** 필요.
- 무중단 필요하면 → `DBMS_REDEFINITION`(그림자 테이블 바꿔치기) 또는 **12c+ `MOVE ... ONLINE`** 한 단어.

## 5. 시계열 데이터 = HCC 정석 케이스

옛날 파티션은 안 바뀌니까(UPDATE 없음 → 압축 풀릴 일 없음) **무중단 같은 복잡한 거 필요 없다.**

```sql
-- 날짜 RANGE 파티셔닝이 전제. 최신 파티션은 안 건드리고 옛날 것만:
ALTER TABLE sales MOVE PARTITION p202401
    COMPRESS FOR ARCHIVE LOW UPDATE INDEXES;  -- 글로벌 인덱스 UNUSABLE 방지
```

압축 레벨: `QUERY HIGH`(~6X) / **`ARCHIVE LOW`(~7X, 시간 대비 가성비 추천)** / `ARCHIVE HIGH`(~12X, 압축시간 4배).

## 6. ILM ADO 자동화의 진실

```sql
ALTER TABLE sales ILM ADD POLICY
    COMPRESS FOR ARCHIVE LOW SEGMENT AFTER 90 DAYS OF NO MODIFICATION;
```

자동으로 되긴 하는데 **즉시도 아니고 공짜도 아니다**:
- **Heat Map을 먼저 켜야 함** (`ALTER SYSTEM SET HEAT_MAP = ON`) — 안 켜면 정책 걸어도 아예 안 돈다. **가장 흔한 실수.**
- 90일 카운트 기준 = 데이터의 **마지막 수정 시각**, 실행 시각 = **야간 유지보수 윈도우**. "정책 만든 시각"이나 "90일 뒤 그 시각"과 무관.
- 12c 이상 + Advanced Compression Option 라이선스 필요. 11g엔 ADO 없음.

## 7. 그래서 결론: 정확한 시점 통제는 ADO 말고 DBMS_SCHEDULER

ADO는 "야간에 알아서 대략" → 시점 통제가 약하다. **"내가 정한 주기에 확실히"** 가 필요하면:

```sql
DBMS_SCHEDULER.CREATE_JOB(
  job_name        => 'COMPRESS_OLD_PARTITIONS',
  job_type        => 'STORED_PROCEDURE',
  job_action      => 'PRC_COMPRESS_OLD_PART',   -- 90일 지난 파티션 MOVE
  repeat_interval => 'FREQ=DAILY; BYHOUR=3',     -- 매일 새벽 3시
  enabled         => TRUE);
```

