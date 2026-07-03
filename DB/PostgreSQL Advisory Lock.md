# TIL: PostgreSQL Advisory Lock으로 배치 중복 실행 방지하기

## 배경

REFRESH MATERIALIZED VIEW 배치가 같은 시각에 중복 실행되는 문제가 있었다.
스케줄러(cron 등)가 여러 번 발사되거나, 여러 노드에서 같은 배치를 쏘는 상황에서
DB 레벨에서 "동시에 하나만 실행"을 보장할 방법이 필요했다.

→ **PostgreSQL advisory lock**이 정답이었다.

## Advisory Lock이란

- PG가 제공하는 **애플리케이션 정의 락**. 특정 테이블/행이 아니라 임의의 정수 키에 락을 건다
- 락의 의미는 애플리케이션이 정한다 (예: "이 배치는 지금 실행 중")
- 인스턴스 전역으로 동작 → 어떤 커넥션에서 잡았든 다른 커넥션에서 보인다
- 스케줄러가 pg_cron이든 OS cron이든 Airflow든 상관없음. 결국 다 DB 커넥션으로 실행되므로 **DB에서 한 번 막으면 끝**

## 두 가지 종류: 세션 락 vs 트랜잭션 락

| | 세션 락 `pg_try_advisory_lock` | 트랜잭션 락 `pg_try_advisory_xact_lock` |
|---|---|---|
| 해제 시점 | 명시적 unlock 또는 세션 종료 | 트랜잭션 종료(커밋/롤백) 시 자동 |
| unlock 필요 | O (try/finally 필수) | X |
| 커넥션 풀 위험 | unlock 누락 시 다음 사용자가 락 상속 | 없음 |
| 여러 트랜잭션에 걸친 유지 | 가능 | 불가 (첫 커밋에서 풀림) |

**기본 선택은 xact 락.** 자동 해제라 실수할 여지가 없다.
세션 락은 청크 단위로 커밋하는 긴 배치처럼 "여러 트랜잭션에 걸쳐 락을 유지해야 할 때"만 쓴다.

## 패턴 1: 스킵 (try + xact 락)

이미 돌고 있으면 그냥 건너뛰고, 다음 스케줄에 다시 시도.
주기 배치(10분/1시간)나 REFRESH MV에 적합.

```sql
DO $$
BEGIN
  IF pg_try_advisory_xact_lock(hashtext('mv_test')) THEN
    REFRESH MATERIALIZED VIEW CONCURRENTLY mv_test;
  ELSE
    RAISE NOTICE 'skip: refresh already running';
  END IF;
END $$;
```

- 성공 → 실행 후 커밋 시점에 자동 unlock
- 실패 → 스킵 (잡은 락이 없으니 해제할 것도 없음)
- 예외 → 롤백 시점에 자동 unlock
- **unlock 코드가 아예 없다** = 실수할 여지가 없다

## 패턴 2: 대기 (블로킹 + xact 락)

이미 돌고 있으면 끝날 때까지 기다렸다가 실행.
하루 1회처럼 반드시 완료돼야 하는 배치에 적합.

```sql
BEGIN;
SELECT pg_advisory_xact_lock(hashtext('batch_monthly_report'));
-- 배치 로직
COMMIT;
```

⚠️ 대기 세션이 쌓일 수 있으므로 `statement_timeout` 또는 `lock_timeout`을 걸어야 안전.

## 패턴 3: 세션 락 + try/finally (긴 배치)

청크 단위로 커밋하는 배치는 xact 락을 쓰면 첫 커밋에서 락이 풀려버린다.

```sql
BEGIN;
  SELECT pg_try_advisory_xact_lock(...);   -- 락 획득
  -- 작업 A
COMMIT;   -- ⚠️ 여기서 락 풀림!
BEGIN;
  -- 작업 B (락 없이 실행됨)
```

**finally에서 unlock을 반드시 호출.** 안 하면 커넥션 풀의 다음 사용자가 락을 물려받는다.

## 패턴 4: 네임스페이스 (2-키 버전)

배치가 여러 개면 키 충돌 방지를 위해 2인자 버전 사용:

```sql
-- (system_id int4, batch_key int4)
SELECT pg_try_advisory_xact_lock(1001, hashtext('daily_settlement'));
SELECT pg_try_advisory_xact_lock(1001, hashtext('hourly_aggregate'));
SELECT pg_try_advisory_xact_lock(2001, hashtext('daily_settlement'));  -- 다른 시스템
```

## 상황별 선택 가이드

| 상황 | 추천 |
|---|---|
| 단일 SQL/DO 블록 배치 (REFRESH MV, 집계 UPDATE) | `pg_try_advisory_xact_lock` (스킵) |
| 여러 트랜잭션에 걸친 긴 배치 (청크 커밋) | `pg_try_advisory_lock` + try/finally |
| 반드시 한 번은 돌아야 하는 배치 | `pg_advisory_xact_lock` (블로킹) + timeout |
| 여러 앱 서버가 같은 배치 발사 | advisory lock으로 해결 가능 (DB가 하나면) |
| 여러 PG 클러스터로 배치가 갈라짐 | ❌ advisory lock 불가 → Redis/ZK/etcd 등 외부 분산 락 |

## 주의할 함정

1. **advisory lock은 단일 인스턴스 전역까지만.** 리플리카/다른 클러스터에는 안 보인다. (스탠바이는 read-only라 애초에 락 획득도 안 됨)
2. **xact 락 + 중간 커밋 = 락 소실.** 청크 커밋 배치엔 세션 락을 쓸 것.
3. **세션 락 + 커넥션 풀 = 락 상속 위험.** try/finally로 반드시 unlock.
4. **세션 좀비.** 네트워크 단절 등으로 세션이 좀비가 되면 락도 같이 좀비가 될 수 있다. PG가 결국 감지해서 정리하지만 시간이 걸린다. `idle_in_transaction_session_timeout`, TCP keepalive 설정으로 완화.

## 모니터링

현재 잡혀 있는 advisory lock 확인:

```sql
SELECT l.pid, l.classid, l.objid, l.granted, a.state, a.query_start
FROM pg_locks l
JOIN pg_stat_activity a ON a.pid = l.pid
WHERE l.locktype = 'advisory';
```
