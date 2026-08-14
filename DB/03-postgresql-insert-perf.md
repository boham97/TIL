# PostgreSQL 적재 성능

> 2026-08-14 · 오라클 조회 → PG 적재 구간

---

## 조회 쿼리를 PG의 PK 순서로 정렬해서 INSERT하면 성능에 도움이 되는가

**결론:** 원리적으로 도움은 되지만, **배치 크기와 PK 구성에 따라 효과 차이가 큽니다.** 수십만~수백만 건 대량 적재라면 체감되고, 수십~수백 건 주기 수집이면 사실상 무의미합니다.

### 이득이 발생하는 지점

PostgreSQL 힙 테이블은 PK 순서로 저장되지 않고 INSERT된 순서대로 쌓입니다. 따라서 정렬 INSERT의 이득은 테이블이 아니라 **PK B-tree 인덱스** 쪽에서 발생합니다.

- **무작위 키**: 매 INSERT가 B-tree 임의 위치 쓰기 → 페이지 분할 빈발, 인덱스 팽창, 쓰기 증폭, 캐시 효율 저하
- **정렬 키**: 인덱스 우측 끝 근처에 append 형태 → 같은 페이지를 연속으로 건드려 buffer cache 히트율 상승 (최우측 리프 삽입 fastpath 최적화도 존재)

### 수치 자료

정렬 vs 무작위 삽입 차이를 보여주는 공개 벤치마크는 UUID v4(무작위) vs v7(시간순) 비교가 대표적입니다.

- 500만 행 기준 무작위 키가 정렬 키 대비 **인덱스 크기 약 2.7배, INSERT 지연 약 4배**
  - https://iurii.rogulia.fi/blog/uuid-v7-ulid-nanoid
- 무작위 키의 페이지 분할로 인한 인덱스 비대화 분석
  - https://dev.to/umangsinha12/postgresql-uuid-performance-benchmarking-random-v4-and-time-based-v7-uuids-n9b
- B-tree 정렬 특성(공식 문서): https://www.postgresql.org/docs/current/indexes-ordering.html

단, 위 수치는 **완전 무작위 vs 완전 순차**라는 극단 비교입니다. "조회 쿼리에 ORDER BY만 추가"하는 실무 케이스의 이득은 이보다 훨씬 작습니다.

### 현재 모듈에 적용하면

1. PK 선두 컬럼이 `logtime`이면 매 수집 주기마다 값이 단조 증가하므로 삽입은 이미 인덱스 우측 끝에 몰립니다. 배치 내부를 나머지 PK 컬럼으로 정렬해도 이득이 매우 작습니다.
2. 배치 건수가 작으면(테이블스페이스 수십~수백 개 수준) 인덱스 페이지 몇 장 안에서 끝나 측정 오차 수준입니다.
3. 비용은 거의 없습니다 — "손해는 없고 이득은 상황에 따라 있음". 

### 확인 방법

정렬 유/무로 아래를 비교 측정하는 것이 가장 확실합니다.

```sql
EXPLAIN (ANALYZE, BUFFERS) ...
SELECT * FROM pg_stat_user_indexes WHERE relname = '대상테이블';
SELECT pg_relation_size('대상_pk_인덱스');
```

- pg_stat_user_indexes: https://www.postgresql.org/docs/current/monitoring-stats.html
- pg_relation_size: https://www.postgresql.org/docs/current/functions-admin.html
