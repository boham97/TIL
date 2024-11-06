# 주요 파라미터 목록



- **`host`**: PostgreSQL 서버의 호스트 이름이나 IP 주소를 지정합니다. 기본값은 `localhost`입니다.

- **`hostaddr`**: IP 주소 형식으로 서버 주소를 지정합니다. `host`와 달리 DNS 해석이 필요하지 않습니다.

- **`port`**: 서버가 대기 중인 포트를 지정합니다. PostgreSQL의 기본 포트는 `5432`입니다.

- **`dbname`**: 연결할 데이터베이스 이름을 지정합니다. 기본값은 `postgres`입니다.

- **`user`**: 데이터베이스 사용자 이름입니다.

- **`password`**: 사용자 비밀번호입니다.

- **`connect_timeout`**: 연결 시도 제한 시간을 초 단위로 지정합니다.

- **`options`**: 서버에 전달할 명령줄 옵션을 지정합니다. 예를 들어, `-c geqo=off`와 같은 형식입니다.

- **`sslmode`**: SSL 연결 방식을 지정합니다.
  
  - `disable`: SSL 사용 안 함
  - `allow`: 가능하면 SSL 사용
  - `prefer`: SSL 우선 사용, 불가 시 일반 연결
  - `require`: SSL 필수
  - `verify-ca`: 서버 인증서 CA 확인 필수
  - `verify-full`: 서버 호스트 이름까지 확인

- **`sslcert`**: 클라이언트 SSL 인증서 파일 경로를 지정합니다.

- **`sslkey`**: 클라이언트 SSL 개인 키 파일 경로입니다.

- **`sslrootcert`**: 신뢰할 수 있는 루트 CA 인증서 파일 경로입니다.

- **`sslcrl`**: 인증서 취소 목록 파일 경로입니다.

- **`gssencmode`**: GSSAPI 암호화 모드를 지정합니다. (`disable`, `prefer`, `require`)

- **`service`**: `pg_service.conf` 파일에 정의된 서비스 이름을 사용하여 연결 매개변수를 간단히 지정할 수 있습니다.

- **`application_name`**: 이 연결을 나타내는 애플리케이션 이름을 지정합니다.

- **`client_encoding`**: 연결 인코딩을 설정합니다. 예: `UTF8`, `EUC-KR` 등.

- **`fallback_application_name`**: `application_name`이 설정되지 않은 경우 사용할 기본 애플리케이션 이름입니다.

- **`target_session_attrs`**: 연결하려는 서버의 속성을 지정합니다. (`read-write`, `read-only`, `primary`, `standby`)

- **`load_balance_hosts`**: 여러 호스트를 사용할 경우, 연결 시도 순서를 제어합니다. `random` 값을 설정하면 호스트 목록을 무작위로 시도합니다.
  
  - `disable` 순서대로

- **`requirepeer`**: Unix 소켓을 통한 연결 시 요구되는 피어 인증 사용자 이름입니다.

- **`keepalives`**: TCP 연결 유지 기능을 설정합니다. `0` 또는 `1`로 설정합니다.

- **`keepalives_idle`**: TCP 연결 유휴 시간을 초 단위로 지정합니다.

- **`keepalives_interval`**: TCP 연결 유지 알림 전송 간격을 초 단위로 지정합니다.

- **`keepalives_count`**: TCP 연결 유지 시도 횟수입니다.

- **`tcp_user_timeout`**: TCP 소켓의 데이터 전송 대기 시간입니다.

- **`replication`**: 연결을 복제 모드로 설정합니다. 값을 `database`, `true`, `1`, `on`, `yes`로 지정하면 복제 전용 연결이 됩니다.
  
  - 데이터 복제 및 감사 시스템?
