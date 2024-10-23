# slow query test

1. table 생성후 insert
2. auto commit 끈 세션에서 update
3. 새로운 세션에서도 update 쿼리 실행
4. lock이 걸려 실행 지연됨 -> slow query