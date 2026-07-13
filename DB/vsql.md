# TIL — 2026-07-13

## Oracle: V$SQL child cursor 분리 원인 — "Max Long Length Different"

### 의미
- 커서를 파싱한 세션들 사이에 LONG/LOB 최대 길이 인식값이 달라서 기존 child cursor를 공유하지 못하고 새 child가 생기는 현상.
- 대표적으로 `4000`(MAX_STRING_SIZE=STANDARD 또는 구버전 클라이언트) vs `32767`(EXTENDED 또는 신버전 클라이언트/PL·SQL) 사이를 오감.

### 확인 방법
```sql
-- 사유 상세: XML 안에 커서 저장값 vs 현재 세션값이 나옴
SELECT child_number, DBMS_LOB.SUBSTR(reason, 4000, 1) AS reason_xml
  FROM v$sql_shared_cursor
 WHERE sql_id = :sql_id AND reason IS NOT NULL
 ORDER BY child_number;
```

REASON XML 예시:
```xml
<max_long_length_kkschlngv_cursor>4000</max_long_length_kkschlngv_cursor>
<max_long_length_HSTMXLNG_current>32767</max_long_length_HSTMXLNG_current>
```
- `kkschlngv_cursor` = 기존 커서가 파싱될 때 저장된 값
- `HSTMXLNG_current` = 지금 이 SQL을 다시 요청한 세션의 값
- 두 값이 다르면 기존 child를 재사용하지 못하고 새 child를 만든다.

### 원인 후보와 대조 쿼리
| 원인 | 확인 방법 |
|---|---|
| 직접 실행 vs PL/SQL 경유 실행 | child별 `program_id`가 0 / 非0으로 갈리는지 |
| 클라이언트 드라이버 버전 차이 | `V$SESSION_CONNECT_INFO`의 client_version 분포 |
| MAX_STRING_SIZE 파라미터와 클라이언트 인식 불일치 | `SHOW PARAMETER MAX_STRING_SIZE` |

```sql
-- child별 호출 경로: program_id = 0 → 직접 실행, ≠ 0 → PL/SQL 오브젝트 내부에서 실행
SELECT child_number, program_id, program_line_no, executions
  FROM v$sql WHERE sql_id = :sql_id ORDER BY child_number;

SELECT name, value FROM v$parameter WHERE name = 'max_string_size';
```

### 팁
- `v$sql_shared_cursor`는 버전마다 컬럼 구성이 달라서, UNPIVOT류 점검 쿼리를 돌리기 전에 `DESC v$sql_shared_cursor`로 실제 존재하는 컬럼을 먼저 확인할 것.

### 실무적 판단
- SQL 자체의 문제가 아니라 **호출 경로가 두 갈래**(직접 실행 vs 프로시저 경유, 또는 신/구 클라이언트 혼재)라는 신호.
- child가 2개 수준으로 유지되는 정도면 성능상 큰 문제는 아님. 근본적으로 정리하려면 호출 경로를 통일하거나 클라이언트 드라이버 버전을 맞추는 방향.

## 병렬쿼리에서 QC와 슬레이브의 통계 집계 방식

### 카운트 방식은 어떤 병렬 SQL이든 동일
- `EXECUTIONS` → **QC 몫만** +1
- `PX_SERVERS_EXECUTIONS` → 슬레이브 몫이 +DOP만큼 증가
- `FETCHES` / `ROWS_PROCESSED` / `END_OF_FETCH_COUNT` → 클라이언트 접점인 **QC에서만** 집계
- `PARSE_CALLS` → 슬레이브들도 각자 커서를 parse하므로 1 + DOP 수준으로 증가
- 그래서 병렬 SQL은 "executions는 1인데 px는 16" 같은 패턴이 정상. CONTAINERS() 특유가 아니라 병렬 실행 아키텍처 자체의 특성.

### child cursor가 갈라지느냐는 경우에 따라 다름
| 상황 | V$SQL 모습 |
|---|---|
| 단일 인스턴스 일반 PX | 보통 **행 1개**, 슬레이브 비용까지 합산됨 → buffer_gets/executions 그대로 유효 |
| PQ mismatch 발생 (`PQ_SLAVE_MISMATCH`, `PX_MISMATCH`) | QC child + 슬레이브 child 분리 → **SUM 해서** 봐야 진짜 비용 |
| RAC 크로스 노드 PX | 슬레이브가 배치된 노드의 V$SQL에 child가 따로 생김 → GV$SQL에서 inst_id별 합산 필요 |
| CONTAINERS() 쿼리 | 컨테이너 전환 때문에 거의 항상 분리 + 컨테이너별 파싱 |

### 델타 수집 파이프라인 관점의 결론
- 병렬 SQL은 어떤 형태로 쪼개져 있을지 모르니, 실행당 비용 계산 시 **`SQL_ID`(+RAC이면 전 노드) 기준으로 자원 통계는 SUM, 분모(executions)는 QC 것만** 쓰는 게 안전.
- child 하나만 집어서 나누면 QC child만 봐서 과소 계산되거나, 슬레이브 child에서 0으로 나누는 함정에 빠진다.

```sql
-- 분리형 child가 얼마나 있는지 점검
SELECT sql_id, COUNT(*) AS children,
       SUM(executions) AS total_exec,
       SUM(px_servers_executions) AS total_px
  FROM v$sql
 WHERE px_servers_executions > 0
 GROUP BY sql_id
HAVING COUNT(*) > 1
 ORDER BY total_px DESC;
```

### 슬레이브(워커) 개수 확인
```sql
-- 실행 중일 때 (가장 정확): QC별 슬레이브 수와 강등 여부
SELECT qcsid, COUNT(*) - 1 AS slave_count,   -- QC 자신 제외
       MAX(degree) AS actual_dop,
       MAX(req_degree) AS requested_dop
  FROM v$px_session
 GROUP BY qcsid;

-- 슬레이브 프로세스 목록
SELECT server_name, status, sid FROM v$px_process;
```
- `req_degree`(요청) vs `degree`(실제 할당)가 다르면 **DOP 강등(downgrade)** 발생.
- 실행이 끝난 뒤에는 `px_servers_executions 델타 ÷ executions 델타 ≈ 슬레이브 수`로 역산 가능. 단, 계획에 DFO가 여러 개면 중복 카운트되고 실행마다 DOP가 달랐으면 평균으로 뭉개지는 근사치.

## 곁들여 본 것
- V$SQL 스냅샷에서 `executions`는 안 늘고 `fetches`만 느는 구간은 장시간 fetch 중인 커서(배치 잡 등)일 수 있음 — `users_executing`, `last_active_time`과 함께 봐야 한다.