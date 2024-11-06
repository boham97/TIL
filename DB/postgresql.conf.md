### 1. **Connection and Authentication Settings (연결 및 인증 설정)**

- `listen_addresses` : 서버가 수신할 IP 주소 설정. 예: `'localhost'` 또는 `'*'` (모든 주소).
- `port` : 서버가 수신하는 포트 번호 (기본값: `5432`).
- `max_connections` : 허용 가능한 최대 동시 연결 수.
- `superuser_reserved_connections` : 슈퍼유저 연결을 위한 예약된 연결 수.
- `authentication_timeout` : 클라이언트 인증 제한 시간.

### 2. **Resource Usage (리소스 사용 설정)**

- `shared_buffers` : 공유 메모리 버퍼 크기 설정. 일반적으로 시스템 메모리의 1/4로 설정.
- `work_mem` : 정렬, 해시 작업 등의 임시 작업에 할당되는 메모리 양.
- `maintenance_work_mem` : 유지 관리 작업에 사용되는 메모리 크기 (예: VACUUM, CREATE INDEX).
- `max_worker_processes` : 사용 가능한 최대 작업자 프로세스 수.

### 3. **Write Ahead Log (WAL) Settings (WAL 설정)**

- `wal_level` : WAL 기록의 상세 수준. 일반적으로 `replica` 또는 `logical`.
- `fsync` : 모든 데이터 변경 후 파일 동기화 여부 설정. 데이터 안정성을 위해 `on`이 권장.
- `synchronous_commit` : 트랜잭션 커밋 동기화 모드.
- `wal_buffers` : WAL 버퍼 크기 설정.
- `checkpoint_timeout` : 체크포인트가 발생하는 시간 간격.

### 4. **Replication Settings (복제 설정)**

- `max_wal_senders` : WAL 발신기 프로세스의 최대 수. 복제 서버 사용 시 설정 필요.
- `wal_keep_size` : 복제 클라이언트를 위한 WAL 파일 유지 크기.
- `hot_standby` : 읽기 전용 복제본에서 쿼리 허용 여부.
- `primary_conninfo` : 스트리밍 복제를 위한 기본 서버 연결 정보.

### 5. **Query Tuning (쿼리 튜닝)**

- `effective_cache_size` : 쿼리 플래너가 사용 가능한 캐시 메모리 추정치.
- `random_page_cost` : 비순차 디스크 읽기의 상대적 비용.
- `cpu_tuple_cost` : 튜플(행) 하나를 처리하는 데 걸리는 CPU 비용.
- `default_statistics_target` : 통계 수집 샘플 크기. 더 높은 값은 더 나은 쿼리 계획을 제공할 수 있지만 성능에 영향을 줄 수 있음.

### 6. **Logging Settings (로깅 설정)**

- `logging_collector` : PostgreSQL이 독립적인 로그 수집 프로세스를 사용할지 여부.
- `log_directory` : 로그 파일이 저장되는 디렉터리.
- `log_filename` : 로그 파일 이름 형식.
- `log_min_duration_statement` : 로그에 기록할 쿼리의 최소 실행 시간 (밀리초).
- `log_statement` : 로그에 기록할 SQL 명령의 유형 (예: `none`, `ddl`, `mod`, `all`).

### 7. **Autovacuum (오토 베큠)**

- `autovacuum` : 자동 진공 프로세스 사용 여부.
- `autovacuum_naptime` : 자동 진공 주기 간격.
- `autovacuum_vacuum_threshold` : 진공 시작 전 변경된 최소 튜플 수.
- `autovacuum_analyze_threshold` : 분석 시작 전 변경된 최소 튜플 수.

### 8. **Client Connection Defaults (클라이언트 연결 기본값)**

- `timezone` : 타임존 설정.
- `datestyle` : 날짜 형식 설정.
- `lc_messages` : 서버 메시지의 언어.
- `lc_time` : 시간 표시 형식 언어.

### 9. **Security and Encryption (보안 및 암호화)**

- `ssl` : SSL 연결 사용 여부.
- `ssl_cert_file` 및 `ssl_key_file` : SSL 인증서와 키 파일 경로.
- `password_encryption` : 암호 저장 방식 (예: `md5`, `scram-sha-256`).
