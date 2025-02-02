### 연결 관련
PGconn *PQconnectdb(const char *conninfo) 
- 동기식 데이터베이스 연결

PGconn *PQconnectStart(const char *conninfo) 
- 비동기식 연결 시작

PostgresPollingStatusType PQconnectPoll(PGconn *conn) 
- 비동기 연결 진행 상태 확인

void PQfinish(PGconn *conn) 
- 연결 종료

int PQstatus(const PGconn *conn) 
- 연결 상태 확인

### 쿼리 실행
PGresult *PQexec(PGconn *conn, const char *query) 
- 동기식 쿼리 실행

int PQsendQuery(PGconn *conn, const char *query) 
- 비동기식 쿼리 전송

PGresult *PQgetResult(PGconn *conn) 
- 비동기 쿼리 결과 받기

### 결과 처리
ExecStatusType PQresultStatus(const PGresult *res) 
 - 쿼리 실행 결과 상태

int PQntuples(const PGresult *res) 
- 결과 행 수

int PQnfields(const PGresult *res) 
- 결과 열 수

char *PQgetvalue(const PGresult *res, int row_number, int column_number) 
- 특정 위치의 결과값

void PQclear(PGresult *res) 
- 결과 객체 메모리 해제


### 비동기 처리 관련
`int PQsetnonblocking(PGconn *conn, int arg) `
- 비동기 모드 설정
- 0은 동기 1 비동기

`int PQisnonblocking(const PGconn *conn) `
- 비동기 모드 확인

`int PQconsumeInput(PGconn *conn) `
- 서버로부터 입력 처리
- 서버로 부터 들어오는 데이터 처리
- 입력 버퍼이 읽음

`int PQisBusy(PGconn *conn) `
- 서버가 현재 요청 처리 중인지 확인

### 소켓 관련
`int PQsocket(const PGconn *conn) `
- 연결의 소켓 파일 디스크립터 얻기
- 소켓은 `poll()` 과 `pollfd`  구조체로 감지
```c
struct pollfd {
    int fd;        // 감시할 파일 디스크립터
    short events;  // 요청하는 이벤트들
    short revents; // 발생한 이벤트들
};

// 입력 이벤트
POLLIN      // 읽을 데이터가 있음
POLLPRI     // 긴급 데이터가 있음 
POLLRDNORM  // 일반 데이터가 있음
POLLRDBAND  // 우선순위 데이터가 있음

// 출력 이벤트
POLLOUT     // 쓰기 가능
POLLWRNORM  // 일반 쓰기 가능
POLLWRBAND  // 우선순위 쓰기 가능

// 에러 이벤트 (revents로만 반환)
POLLERR     // 에러 발생
POLLHUP     // 연결이 끊김
POLLNVAL    // 잘못된 요청 (fd가 열려있지 않은 등)
```


### 상태
`char *PQerrorMessage(const PGconn *conn) `
- 에러 메시지

`char *PQdb(const PGconn *conn) `
- 현재 데이터베이스 이름

`char *PQuser(const PGconn *conn) `
- 현재 사용자 이름

