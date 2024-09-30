https://blog.naver.com/shsoul12/10123250467

# postgres 디비 설정은 세션 별로 저장될수있다!!!

# **PostgreSQL 8.3.3문서**

`PQexec`

커맨드를 서버에 전송하고, 결과를 대기합니다.
`PGresult *PQexec(PGconn *conn, const char *command);`
반환되는 것은 `PGresult` 포인터나 null 포인터

- 메모리 부족 상태이나 서버의 커맨드 전송이 불가능한 것과 같이 심각한 에러의 경우 NULL
- 반환 값이 null 포인터의 경우에는 `PGRES_FATAL_ERROR` 결과와 같이 다루어져야 합니다.
- 자세한 정보는 `PQerrorMessage` 로 

커맨드 문자열에는(세미콜론으로 분리된) 복수의 SQL 커맨드를 포함
단일의`PQexec`호출로 전송된 복수의 쿼리는 하나의 트랜잭션으로 처리
쿼리 문자열 내에 명시적인`BEGIN`/`COMMIT`가 있는 경우는 복수의 트랜잭션으로 분리
`PGresult`구조는 마지막에 실행된 커맨드의 결과만 포함

`PQexecParams`

서버에 커맨드를 전송하고, 결과를 기다립니다.다만, SQL 커맨드 텍스트와는 별도로 파라미터를 건네줄 수 있습니다.

`PGresult *PQexecParams(PGconn *conn, const char *command,                                      _                         int nParams, const Oid *paramTypes,                       _                       const char * const *paramValues,                       _                        const int *paramLengths,
                       _const int *paramFormats,  
                         int resultFormat);`
파라미터 값을 커맨드 문자열과는 별도로 적절히 지정
쿼리의 결과를 텍스트나 바이너리 서식으로 요구가능
`PQexecParams`는 프로토콜 3.0이후에서만 지원
`conn`접속 오브젝트입니다. 이것을 통해 커맨드를 전송합니다.
`command`실행되는 SQL 커맨드 문자열입니다.파라미터가 사용되는 경우는 커맨드 문자열 내에서`$1`, `$2`로서 참조됩니다.
`nParams`제공하는 파라미터 숫자입니다.이것은 배열`paramTypes[]`, `paramValues[]`, `paramLengths[]`, `paramFormats[]`의 길이입니다. 

**`paramValues[]`**:

- 실제 파라미터 값들을 지정하는 배열입니다.
- `NULL` 포인터는 해당 파라미터가 `NULL`임을 의미합니다.
- 값은 0으로 끝나는 텍스트 문자열(텍스트 서식) 또는 바이너리 데이터(바이너리 서식)로 지정할 수 있습니다.

**`paramLengths[]`**:

- 바이너리 서식의 파라미터에 대한 실제 데이터 길이를 지정하는 배열입니다.
- 텍스트 서식의 파라미터나 `NULL` 파라미터는 이 배열에서 무시됩니다.
- 바이너리 파라미터가 없다면, 이 배열 포인터는 `NULL`이어도 됩니다.

**`paramFormats[]`**:

- 각 파라미터가 텍스트(0)인지 바이너리(1)인지 지정하는 배열입니다.
- 이 배열이 `NULL`이면 모든 파라미터는 텍스트 문자열로 간주됩니다.

**`paramTypes[]`**:

- 파라미터의 데이터 유형을 OID로 지정하는 배열입니다.
- `NULL`이거나 특정 배열 요소가 0일 경우, 서버는 리터럴 문자열에 대한 추정 방법을 사용하여 데이터 유형을 결정합니다.

**`resultFormat`**:

- 쿼리 결과의 포맷을 지정합니다.
- `0`을 지정하면 텍스트 서식으로, `1`을 지정하면 바이너리 서식으로 결과를 반환받습니다.

`PQexec`에 대한`PQexecParams`의 주된 이점은 커맨드 문자열로부터 파라미터 값이 분리 가능하다는 점입니다.이것에 의해, 지루한 에러를 부르기 쉬운 인용부호나 이스케이프 처리를 행할 필요가 없어집니다.

`PQexec`와 달리, `PQexecParams`는 문자열 내에 최대 1개의 SQL 커맨드를 넣을 수 있습니다. (세미콜론을 넣을 수 있지만, 공백이 아닌 커맨드를 1개 이상 넣을 수 없습니다. )

- `PQprepare`
  
  지정 파라미터를 가지는 준비된 문장의 생성 요구를 전송하고, 그 완료를 기다립니다.`PGresult *PQprepare(PGconn *conn,
  
                      const char *stmtName,
                      const char *query,
                      int nParams,
                      const Oid *paramTypes);`
  
  `PQprepare`는 다음에`PQexecPrepared`를 사용해 실행할 준비된 문장을 생성합니다.이 기능을 사용해서 반복 사용되는 커맨드의 해석과 계획 생성을 실행할 때마다 매번 실시하는 것이 아니라, 1회만 실시하도록 할 수 있습니다.`PQprepare`는 프로토콜 3.0이후에서만 지원되므로 프로토콜 2.0을 사용하고 있는 경우는 실패합니다.이 함수는 `query`문자열으로부터`stmtName`라는 이름의 준비된 문장을 생성합니다.`query`는 단일의 SQL 커맨드가 아니면 안됩니다.`stmtName`를`""`로 해, 이름 없는 문장을 생성할 수 있습니다.만약, 이름 없는 문장이 이미 존재하고 있을 경우는 자동으로 교체됩니다.그 외의 경우, 문장 이름이 현재의 세션에 이미 정의되어 있으면 에러가 납니다.어떠한 파라미터가 사용되는 경우, 이들은 쿼리 내에서 `$1`, `$2` 등으로 참조됩니다.`nParams`는 파라미터 숫자입니다.그 형태에 대해서는 사전에`paramTypes[]`배열로 지정되고 있습니다. (`nParams`가 0일 경우, 이 배열 포인터는 `NULL`로 할 수 있습니다. )`paramTypes[]`는 OID에 의해 파라미터 심볼에 할당하는 데이터형을 지정합니다.`paramTypes`가`NULL`이거나 배열 내의 특정 요소가 0일 경우, 서버는 그 파라미터 심볼에 대해, 형태 지정이 없는 리터럴 문자열에 대한 처리와 같은 방법으로 데이터형을 할당합니다.또한, 쿼리에서는 `nParams`보다 많은 파라미터 심볼을 사용할 수 있습니다.이러한 심볼에 대한 데이터형도 이와 같이 추측됩니다. (어떠한 데이터형이 추측되는지를 검출하는 방법에 대해서는 `PQdescribePrepared`를 참조해 주십시요. )`PQexec`같이, 결과는 보통`PGresult`오브젝트로, 그 내용은 서버측의 성공이나 실패를 나타냅니다.null 결과는 메모리 부족이나 전혀 커맨드를 전송할 수 없었던 것을 나타냅니다.이러한 에러의 세부 사항 정보를 입수하려면`PQerrorMessage`를 사용해 주십시요.

`PQexecPrepared`로 사용하기 위한 준비된 문장은 [*PREPARE*](http://www.postgresplus.co.kr/man/sql-prepare.html) SQL문을 실행하는 것도 생성 가능합니다. (그러나 `PQprepare`는 파라미터의 형태를 사전에 정의할 필요가 없기 때문에, 보다 유연성이 있습니다. )또한, 준비된 문장을 삭제하는 libpq함수는 없지만, 이 목적을 위해서[*DEALLOCATE*](http://www.postgresplus.co.kr/man/sql-deallocate.html)SQL문을 사용할 수 있습니다.

- `PQexecPrepared`
  
  지정 파라미터를 가진 준비된 문장의 실행 요구를 전송하고, 결과를 기다립니다.
  `PGresult *PQexecPrepared(PGconn *conn,
  
                           const char *stmtName,
                           int nParams,
                           const char * const *paramValues,
                           const int *paramLengths,
                           const int *paramFormats,
                           int resultFormat);`
  
  `PQexecPrepared`와 `PQexecParams`는 유사하지만, 전자에서 실행되는 커맨드는 쿼리 문자열을 주는 것이 아니라, 사전에 준비된 문장을 지명하는 것으로 지정됩니다.이 기능은 커맨드가 실행될 때마다가 아닌 반복적으로 사용될 커맨드의 분석, 계획할 때 한번만 실행할 수 있습니다.이 문장은 현재의 세션으로 사전에 준비되어 있지 않으면 안됩니다.`PQexecPrepared`는 프로토콜 3.0이후의 접속에서만 서포트됩니다.프로토콜 2.0으로 사용했을 경우는 실패합니다.파라미터는 쿼리 문자열 대신에 주어진 준비된 문장의 이름을 주는 점을 제외하고, `PQexecParams`와 같습니다.또한, `paramTypes[]`파라미터는 존재하지 않습니다. (준비된 문장의 파라미터형은 그 생성 시점에서 결정되기 때문에, 이것은 불필요합니다. )

- `PQdescribePrepared`
  
  주어진 준비된 문장에 관한 정보 입수 요구를 보내고, 완료될 때까지 대기합니다.`PGresult *PQdescribePrepared(PGconn *conn, const char *stmtName);``PQdescribePrepared`에 의해, 어플리케이션은 사전에 준비된 문장에 관한 정보를 입수할 수 있습니다.`PQdescribePrepared`는 프로토콜 3.0이후의 접속에서만 서포트됩니다.프로토콜 2.0으로 사용하면 실패합니다.`stmtName`를`""`또는 NULL로 하는 것으로, 이름 없는 문장을 참조할 수 있습니다.이외에는 존재하는 준비된 문장의 이름이 아니면 안됩니다.성공하면, `PGRES_COMMAND_OK` 상태의 `PGresult`가 반환됩니다.함수`PQnparams` 및 `PQparamtype`를 이`PGresult`에 적용해, 준비된 문장의 파라미터에 관한 정보를 얻을 수 있습니다.또한, 함수`PQnfields`, `PQfname`, `PQftype`등을 사용하고, 문장의 결과열(만약 있으면)에 관한 정보를 제공할 수 있습니다.

- `PQdescribePortal`
  
  지정한 포털에 관한 정보 입수 요구를 전송하고, 완료될 때까지 대기합니다.
  `PGresult *PQdescribePortal(PGconn *conn, const char *portalName);``PQdescribePortal`에 의해, 어플리케이션은 사전에 생성된 포털의 정보를 입수할 수 있습니다. (libpq는 포털에 직접 액세스 하는 방법을 제공하고 있지만, 이 함수를 사용해`DECLARE CURSOR` SQL 커맨드로 생성한 커서의 속성을 확인할 수 있습니다. )`PQdescribePortal`는 프로토콜 3.0이후의 접속에서만 서포트됩니다.프로토콜 2.0으로 사용하면 실패합니다.`portalName`에`""`또는 NULL를 지정해, 이름 없는 포털을 참조할 수 있습니다.이외에서는 존재하는 포털의 이름이 아니면 안됩니다.성공하면, `PGRES_COMMAND_OK` 상태의`PGresult`가 반환됩니다.함수`PQnfields`, `PQfname`, `PQftype`등을 이`PGresult`에 적용해, 포털의 결과열(만약 있으면)에 관한 정보를 얻을 수 있습니다.

`PGresult`구조는 서버로부터 리턴된 결과를 캡슐화합니다.libpq어플리케이션 프로그래머는 주의해서 `PGresult` 추상화를 유지해 주십시요.이하의 액세서 함수를 사용하고, `PGresult`의 내용을 꺼내 주십시요.향후 변경에 영향을 받기 때문에, `PGresult`구조의 필드를 직접 참조하는 것은 피해 주십시요.

- `PQresultStatus`
  
  커맨드의 결과 상태를 반환합니다.`ExecStatusType PQresultStatus(const PGresult *res);``PQresultStatus`는 이하의 몇 개의 값을 반환합니다.`PGRES_EMPTY_QUERY`서버에 전송된 문자열이 비었습니다.`PGRES_COMMAND_OK`아무런 데이터도 리턴하지 않는 커맨드가 정상 종료했습니다.`PGRES_TUPLES_OK`데이터를 반환하는 커맨드(`SELECT`나 `SHOW`등)가 정상 종료했습니다.`PGRES_COPY_OUT`(서버로부터의) 복사 아웃 데이터 전송이 시작되었습니다.`PGRES_COPY_IN`(서버로의) 복사 인 데이터 전송이 시작되었습니다.`PGRES_BAD_RESPONSE`서버의 반응을 이해할 수 없습니다.`PGRES_NONFATAL_ERROR`치명적이지 않는(주의 환기 혹은 경고) 에러가 발생했습니다.`PGRES_FATAL_ERROR`치명적인 에러가 발생했습니다.결과 상태가`PGRES_TUPLES_OK`이면, 이하에 설명하는 함수를 사용해 쿼리가 돌려준 행을 꺼낼 수 있습니다. 다만, 우연히`SELECT`커맨드가 돌려주는 행이 0개였던 것 같은 경우에서도`PGRES_TUPLES_OK`가 되는 것에 주의하십시요.`PGRES_COMMAND_OK`는 행을 전혀 돌려주지 않는(`INSERT`, `UPDATE`등의) 커맨드용입니다.`PGRES_EMPTY_QUERY` 응답은 클라이언트 소프트웨어의 불편을 나타내고 있을지도 모릅니다.`PGRES_NONFATAL_ERROR`상태의 결과는 `PQexec`나 다른 쿼리 실행 함수에 의해 직접 반환되지 않습니다.그 대신에, 이런 종류의 결과는 주의 프로세서([Section 29.11](http://www.postgresplus.co.kr/man/libpq-notice-processing.html)참조)에게 건네집니다.

- `PQresStatus`
  
  `PQresultStatus`가 돌려주는 열거형으로부터 상태 코드를 설명하는 문자열 정수로 변환합니다.호출하는 측은 이 결과를 해제해서는 안됩니다.`char *PQresStatus(ExecStatusType status);`

- `PQresultErrorMessage`
  
  커맨드에 관한 에러 메세지를 반환합니다. 에러가 없으면, 비어있는 문자열을 반환합니다.`char *PQresultErrorMessage(const PGresult *res);`에러가 있었을 경우, 반환되는 문자열의 마지막에는 새로운 행이 포함됩니다.호출하는 측은 이 결과를 직접 해제해서는 안됩니다.관련하는 `PGresult`핸들이`PQclear`에게 건네졌을 때 해제됩니다.(접속에서)`PQerrorMessage`도, `PQexec` 또는 `PQgetResult`호출 직후라면 (결과로)`PQresultErrorMessage`와 같은 문자열을 반환합니다. 그러나 접속에 대한 에러 메세지는 연속되는 조작을 실행하면 변경되는데 반해, `PGresult`는 오브젝트가 파기될 때까지 그 에러 메세지를 계속 유지합니다. 이`PQresultErrorMessage`는 개개의`PGresult`에 연관된 상태를 보고 싶을 때, 그리고`PQerrorMessage`는 접속에 있어서의 마지막 조작 상태를 보고 싶을 때 사용해 주십시요.

- `PQresultErrorField`
  
  에러 보고에서 독립적인 필드를 반환합니다.`char *PQresultErrorField(const PGresult *res, int fieldcode);``fieldcode`는 에러 필드 식별자입니다.이하에 나타내는 심볼을 참조해 주십시요.`PGresult`가 에러가 아니거나 경고 첨부의 결과인 경우, 지정한 필드를 포함하지 않는 경우, `NULL`가 반환됩니다.보통 필드 값은 새로운 행이 포함되지 않습니다.필드치는 관련하는 `PGresult`핸들이`PQclear`에게 건네졌을 때 개방됩니다.이하의 필드 코드를 사용할 수 있습니다.`PG_DIAG_SEVERITY`심각도.이 필드의 내용은(에러 메세지의 경우)`ERROR`, `FATAL`, 혹은 `PANIC`, (주의 환기 메세지의 경우)`WARNING`, `NOTICE`, `DEBUG`, `INFO`, 혹은 `LOG`입니다.이것들은 다언어화에 의해 번역될 가능성이 있습니다.항상 존재합니다.`PG_DIAG_SQLSTATE`에러의 SQLSTATE 코드입니다.SQLSTATE 코드는 발생한 에러의 종류를 식별합니다.프론트엔드 어플리케이션에 의해, 특정한 데이터베이스 에러에 대해서 특정한 조작(에러 처리등)을 실시하기 위해 사용할 수 있습니다.일어날 수 있는 SQLSTATE 코드의 리스트에 대해서는 [Appendix A](http://www.postgresplus.co.kr/man/errcodes-appendix.html)을 참조해 주십시요.이 필드는 다언어화 되지 않으며, 항상 존재합니다.`PG_DIAG_MESSAGE_PRIMARY`주로 가독성을 높인 에러 메세지입니다. (보통은 1행입니다. )항상 존재합니다.`PG_DIAG_MESSAGE_DETAIL`세부 사항:문제에 관해 보다 세부 사항을 나타내는 보조적인 에러 메세지입니다.복수행으로 실행될 가능성이 있습니다.`PG_DIAG_MESSAGE_HINT`힌트:문제의 처리하기 위한 선택적인 제안입니다.이는 세부 사항(detail)과는 달리, 문제의 사상보다는 (적절하지 않을 가능성이 있지만) 조언을 제공하는 것을 목적으로 하고 있습니다.복수행으로 실행될 가능성이 있습니다.`PG_DIAG_STATEMENT_POSITION`원래의 쿼리 문자열의 인덱스로서 에러가 발생한 커서를 나타내는 10진정수를 가지는 문자열입니다.선두 문자가 인덱스 1이 되고, 바이트가 아닌 문자 수로 측정된 위치입니다.`PG_DIAG_INTERNAL_POSITION`이 정의는 `PG_DIAG_STATEMENT_POSITION`필드와 같습니다.그러나 이는 클라이언트가 발행한 커맨드는 아니고, 커서 위치가 내부 생성 커맨드를 참조하는 경우에 사용됩니다.이 필드가 존재할 때는 항상`PG_DIAG_INTERNAL_QUERY`필드가 존재합니다.`PG_DIAG_INTERNAL_QUERY`실패한 내부 생성 커맨드의 텍스트입니다.이는 예를 들면, PL/pgSQL 함수로 발행된 SQL 쿼리가 됩니다.`PG_DIAG_CONTEXT`에러가 발생한 문맥을 나타내는 것입니다.현재 이것은 활동 중의 수속 언어 함수나 내부 생성 쿼리의 호출 스택의 추적 정보를 포함합니다.이 추적은 행마다 1항목이며, 그 차례는 대부분 최근 이용한 것이 처음으로 옵니다.`PG_DIAG_SOURCE_FILE`에러가 보고된 소스 코드의 파일명입니다.`PG_DIAG_SOURCE_LINE`에러가 보고된 소스 코드의 행 번호입니다.`PG_DIAG_SOURCE_FUNCTION`에러를 보고한, 소스 코드의 함수명입니다.요구를 충족하는 표시된 정보의 형식은 클라이언트의 책임입니다.구체적으로 필요에 따라 긴 행을 분할합니다.에러 메세지 필드 내 새로운 행 문자는 새로운 행으로가 아닌 단락으로서 나뉜 것으로서 취급해야 합니다.libpq로 내부에서 생성된 에러는 심각도와 주요 메세지를 가지지만, 보통은 다른 필드를 가지지 않습니다.3.0 이전 프로토콜의 서버로 반환되는 에러는 심각도와 주요 메세지, 경우에 따라 세부 사항 메세지를 가지지만, 다른 필드를 가지지는 않습니다.에러 필드는 `PGresult`로부터 이용할 수 있으나 `PGconn`로부터는 이용할 수 없습니다.`PQerrorField`라고 하는 함수는 없습니다.

- `PQclear`
  
  `PGresult` 에 할당할 수 있었던 저장 영역을 해제합니다.개개의 쿼리 결과는 더이상 필요하지 않으면, `PQclear`로 해제해야 합니다.`void PQclear(PGresult *res);``PGresult`오브젝트는 필요한 만큼 보관할 수 있습니다. 새로운 쿼리를 발행하는 경우에서도, 접속을 닫아 버릴 때까지는 `PGresult`는 사라지지 않습니다. `PGresult`를 해제하려면 , `PQclear`를 호출해야 합니다. 그 조작에 실패하면, 어플리케이션의 메모리 리크를 일으켜 버립니다.

- `PQmakeEmptyPGresult`
  
  주어진 상태를 가진, 비어있는 `PGresult`오브젝트를 생성합니다.`PGresult *PQmakeEmptyPGresult(PGconn *conn, ExecStatusType status);`이것은 비어있는 `PGresult` 오브젝트를 할당하고 초기화하는 libpq의 내부 함수입니다. 이 함수는 메모리를 확보할 수 없으면, NULL를 반환합니다.일부 어플리케이션에서는 자신이 `PGresult`오브젝트(특히 에러 상태를 포함한 오브젝트)를 생성할 수 있으면 유용하기 때문에, 이 함수가 export 되고 있습니다.`conn`가 NULL이 아니고, `status`가 에러를 나타내고 있는 경우, 접속의 현재 에러 메세지가`PGresult`에 복사됩니다.덧붙여libpq자체가 돌려주는 `PGresult`와 같이, 마지막에`PQclear`를 이 오브젝트에 대해서 호출하지 않으면 안 되는 것에 주의하십시요.

## 29.3.2. 쿼리 결과 정보를 꺼냄

이러한 함수를 사용하고, 정상 종료한 쿼리 결과를 나타내는(즉, `PGRES_TUPLES_OK`상태를 가진) `PGresult`오브젝트로부터 정보를 추출할 수 있습니다.또한, 성공한 Describe 조작으로부터 정보를 추출할 수도 있습니다.Describe의 결과는 모두, 실제로 쿼리를 실행했을 때 제공되는 것과 같은 열정보를 가집니다만, 행은 없습니다.다른 상태 값을 가지는 오브젝트에서는 이러한 함수는 결과가 0행 0열으로 동작합니다.

- `PQntuples`
  
  쿼리 결과 내의 행(튜플) 수를 반환합니다.`int PQntuples(const PGresult *res);`

- `PQnfields`
  
  쿼리 결과의 각 행의 열(필드)의 수를 반환합니다.`int PQnfields(const PGresult *res);`

- `PQfname`
  
  지정한 열번호에 대응하는 열의 이름을 반환합니다.열번호는 0으로 시작합니다.호출한 측에서는 이 결과를 직접 해제해서는 안됩니다.관련하는 `PGresult`핸들이`PQclear`에게 건네졌을 때 이것은 해제됩니다.`char *PQfname(const PGresult *res,              int column_number);`열번호가 범위 밖일 경우, `NULL` 가 돌아갑니다.

- `PQfnumber`
  
  지정한 열명에 관련되는 열번호를 반환합니다.`int PQfnumber(const PGresult *res,              const char *column_name);`지정한 이름에 일치하는 열이 없으면,-1이 리턴됩니다.지정한 이름은 SQL 커맨드의 식별자처럼 다루어집니다.즉, 이중 인용부호되지 않는 한, 소문자가 됩니다.예를 들면, 이하의 SQl로 생성된 쿼리 결과를 생각해 봅시다.`select 1 as FOO, 2 as "BAR";`이하에 의해, 결과를 가질 수 있습니다.`PQfname(res, 0)              *foo* PQfname(res, 1)              *BAR* PQfnumber(res, "FOO")        *0* PQfnumber(res, "foo")        *0* PQfnumber(res, "BAR")        *-1* PQfnumber(res, "\"BAR\"")    *1* `

- `PQftable`
  
  지정한 열이 추출된 테이블의 OID를 반환합니다.열번호는 0으로 시작합니다.`Oid PQftable(const PGresult *res,             int column_number);`열번호가 범위를 벗어나거나 지정한 열이 테이블 열의 단순 참조가 아닌 경우, 또는 3.0 전의 프로토콜을 사용하고 있는 경우는 `InvalidOid`가 반환됩니다.`pg_class`시스템 테이블에 쿼리해 어느 테이블이 참조되고 있는지를 정확하게 요구할 수 있습니다.libpq헤더 파일을 포함하면, `Oid`형과 `InvalidOid`정수가 정의됩니다.이들은 모두 임의의 정수형입니다.

- `PQftablecol`
  
  지정한 쿼리 결과 열을 생성한 열의(그것이 속하는 테이블 내에서의) 열번호를 반환합니다.쿼리 결과의 열번호는 0으로 시작합니다.테이블열은 0 이외의 숫자를 가집니다.`int PQftablecol(const PGresult *res,                int column_number);`열번호가 범위를 벗어나거나 지정한 열이 테이블열의 단순 참조가 아니닌 경우, 또는 3.0 이전의 프로토콜을 사용하고 있는 경우 0이 반환됩니다.

- `PQfformat`
  
  지정한 열의 서식을 나타내는 서식 코드가 리턴됩니다.열번호는 0으로 시작합니다.`int PQfformat(const PGresult *res,              int column_number);`0을 가리키는 서식 코드는 텍스트 데이터 표현을 나타내, 1이라고 하는 서식 코드는 바이너리 표현을 나타냅니다. (다른 코드는 향후 정의를 위해 예약되었습니다. )

- `PQftype`
  
  지정한 열번호에 관련한 데이터형을 반환합니다.반환된 정수는 그 형태의 내부 OID 번호입니다.열번호는 0으로 시작합니다.`Oid PQftype(const PGresult *res,            int column_number);``pg_type`시스템 테이블에 쿼리해, 각종 데이터형의 이름이나 속성을 얻을 수 있습니다.내장 데이터형의OID는 소스 트리의 `src/include/catalog/pg_type.h`파일내에서 정의되고 있습니다.

- `PQfmod`
  
  지정한 열번호에 관련한 열의 형태 수정자(modifier)를 반환합니다.열번호는 0으로 시작합니다.
  `int PQfmod(const PGresult *res,
  
             int column_number);`
  
  수정자 값의 해석은 형태에 고유한 것입니다.보통, 이것들은 정밀도나 크기의 제약을 나타냅니다.-1 값은"사용할 수 있는 정보가 없는 "것을 나타냅니다.대부분의 데이터형은 수정자를 사용하지 않습니다. 이 경우는 항상-1이라고 하는 값이 됩니다.

- `PQfsize`
  
  지정한 열번호에 관련한 열의 바이트 단위의 크기를 반환합니다.열번호는 0으로 시작합니다.`int PQfsize(const PGresult *res,
  
              int column_number);
  
  ``PQfsize`는 데이터베이스 은행 내에서 그 열을 위해 할당할 수 있는 영역을 반환합니다.바꿔 말하면, 그 데이터형에 대한 서버에서의 내부 표현의 크기입니다. (따라서, 실제로는 클라이언트에게는 많이 유용하지 않습니다.)음의 값은 가변 길이 데이터형을 나타냅니다.

- `PQbinaryTuples`
  
  `PGresult`가 바이너리 데이터를 가지는 경우는 1을, 텍스트 데이터를 가지는 경우는 0을 반환합니다.
  `int PQbinaryTuples(const PGresult *res);`
  이 함수는 쓸모없게 된 것입니다. (`COPY`를 가지는 접속의 사용을 제외합니다. )단일의`PGresult`가 어느 열은 텍스트 데이터를 포함하고, 다른 열에서 바이너리 데이터를 가지는 것이 가능하기 때문에입니다.`PQfformat` 의 이용이 추천됩니다.결과의 모든 열이 바이너리(서식 1)일 경우만 `PQbinaryTuples`가 1을 반환합니다.

- `PQgetvalue`
  
  `PGresult`의 1행에 있어서의 단일 필드의 값을 반환합니다.행 번호와 열번호는 0으로 시작합니다.호출하는 측은 이 결과를 직접 해제해서는 안됩니다.관련하는 `PGresult`핸들이`PQclear`에게 건네졌을 때 이것은 해제됩니다.
  `char *PQgetvalue(const PGresult *res,
  
                   int row_number,
                   int column_number);`
  
  텍스트 서식의 데이터에서는 `PQgetvalue`로 반환되는 값은 필드 값을 null로 끝나는 문자열로 표현됩니다.바이너리 서식의 데이터에서 이 값은 데이터형의`typsend`함수와`typreceive`함수로 정해지는 바이너리 표현이 됩니다. (실제로는 이 경우에서도 값의 마지막에 0 바이트가 부여됩니다.그러나 이 값의 내부에는 대체로 null을 포함하기 때문에, 보통 이는 유용하지 않습니다. )필드 값이 NULL의 경우, 비어있는 문자열이 반환됩니다.NULL값과 공문자열의 값을 구별하는 방법은`PQgetisnull`를 참조해 주십시요.`PQgetvalue`에 의해 반환되는 포인터는 `PGresult`구조의 일부 저장 영역을 지시합니다.이 포인터가 지시하는 데이터를 변경해야 하지는 않습니다.또한, `PGresult`구조의 존속 기간이 지난 후에도 사용하는 경우, 데이터를 다른 저장 영역에 명시적으로 복사해야 합니다.

- `PQgetisnull`
  
  필드가 null 값인지 검사합니다.행 번호와 열번호는 0으로 시작합니다.
  `int PQgetisnull(const PGresult *res,
  
                  int row_number,
                  int column_number);`
  
  이 함수는 필드가 NULL인 경우에 1을, 필드가 NULL이 아닌 값를 가지는 경우는 0을 반환합니다. (`PQgetvalue`에서는 NULL 필드는 null 포인터가 아닌 공문자열을 리턴하는 것을 주의하십시요. )

- `PQgetlength`
  
  실제 필드 값의 길이를 바이트 단위로 반환합니다.행 번호와 열번호는 0으로 시작합니다.`int PQgetlength(const PGresult *res,                int row_number,                int column_number);`이것은 특정한 데이터 값에 대한 실제 데이터 길이입니다.즉, `PQgetvalue`에 의해 지시되는 오브젝트의 크기입니다.텍스트 데이터 서식에서는 `strlen()`와 동일합니다.바이너리 서식에서는 이것은 중요한 정보입니다.실제 데이터 길이를 꺼내기 위해서`PQfsize`를 의존*하지는 않습니다*.

- `PQnparams`
  
  준비된 문장의 파라미터 숫자를 반환합니다.`int PQnparams(const PGresult *res);`이 함수는 `PQdescribePrepared`의 결과를 확인할 경우에만 유용합니다.다른 종류의 쿼리에서는 0을 반환합니다.

- `PQparamtype`
  
  지정된 문장의 데이터형을 반환합니다.파라미터 번호는 0으로 시작합니다.`Oid PQparamtype(const PGresult *res, int param_number);`이 함수는 `PQdescribePrepared`의 결과를 확인할 경우에만 유용합니다.다른 종류의 쿼리에서는 0을 반환합니다.

- `PQprint`
  
  모든 행과 열명(생략 가능)을 지정한 출력 스트림에 표시합니다.`void PQprint(FILE *fout,      /* output stream */             const PGresult *res,             const PQprintOpt *po); typedef struct {    pqbool  header;      /* print output field headings and row count */    pqbool  align;       /* fill align the fields */    pqbool  standard;    /* old brain dead format */    pqbool  html3;       /* output HTML tables */    pqbool  expanded;    /* expand tables */    pqbool  pager;       /* use pager for output if needed */    char    *fieldSep;   /* field separator */    char    *tableOpt;   /* attributes for HTML table element */    char    *caption;    /* HTML table caption */    char    **fieldName; /* null-terminated array of replacement field names */ } PQprintOpt; --> void PQprint(FILE *fout,      /* 출력 스트림 */             const PGresult *res,             const PQprintOpt *po); typedef struct {    pqbool  header;      /* 필드 헤더 정보와 행 수의 표시 출력 */    pqbool  align;       /* 필드를 정열해서 채움 */    pqbool  standard;    /* 낡아 없어질 것 같은 서식 */    pqbool  html3;       /* HTML 표출력 */    pqbool  expanded;    /* 확장 테이블 */    pqbool  pager;       /* 필요할 경우 출력을 위한 pager 사용 */    char    *fieldSep;   /* 필드 단락 문자 */    char    *tableOpt;   /* HTML 표 요소의 속성 */    char    *caption;    /* HTML 표의 표제 */    char    **fieldName; /* 필드명을 교체하는 null로 끝나는 배열 */ } PQprintOpt;`이 함수는 이전 쿼리 결과를 표시하기 위해서psql로 사용되었지만, 지금은 사용되고 있지 않습니다.이것은 모든 데이터가 텍스트 서식이라는 전0 동작하는 것에 주의하십시요.

## 29.3.3. 다른 커맨드용의 결과 정보의 획득

이러한 함수는 `SELECT`결과 이외의`PGresult`오브젝트로부터 정보를 꺼내기 위해서 사용됩니다.

- `PQcmdStatus`
  
  `PGresult`를 생성하는 SQL 커맨드의 커맨드 상태 태그를 반환합니다.`char *PQcmdStatus(PGresult *res);`이것은 보통 단순한 커맨드명이지만, 처리된 행의 수와 같은 추가 데이터를 포함할 수 있습니다.호출하는 측은 이 반환 값을 직접 해제해서는 안됩니다.관련하는 `PGresult`핸들이`PQclear`에게 건네졌을 때 이것은 해제됩니다.

- `PQcmdTuples`
  
  SQL 커맨드에 의해 영향을 받은 행수를 반환합니다.`char *PQcmdTuples(PGresult *res);``PGresult`를 생성한SQL커맨드에 의해 영향을 받은 행 수를 포함하는 문자열을 반환합니다.이 함수는 `INSERT`, `UPDATE`, `DELETE`, `MOVE`, `FETCH`, `COPY`쿼리 실행, 혹은 `INSERT`, `UPDATE`, `DELETE`를 포함할 준비된 쿼리의`EXECUTE`쿼리 다음에만 사용할 수 있습니다.`PGresult`를 생성한 커맨드가 다른 커맨드였을 경우, `PQcmdTuples`는 공문자열을 반환합니다.호출하는 측은 이 반환 값을 직접 해제해서는 안됩니다.관련하는 `PGresult`핸들이`PQclear`에게 건네졌을 때 이것은 해제됩니다.

- `PQoidValue`
  
  SQL커맨드가, OID를 가지는 테이블 내에 1행만을 삽입하는 `INSERT`였던 경우, 혹은 적절한`INSERT`를 가질 준비된 쿼리의`EXECUTE`였던 경우에, 삽입된 행의 OID를 반환합니다.아니면`InvalidOid`를 반환합니다.또한, `INSERT`쿼리 영향을 받은 테이블이 OID를 가지지 않은 경우, 이 함수는 `InvalidOid`를 반환합니다.`Oid PQoidValue(const PGresult *res);`

- `PQoidStatus`
  
  SQL커맨드가 1행만을 삽입하는 `INSERT`였을 경우, 혹은 적절한`INSERT`를 가질 준비된 문장의`EXECUTE`였을 경우, 삽입된 행의 OID를 문자열으로 반환합니다. (`INSERT`가 복수행을 삽입했을 경우나 대상 테이블이 OID를 가지지 않는 경우는 `0`이라고 하는 문자열을 반환합니다. )커맨드가`INSERT`가 아니면, 공문자열을 반환합니다.`char *PQoidStatus(const PGresult *res);`이 함수는 `PQoidValue`를 위해서 쓸모없게 되었습니다.이것은 thread 세이프가 아닙니다.

## 29.3.4. SQL 커맨드에 포함하기 위한 문자열의 이스케이프 처리

`PQescapeStringConn`는 SQL 커맨드 내에서 사용하기 위해 문자열을 이스케이프합니다.이는 SQL 커맨드 내의 리터럴 정수로서 데이터 값을 삽입할 때 유용합니다.특정한 문자(인용부호나 backslash)는 SQL 파서에 의해 특수한 해석이 이루어지지 않게 이스케이프 되어야 합니다.`PQescapeStringConn`는 이 조작을 실행합니다.

> **Tip:** 신용할 수 없는 입력원으로부터 받은 문자열을 취급하는 경우에 적절한 이스케이프 처리를 실행하는 것은 매우 중요합니다.아니면, 보안상의 위험성이 발생합니다."SQL 삽입" 공격에 상처입기 쉬워, 바람직하지 않은 SQL 커맨드가 데이터베이스에 들어갑니다.

`PQexecParams`나 그 파생 루틴을 사용해 분리된 파라미터로서 데이터 값을 건네주는 경우, 이스케이프 처리는 필요없으며 부정확하지 않도록 주의하십시요.

```
size_t PQescapeStringConn (PGconn *conn,
                           char *to, const char *from, size_t length,
                           int *error);
```

`PQescapeStringConn` 는 `from`문자열, 특수 문자를 이스케이프해 문제가 일어나지 않도록, 0으로 끝나는 바이트를 추가해`to`버퍼에 기입합니다. PostgreSQL의 문자 리터럴로서 둘러싸여야 할 단일 인용부호는 결과 문자열에 포함되지 않습니다. 이는 결과를 SQL 커맨드에 삽입할 경우에 부여해야 합니다.`from`파라미터는 이스케이프 대상의 문자열의 선두를 가리키는 포인터입니다.`length`파라미터는 이 문자열의 바이트 수를 나타냅니다.0바이트로 끝나도록 요구되지 않으며, `lenth`에서는 이것을 세지 않습니다. (만약`length`바이트 처리되기 전에 0으로 끝나는 바이트가 존재하면, `PQescapeStringConn`는 0에서 종료합니다.이 동작은`strncpy`와 같습니다. )`to` 은 `length` 2배보다 최소한 1바이트 많은 문자를 보관할 수 있는 버퍼의 포인터로 해야 합니다.그렇지 않으면, 동작은 정의되지 않습니다.`to` 와`from`문자 영역이 겹치는 경우, 동작은 정의되지 않습니다.

`error` 파라미터가 NULL이 아니면, `*error` 는 성공할 경우 0으로, 에러일 경우 0이 아닌 값으로 설정됩니다.현재 유일하게 가능한 에러 조건은 소스 문자열에 무효인 멀티 바이트 부호가 포함되어 있는 경우입니다.출력 문자열은 에러여도 생성되지만, 서버가 오작동으로 거절할 것입니다. 에러에서, 적절한 메세지는 `error`가 NULL인지와 관계없이`conn`오브젝트 내에 저장됩니다.

`PQescapeStringConn`는 `to`에 입력된 바이트 수를 반환합니다.다만, 문자 수에는 0으로 끝나는 바이트는 포함되지 않습니다.

```
size_t PQescapeString (char *to, const char *from, size_t length);
```

`PQescapeString`은`PQescapeStringConn`의 추천되지 않는 오래된 버전입니다.둘의 차이는 `conn`나 `error`파라미터를 취하지 않는 것입니다.이로 인해 `PQescapeString`가(문자 인코딩과 같은) 접속 프로퍼티에 의한 행동을 조정할 수 없는 것과 그에 따른 *잘못한 결과를 반환할 가능성*이 있습니다.또한, 에러 상태를 통보하는 기능이 없습니다.

`PQescapeString`는 한 번에 하나의 접속만 하는 single-threaded 클라이언트 프로그램에서 안전하게 이용될 수 있습니다. (이 경우, 알아야 하는 "뒤에 숨겨진 정보"가 무엇인지 발견할 수 있습니다.)다른 경우에는 보안 요인이며`PQescapeStringConn`를 이용하는 것으로 피해야 합니다.

## 29.3.5. SQL 커맨드에 포함하기 위한 바이너리 문자열의 이스케이프 처리

- `PQescapeByteaConn`
  
  `bytea`형으로서 SQL 커맨드 내에서 사용하는 바이너리 데이터를 이스케이프합니다.`PQescapeStringConn`와 같이, 이는 SQL 커맨드 문자열에 데이터를 직접 포함하는 경우에만 사용됩니다.`unsigned char *PQescapeByteaConn(PGconn *conn,                                 const unsigned char *from,                                 size_t from_length,                                 size_t *to_length);`SQL내 `bytea`리터럴의 일부로서 사용하는 경우, 특정한 바이트 값은 이스케이프*되지 않으면 안됩니다*. (모든 바이트 값을 이스케이프해도*상관하지 않습니다*. )일반적으로 바이트를 이스케이프하려면 , 보통 2개의 backslash의 뒤에 8 진수치와 동일한 3자리수의 8 진수로 변환합니다.단일 인용부호(`'`)와 backslash(`\`)는 특별히 다른 이스케이프 처리를 합니다.자세한 사항은 [Section 8.4](http://www.postgresplus.co.kr/man/datatype-binary.html)을 참조해 주십시요.`PQescapeByteaConn`는 최소로 필요한 바이트만을 이스케이프하여, 조작을 실시합니다.`from`파라미터는 이스케이프 대상 문자열의 선두 바이트를 지시하는 포인터입니다.`from_length`파라미터는 이 바이너리 문자열 내의 바이트 수를 지정합니다. (0으로 끝나는 바이트는 불필요하며, 셀 수 없습니다. )`to_length`파라미터는 결과가 되는 이스케이프 된 문자열의 길이를 보관하는 변수의 포인터입니다.이 결과 문자열 길이는 결과로 0으로 끝나는 바이트를 포함합니다.`PQescapeByteaConn`는 `from`파라미터가 가리키는 바이너리 문자열을 이스케이프한 것을`malloc()`로 확보한 메모리 내에 반환합니다.그 결과가 불필요하게 되면, 이 메모리를`PQfreemem`를 사용해 해제해야 합니다.반환되는 문자열은 PostgreSQL리터럴 문자열 파서와`bytea`입력 함수에 의해 적절히 처리할 수 있도록, 모든 특수한 문자가 치환되고 있습니다.0으로 끝나는 바이트도 추가됩니다.PostgreSQL의 리터럴 문자열을 둘러 싼 인용부호는 결과 문자열에 포함되지 않습니다.에러시, NULL 포인터를 돌려주어 적절한 에러 메세지를`conn`오브젝트에 저장합니다. 현재, 유일한 에러는 결과 문자열의 메모리 부족입니다.

- `PQescapeBytea`
  
  `PQescapeBytea`는 `PQescapeByteaConn`의 추천되지 않는 오래된 것입니다.`unsigned char *PQescapeBytea(const unsigned char *from,                             size_t from_length,                             size_t *to_length);``PQescapeBytea`의`PQescapeByteaConn`와의 유일한 차이는 `PGconn`파라미터입니다.이는 `PQescapeBytea`가 접속 프로퍼티(규격에 적합하는 문자열이 아니어도)에 의한 행동을 조정할 수 없어, 그로 인해 *잘못한 결과를 반환할* 수 있습니다.또한, 실패의 에러 메세지를 통보하는 기능은 없습니다.`PQescapeBytea`는 한 번에 하나의 접속만 하는 single-threaded의 클라이언트 프로그램에서는 안전하게 이용할 수 있습니다. (이 경우, 알아야 하는 "뒤에 숨겨진 정보"가 무엇인지 발견할 수 있습니다.)다른 경우는 보안 요인이며 `PQescapeByteaConn` 이용을 피해야 합니다.

- `PQunescapeBytea`
  
  바이너리 데이터의 문자열 표현을 바이너리 데이터로 변환합니다.즉, `PQescapeBytea`의 반대입니다.이는 `bytea`데이터를 텍스트 서식에서 받았을 경우에 필요하게 됩니다.그러나 바이너리 서식에서 받았을 경우는 불필요합니다.`unsigned char *PQunescapeBytea(const unsigned char *from, size_t *to_length);``from`파라미터는 예를 들면, `bytea`열에`PQgetvalue`를 행했을 경우에 돌려주어질 가능성이 있는 문자열을 지시하는 포인터입니다.`PQunescapeBytea`는 이 문자열 표현을 바이너리 표현으로 변환합니다.`malloc()`로 확보한 버퍼의 포인터를 반환합니다.에러시는 null 포인터입니다.또한, 이 버퍼의 크기를`to_length`에 저장합니다.불필요하게 되면, 이 결과를`PQfreemem`를 사용해 해제해야 합니다.이 변환은 `PQescapeBytea`의 반대가 아닙니다. 문자열은`PQgetvalue` 로부터 받는 경우"이스케이프 된"일을 예상하지 않기 때문입니다.특히 이는 문자열의 인용부호를 의식할 필요가 없고, 그 때문에`PGconn`파라미터를 가질 필요가 없는 것을 의미합니다.

- `PQfreemem`
  
  libpq로 확보한 메모리를 해제합니다.`void PQfreemem(void *ptr);`libpq, 구체적으로 `PQescapeByteaConn`, `PQescapeBytea`, `PQunescapeBytea`, `PQnotifies`로 확보된 메모리를 해제합니다.Microsoft Windows에서는 `free()`가 아닌, 이 함수가 사용됩니다는 점이 특히 중요합니다.DLL로 할당한 메모리를 어플리케이션으로 해제하는 것은 multithreaded/single-threaded, release/debug, static/dynamic 플래그가 그 DLL와 어플리케이션으로 동일한 경우에만 가능하기 때문입니다.Microsoft Windows 이외의 플랫폼에서는 이 함수는 `free()`표준 라이브러리 함수와 동일합니다.**[출처]** [29.3. 커맨드 수행 함수 | postgresql](https://blog.naver.com/shsoul12/10123250467)|**작성자** [밍밍](https://blog.naver.com/shsoul12)