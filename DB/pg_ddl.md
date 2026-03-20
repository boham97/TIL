PG DDL과 트랜잭션 보상

PG의 DDL은 트랜잭션 안에서 동작
sqlBEGIN;
CREATE TABLE test (id int);
INSERT INTO test VALUES (1);
ROLLBACK;  -- CREATE TABLE도 같이 롤백됨
Oracle과 달리 PG는 DDL도 트랜잭션으로 묶임 → 롤백 가능