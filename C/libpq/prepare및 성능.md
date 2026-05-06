# prepare 성능

[기능별 성능](https://www.tigerdata.com/blog/benchmarking-postgresql-batch-ingest)


```
[postgres@localhost bin]$ pgbench -c 1 -j 1 -T 10 -M simple -f ~/insert.sql  postgres
pgbench (17.6, server 16.13)
starting vacuum...end.
transaction type: /home/postgres/insert.sql
scaling factor: 1
query mode: simple
number of clients: 1
number of threads: 1
maximum number of tries: 1
duration: 10 s
number of transactions actually processed: 14994
number of failed transactions: 0 (0.000%)
latency average = 0.667 ms
initial connection time = 5.128 ms
tps = 1500.123960 (without initial connection time)

[postgres@localhost bin]$ pgbench -c 1 -j 1 -T 10 -M prepared -f ~/insert.sql postgres
pgbench (17.6, server 16.13)
starting vacuum...end.
transaction type: /home/postgres/insert.sql
scaling factor: 1
query mode: prepared
number of clients: 1
number of threads: 1
maximum number of tries: 1
duration: 10 s
number of transactions actually processed: 16399
number of failed transactions: 0 (0.000%)
latency average = 0.610 ms
initial connection time = 5.885 ms
tps = 1640.657984 (without initial connection time)
```

Simple 모드
- 매 트랜잭션마다 parse → bind → execute 전 과정을 반복
Prepared 모드
- 최초 1회만 parse/plan을 수행
- 이후 트랜잭션은 bind → execute만 반복
- SQL 파싱과 실행 계획 수립 오버헤드가 제거
