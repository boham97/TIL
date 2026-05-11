# TIL: PostgreSQL 대량 COPY 시 인덱스 DROP 후 재생성이 훨씬 빠르다

## 핵심 결론

대용량 데이터를 PostgreSQL 테이블에 적재할 때, **인덱스를 DROP → COPY → CREATE** 순서로 처리하면 인덱스 유지 상태로 COPY하는 것보다 훨씬 빠르다. 단, 소규모 테이블에서는 오히려 역효과가 날 수 있다.

---

## 공식 문서 근거

PostgreSQL 공식 문서에서도 명시적으로 권장하는 방법이다.

- **기존 테이블에 대량 적재 시**: 인덱스를 DROP하고 데이터를 적재한 후 인덱스를 재생성하는 것이 유리할 수 있다.
- **신규 테이블의 경우**: COPY 후 인덱스를 생성하는 것이 row 단위로 인덱스를 업데이트하는 것보다 빠르다.

---

## 실측 사례

### Postgres Professional 메일링 리스트

| 방식 | 소요 시간 |
|------|-----------|
| 인덱스 유지 상태로 COPY | **14시간 이상** |
| DROP → COPY → 인덱스 재생성 | **1시간 40분** |

- 조건: 4,700만 건 / 인덱스 16개 테이블

### django-postgres-copy 사례

- 수천만 건 테이블: 로드 시간 약 **1/3 단축**
- 20,000건 미만 소규모 테이블: 오히려 **느려지는 경우** 있음

---

## 적용 기준

| 데이터 규모 | 권장 방식 |
|-------------|-----------|
| 충분히 큰 경우 | DROP → COPY → CREATE 인덱스 |
| 소규모 (예: ~2만 건 미만) | 인덱스 유지 상태 COPY (기본 방식) |

컬럼 70개 5만건 copy나 insert시 몇초 차이나지 않음
한번에 저장하지 않고 나눠서 batch 처리하는경우에 1~2초내 차이남

---

## 참고

- [PostgreSQL 공식 문서 - Populating a Database](https://www.postgresql.org/docs/current/populate.html)
- Postgres Professional Mailing List
- [django-postgres-copy](https://django-postgres-copy.readthedocs.io/)