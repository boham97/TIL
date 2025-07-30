UNLOGGED 테이블은 PostgreSQL에서 WAL(Write-Ahead Logging) 을 사용하지 않는 테이블

```
CREATE UNLOGGED TABLE my_temp_table (
    id SERIAL PRIMARY KEY,
    data TEXT
);
```


WAL에 기록되지 않기 때문에 쓰기 성능이 매우 빠름.

PostgreSQL이 재시작(crash 후 재기동 포함)되면 내용이 모두 사라짐.

복구(recovery), 복제(replication) 에는 포함되지 않음.



사용처
캐시 테이블

임시 결과 저장용

빠른 insert/update가 필요하고, 데이터 유실이 용인되는 경우

예: ETL 파이프라인 중간 단계

MVCC 트랜잭션 처리대상이므로 롤백 커밋 가능

크래시되면 복구 불가 
