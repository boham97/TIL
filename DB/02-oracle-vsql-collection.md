# Oracle V$SQL 통계 수집 모듈


## 2. child cursor 합산이 의미 있는가 (메모리 컬럼)

**질문:** `SHARABLE_MEM` / `PERSISTENT_MEM` / `RUNTIME_MEM`을 child별로 갈라진 로우끼리 합치는 게 의미 있는가? RUNTIME_MEM만 MAX()로 쓰는 건 어떤가?

> 이 대화에서 처음에는 "오라클 V$SQLAREA가 세 개 다 SUM한다"는 쪽으로 답이 흘렀는데, 질문의 요지는 **오라클이 뭘 하느냐가 아니라 그 집계가 실제로 의미가 있느냐**였습니다. 아래가 정정된 결론입니다.

| 컬럼 | 성격 | 집계 판단 |
|---|---|---|
| `SHARABLE_MEM` | 커서가 library cache에 살아있는 동안 **지속 점유**하는 공유 메모리 | **SUM 의미 있음** — 그 시점 실제 점유량 |
| `PERSISTENT_MEM` | 커서 open 상태 동안 유지되는 고정 메모리 | **SUM 의미 있음** |
| `RUNTIME_MEM` | **실행 중에만** 할당. V$SQL에 남는 값은 마지막 실행 시점(또는 5초 스냅샷) 기록 | SUM도 MAX도 현재 점유량이 아님 |

**RUNTIME_MEM 핵심:** child들이 동시에 실행된 적이 없다면, SUM은 **공존한 적 없는 값들을 더한 숫자**입니다. MAX 역시 "가장 컸던 어느 시점"일 뿐 현재 점유가 아닙니다. 실시간 실행 메모리가 필요하면 `V$SQL_WORKAREA_ACTIVE`를 봐야 합니다.

**수집 모듈 관점 권고:** 메모리 이상 탐지·추세 분석 목적이라면 집계 방식과 무관하게 `SHARABLE_MEM`, `PERSISTENT_MEM`이 더 신뢰할 만한 신호입니다.

**갈라진 원인(PX냐 다른 이유냐)이 합산에 영향을 주는가**
→ 메모리 점유량 관점에서는 **원인 불문 합산이 맞습니다.** 어떤 사유로 갈라졌든 각 child는 shared pool에 자기 몫의 실행계획을 실제로 따로 들고 있기 때문입니다. 오히려 bind mismatch 등으로 child가 수백 개 생기는 케이스는 합산 메모리가 커지는 것 자체가 문제의 본질입니다.
→ 반대로 **실행 성능 지표는 구분 유지**가 안전합니다. `SUM(elapsed_time)/SUM(executions)` 같은 평균은 좋은 플랜과 나쁜 플랜을 섞어버립니다.
→ `LOADED_VERSIONS = 0`인 child는 age-out되어 메모리를 안 쓰므로 정밀 합산 시 제외 대상입니다.

**근거**
- V$SQL: https://docs.oracle.com/en/database/oracle/oracle-database/19/refrn/V-SQL.html
- V$SQLAREA (child 합산 뷰): https://docs.oracle.com/en/database/oracle/oracle-database/19/refrn/V-SQLAREA.html
- V$SQL_WORKAREA_ACTIVE: https://docs.oracle.com/en/database/oracle/oracle-database/19/refrn/V-SQL_WORKAREA_ACTIVE.html

---

## 3. child cursor 갈라짐 원인 추적

**질문:** child가 왜 갈라졌는지 확인하는 방법과, PX 관련 사유.

- `V$SQL_SHARED_CURSOR`의 REASON 컬럼으로 mismatch 사유 확인
- 실제 데이터에서 나온 사유는 **`DIFFERENT_LONG_LENGTH`** — PX slave 세션이 코디네이터와 다른 max long length를 사용하면서 발생
  - (초기에 `SLAVE_QC_MISMATCH`로 추정했으나 실제 REASON 데이터가 이를 정정함)
- `FORCE_MATCHING_SIGNATURE`: 리터럴만 다른 SQL 패밀리를 묶어 식별하는 용도
- heap 6 = 실행계획을 담는 shared pool 컨텍스트 힙

**FORCE_MATCHING_SIGNATURE 테스트 메모:** DBeaver에서 PL/SQL 블록에 바인드 변수를 써도 리터럴 SQL이 잡히는 현상 → DBeaver가 익명 블록이 아니라 개별 SELECT 문으로 실행했기 때문.

**근거**
- V$SQL_SHARED_CURSOR: https://docs.oracle.com/en/database/oracle/oracle-database/19/refrn/V-SQL_SHARED_CURSOR.html

---

## 4. Pro*C 배열 fetch와 NULL

**질문:** 지시자 변수(indicator variable) 없는 NULL 컬럼이 배열 fetch에 영향을 주는가?

**결론:** 영향을 **줍니다.** 지시자 변수 없이 NULL이 반환되면 지정한 배열 크기만큼 fetch되지 않고 중단됩니다.
(초기 답변에서 "영향 없다"고 한 부분이 잘못이었고, 실제 동작으로 정정됨)

**대응:** 쿼리에서 `NVL()`로 감싸거나, 지시자 변수 배열을 함께 선언.

**근거**
- Pro*C/C++ 지시자 변수 및 배열 fetch: https://docs.oracle.com/en/database/oracle/oracle-database/19/lnpcc/host-variables.html

---

## 5. 개별 컬럼 모니터링 가치 정리

| 컬럼 | 무엇을 보나 |
|---|---|
| `PARSE_CALLS` | executions 대비 비율이 높으면 소프트 파싱 과다 (커서 캐싱 문제) |
| `LOADS` | 커서가 몇 번 다시 로드됐나 → shared pool 압박·age-out 지표 |
| `INVALIDATIONS` | DDL·통계수집 등으로 커서 무효화된 횟수 |
| `SORTS` | 정렬 발생 횟수 |
| `FETCHES` vs `ROWS_PROCESSED` | fetch 왕복 횟수 vs 실제 행 수 → array size 튜닝 근거 |
| `USERS_OPENING` vs `USERS_EXECUTING` | 커서를 열어둔 세션 수 vs 지금 실행 중인 세션 수 |
| `DIRECT_READS` | 다이렉트 패스 읽기(버퍼 캐시 우회) 발생량 |
| `AVOIDED_EXECUTIONS` | 결과 캐시 등으로 회피된 실행 |

---

## 6. 부수 확인: DB 버전 조회

```sql
-- Oracle
SELECT * FROM v$version;
SELECT * FROM product_component_version;
-- PostgreSQL
SELECT version();
-- MySQL / MariaDB
SELECT VERSION();   -- 또는 SELECT @@version;
-- SQLite
SELECT sqlite_version();
```

**근거**
- V$VERSION: https://docs.oracle.com/en/database/oracle/oracle-database/19/refrn/V-VERSION.html
- PostgreSQL 시스템 정보 함수: https://www.postgresql.org/docs/current/functions-info.html
