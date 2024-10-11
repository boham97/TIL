# autovacuum

PG에서 update 과정

기존 row 제거, flag 변경

새 row insert

--> update 빠름



삭제 flag 있는 row 정리 필요 -> vaccum

Insert, update, delete  허용되나 alter table, create index 불가

autovacuum 수행시 최대 동시 수행 백그라운 수 제한됨

Vacuum full은 관련 인덱스도 리빌딩하며  crud query 불가

show autovacuum;

show track_counts;