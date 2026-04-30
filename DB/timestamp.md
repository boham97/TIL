# 타임존 & Unix Time 처리 정리

> Oracle / PostgreSQL에서 Unix time을 다루며 타임존, DST, 시간 단위 자르기, 파티셔닝까지 다룬 토론 정리.

---

## 1. Oracle에서 Unix time 뽑기

### ✅ 정확한 Unix time (UTC 기준)

```sql
(CAST(SYS_EXTRACT_UTC(SYSTIMESTAMP) AS DATE) - DATE '1970-01-01') * 86400 AS unixtime
```

- `SYS_EXTRACT_UTC()` 가 OS 타임존을 UTC로 변환해줌
- Unix time은 정의상 UTC 기준이므로 "OS 타임존 기준 Unix time"이란 개념은 없음

### ❌ 잘못된 패턴

```sql
-- 서버 타임존에 따라 결과가 달라짐
(SYSDATE - DATE '1970-01-01') * 86400
```

KST 서버에서는 실제 Unix time보다 32400초(9시간) 크게 나옴.

---

## 2. 월/시간 단위 Truncate

### UTC 기준 월 1일

```sql
(TRUNC(CAST(SYS_EXTRACT_UTC(SYSTIMESTAMP) AS DATE), 'MM') - DATE '1970-01-01') * 86400
```

### OS 타임존 기준 월 1일 (DST까지 안전)

```sql
(CAST(
    SYS_EXTRACT_UTC(
        FROM_TZ(
            CAST(TRUNC(CAST(SYSTIMESTAMP AS DATE), 'MM') AS TIMESTAMP),
            TO_CHAR(SYSTIMESTAMP, 'TZR')   -- ⚠️ 'TZH:TZM' 아님
        )
    ) AS DATE
) - DATE '1970-01-01') * 86400 AS unixtime
```

### ⚠️ TZH:TZM vs TZR

| 방식 | DST 환경에서의 안전성 |
|------|----------------------|
| `TZH:TZM` (오프셋) | ❌ 쿼리 시점 오프셋을 과거 시각에 적용 → DST 전후 결과 다름 |
| `TZR` (타임존 이름) | ✅ Oracle이 해당 시점의 룰을 직접 참조 |

#### 영국 예시 (DST 종료: 10/26 02:00 → 01:00)

`TZH:TZM` 방식이면 **10월 26일 쿼리 시점에 따라 10월 1일 Unix time 값이 달라짐.**

- DST 종료 전 → BST(+01:00) 적용 → 09-30 23:00 UTC ✅
- DST 종료 후 → GMT(+00:00) 적용 → 10-01 00:00 UTC ❌

---

## 3. PostgreSQL에서 Unix time 뽑기

```sql
EXTRACT(EPOCH FROM your_timestamp AT TIME ZONE 'Pacific/Fiji')
```

| 컬럼 타입 | 쿼리 |
|---------|------|
| `TIMESTAMP` (TZ 없음) | `EXTRACT(EPOCH FROM ts AT TIME ZONE 'Pacific/Fiji')` |
| `TIMESTAMPTZ` | `EXTRACT(EPOCH FROM ts)` (TZ 정보 이미 있음) |

### 컬럼 별칭에 물음표 넣기

```sql
SELECT ... AS "unixtime?"  -- 큰따옴표로 감싸기
```

---

## 4. AT TIME ZONE 의 핵심 규칙

| 입력 타입 | 의미 | 출력 타입 |
|---------|------|---------|
| `TIMESTAMP` (naive) | "이 시각이 X TZ다" 선언 | `TIMESTAMPTZ` |
| `TIMESTAMPTZ` | X TZ에서 본 벽시계 시각으로 변환 | `TIMESTAMP` (naive) |

### 자주 하는 실수

```sql
-- ❌ '런던 시각으로 해석'을 의도했지만 동작 안 함
to_timestamp('20260429140001', 'YYYYMMDDHH24MISS')  -- 이미 세션TZ로 박힘
    AT TIME ZONE 'Europe/London'                     -- 표시만 변환
```

```sql
-- ✅ ::timestamp 로 TZ 떼고 다시 붙이기
to_timestamp('20260429140001', 'YYYYMMDDHH24MISS')::timestamp
    AT TIME ZONE 'Europe/London'
```

### `EXTRACT(EPOCH FROM timestamp)` 의 동작

공식 문서 ([PG 18 §9.9.1](https://www.postgresql.org/docs/current/functions-datetime.html)):
> "the **nominal** number of seconds since 1970-01-01 00:00:00, **without regard to timezone or daylight-savings rules**"

- 입력이 naive `TIMESTAMP`면 TZ 무시하고 그대로 epoch 차이 계산
- 결과적으로 "UTC로 간주한 것과 동일한 값" 이 나옴

---

## 5. DST 종료 시 모호한 시각 처리

PostgreSQL 공식 동작 ([Appendix B.2](https://www.postgresql.org/docs/current/datetime-invalid-input.html)):

> "an ambiguous timestamp that could fall on either side of a jump-back transition is assigned the UTC offset that prevailed **just after the transition**"

### 영국 10/26 01:30 입력 시

| 가능한 해석 | UTC | 채택? |
|-----------|-----|------|
| 01:30 BST (+01:00) | 00:30 UTC | ❌ |
| 01:30 GMT (+00:00) | 01:30 UTC | ✅ (DST 종료 후 채택) |

### Spring-forward (없는 시각)

- `2018-03-11 02:30` (미국 동부): 존재하지 않는 시각
- 종료 전 오프셋 적용 → `03:30 EDT` 로 해석

> 원칙: **"Standard-time interpretation is preferred when in doubt"**

---

## 6. 특이한 타임존

### 30분 / 45분 오프셋

| 타임존 | 오프셋 | 비고 |
|--------|------|------|
| `Asia/Kolkata` | +05:30 | 인도 |
| `Asia/Kathmandu` | **+05:45** | 네팔 |
| `Australia/Eucla` | +08:45 | 호주 비공식 |
| `Pacific/Chatham` | **+12:45/+13:45** | DST까지 있음 |
| `Australia/Lord_Howe` | +10:30/+11:00 | DST 점프 30분 |

### 극단 케이스
- `Pacific/Kiritimati`: +14:00 (가장 빠름)
- 사모아: 2011년 12월 30일을 통째로 건너뜀

### 테스트 추천 타임존
```
Pacific/Chatham        - 45분 + DST
Australia/Lord_Howe    - 30분 DST 점프
Africa/Casablanca      - 라마단으로 연 4회 전환
Asia/Kathmandu         - +05:45 고정
America/St_Johns       - -03:30/-02:30
```

---

## 7. Unix time 시간 단위로 자르기

### ❌ 흔한 오해

```python
unixtime // 3600 * 3600   # UTC 매시 00분
unixtime // 900 * 900     # UTC 매 15분 (모든 타임존 경계)
```

이건 **UTC 기준**이지 대상 타임존 매시 00분이 **아님**.

### 타임존별 매시 00분 epoch 의 `% 3600`

| 타임존 | epoch % 3600 |
|--------|------------|
| 한국 (+09:00) | 0 |
| 인도 (+05:30) | 1800 |
| 네팔 (+05:45) | 900 |
| Chatham (+12:45) | 2700 |

→ **타임존 정보 없이 산술만으로는 절대 못 자름.**

### ✅ 올바른 산술 (보정값 활용)

```
hour_boundary = ((epoch - correction) / 3600) * 3600 + correction
```

⚠️ `(epoch / 3600 * 3600) + correction` 는 매시 45~59분 구간에서 **미래 시각**으로 튐.

### ✅ 또는 PostgreSQL `date_trunc` 사용

```sql
extract(epoch from
    date_trunc('hour', to_timestamp(:unixtime) AT TIME ZONE :tz)
    AT TIME ZONE :tz
)::bigint
```

`AT TIME ZONE` 두 번 쓰는 이유:
1. 첫 번째: TIMESTAMPTZ → TIMESTAMP (로컬 벽시계로) → `date_trunc` 가 로컬 기준으로 자르도록
2. 두 번째: TIMESTAMP → TIMESTAMPTZ (이 시각이 해당 TZ라고 선언) → DST 자동 처리

---

## 8. 보정값 룩업 테이블 (선택적 최적화)

### 생성

```sql
CREATE TABLE tz_hour_correction AS
SELECT 
    name AS tz_name,
    (extract(epoch FROM utc_offset)::int % 3600 + 3600) % 3600 AS correction
FROM pg_timezone_names;

CREATE UNIQUE INDEX ON tz_hour_correction(tz_name);
```

### 안전성 검증 (분 단위 보정값이 변하는 변태 TZ 골라내기)

```sql
WITH samples AS (
    SELECT 
        n.name AS tz_name,
        (extract(epoch FROM (gs AT TIME ZONE 'UTC' AT TIME ZONE n.name) - gs)::int 
            % 3600 + 3600) % 3600 AS correction
    FROM pg_timezone_names n
    CROSS JOIN generate_series(
        '2026-01-01'::timestamptz, 
        '2026-12-01'::timestamptz, 
        '1 month'::interval
    ) gs
)
SELECT tz_name, array_agg(DISTINCT correction)
FROM samples
GROUP BY tz_name
HAVING count(DISTINCT correction) > 1;
```

→ 거의 `Australia/Lord_Howe` 정도만 나옴.

### ⚠️ `pg_timezone_names` 의 한계

> 공식 문서: "The displayed information is computed based on the current value of CURRENT_TIMESTAMP."

- `utc_offset` 값은 **"지금 이 순간"** 기준
- 다행히 분 단위 보정값(`% 3600`)은 거의 모든 TZ에서 시점 무관하게 일정
- 분기 1회 정도 갱신 권장

---

## 9. 그래서 진짜 어떻게 할까?

### 비용 비교 (대략)

| 연산 | 1회 비용 |
|------|--------|
| `date_trunc` + `AT TIME ZONE` | ~1μs |
| 룩업 테이블 JOIN | ~0.5~1μs |
| 단순 산술 보정 | ~50ns |
| **INSERT 한 줄 (WAL/인덱스/fsync)** | **~50~500μs** |

→ `date_trunc` 비중은 INSERT 전체의 0.2~2%. **노이즈 수준.**

### 산술 보정이 진짜 의미 있는 경우

- **`IMMUTABLE` 표현식**이 필요할 때 (파티션 키 표현식, 표현식 인덱스)
- `AT TIME ZONE 'literal'` 은 **STABLE** 이라 파티션/인덱스 표현식으로 쓸 수 없음

### 그냥 컬럼에 저장하는 용도라면

```sql
ALTER TABLE logs ADD COLUMN hour_bucket BIGINT 
    GENERATED ALWAYS AS (
        extract(epoch FROM date_trunc('hour', 
            to_timestamp(unixtime) AT TIME ZONE tz_name)
            AT TIME ZONE tz_name)::bigint
    ) STORED;
```

- 모든 엣지케이스 자동 처리
- 코드 단순
- IANA tzdata 갱신 자동 반영

---

## 10. 파티션 / 유니크 인덱스 영향

### SELECT 프루닝 vs INSERT 라우팅 — 메커니즘 다름

| 항목 | SELECT 프루닝 | INSERT 라우팅 |
|------|------------|-------------|
| 결정 시점 | plan time / execution time | row 평가 시점 |
| 결정 단위 | 쿼리 단위 | row 단위 |
| EXPLAIN 표시 | `Subplans Removed`, partition list | 안 보임 |
| WHERE 절 분석 | O | 무관 |
| STABLE 표현식 허용 | O (runtime pruning) | O (값 평가 후) |
| IMMUTABLE 요구 | X | X |

### 인덱스/파티션 키 표현식 자체 — IMMUTABLE 필수

```sql
-- ❌ 에러: STABLE 함수 사용 불가
CREATE INDEX ON logs (date_trunc('hour', to_timestamp(unixtime) AT TIME ZONE 'Asia/Seoul'));
```

→ 단, `unixtime` 자체를 잘라서 BIGINT 컬럼에 저장하는 설계라면 이 문제 자체가 발생 안 함.

### 잘린 unixtime을 컬럼/파티션 키/유니크 인덱스에 쓸 때

| 항목 | 결과 |
|------|------|
| INSERT 라우팅 | ✅ 정상 |
| 유니크 검사 | ✅ 정상 |
| SELECT 정적 프루닝 | ✅ 잘 됨 |
| SELECT 동적 프루닝 (`now()` 등) | ✅ runtime pruning 동작 |
| `date_trunc` 사용 | ✅ 문제없음 |

---

## 11. 중복 처리는 어디서?

| 자르는 위치 | 정확성 | 성능 (중복 빈번 시) |
|------------|-------|------------------|
| INSERT문 | ✅ | △ DB까지 와서 막힘 |
| 앱에서 미리 + 메모리 dedup | ✅ | ✅ 네트워크/DB 모두 절약 |
| 앱에서 미리 + `ON CONFLICT DO NOTHING` | ✅ | △ DB까지 와서 막힘 |

→ 중복 빈도가 핵심 변수. 잦으면 앱 측 dedup 권장.

---

## 12. 최종 결론

> ✅ **그냥 `date_trunc` 쓰자.**

- 정확함 (DST, 45분 오프셋, 사모아 같은 변태 케이스 모두 처리)
- 단순함
- INSERT 성능엔 사실상 영향 없음
- 라우팅/유니크 인덱스 동작 모두 정상
- IANA tzdata 갱신 자동 반영

산술 보정값 룩업은 **표현식 인덱스/파티션 키에 IMMUTABLE이 필요할 때만** 의미 있음.

---

## 부록: 자주 헷갈린 포인트

1. **`SYSTIMESTAMP` vs `SYSDATE`**: 전자만 TZ 정보 포함
2. **`SESSIONTIMEZONE` ≠ `DBTIMEZONE` ≠ OS TZ**: 서로 다를 수 있음
3. **`current_setting('timezone')`은 세션 TZ**: DB TZ 아님
4. **`to_timestamp()`은 TIMESTAMPTZ 반환**: 세션 TZ로 해석됨
5. **`AT TIME ZONE` 두 번**: 입력 타입에 따라 의미 정반대
6. **`EXTRACT(EPOCH FROM naive_ts)`**: TZ 무시하고 nominal 계산
7. **DST jump-back 모호 시각**: 표준시(나중) 채택
8. **`pg_timezone_names`**: 현재 시점 기준 값
9. **STABLE vs IMMUTABLE**: 인덱스/파티션 키 표현식 가능 여부 결정
10. **INSERT 라우팅**: row 단위 평가, WHERE 절 분석과 무관