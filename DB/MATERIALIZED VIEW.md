# material view

https://www.postgresql.org/docs/current/rules-materializedviews.html
- 쿼리 결과를 실제 디스크에 저장해놓는 뷰

- 일반 view는 조회할떄 원본 테이블 퀄리를 재실행
- material view는 생성시 결과를 파일에 저장
- 오래걸린느 결과를 미리 저장해둠
- 원본 테아블이 변경되도 반영되지 않으므로 직접 갱신 필요

`REFRESH MATERIALIZED VIEW CONCURRENTLY `
- select를 막지않고 기존 데이터를 유지한채 교체함
- 더 느리고 리소스 사용량 증가
- unique index 필요


https://www.postgresql.org/docs/17/sql-refreshmaterializedview.html?utm_source=chatgpt.com
- `REFRESH MATERIALIZED VIEW completely replaces the contents of a materialized view.`
- 기존 컨텐츠를 완전히 교체함
- 동잏한 내용도 다시계산필요
