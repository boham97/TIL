##### PostgreSQL 접속하기

psql을 사용하여 PostgreSQL에 접속하는 방법은 다음과 같다.

1. 터미널 또는 명령 프롬프트를 실행한다.
2. 다음 명령을 실행하여 PostgreSQL 데이터베이스에 로그인한다.

```css
psql -U [username] -d [database_name] -h [hostname] -p [port_number]
```

- `username`: PostgreSQL 데이터베이스에 로그인할 사용자 이름이다. 기본적으로 'postgres'로 설정되어 있다.
- `database_name`: 접속할 데이터베이스의 이름이다. 기본값은 'postgres'이다.
- `hostname`: PostgreSQL 서버의 호스트 이름이다. 기본값은 'localhost'이다.
  - `-h` 옵션인 경우 tcp/ip 통신을한다 local인경우 생략시 unix 소켓통신
- `port_number`: PostgreSQL 서버의 포트 번호이다. 기본값은 '5432'이다.







`PGPASSWARD PGUSER PGDATABASE PGPORT` 파라미터로 접속가능



##### psql 명령어

psql은 다양한 명령어를 지원하여 PostgreSQL 데이터베이스와 상호작용할 수 있다. 몇 가지 유용한 psql 명령어에 대해 알아보자.

- `\l`: 현재 서버에 있는 모든 데이터베이스 목록을 조회한다.
- `\c database_name`: 지정한 데이터베이스로 접속한다.
- `\d`: 현재 데이터베이스에 있는 모든 테이블, 뷰, 시퀀스, 인덱스 등의 목록을 조회한다.
- `\dt`: 현재 데이터베이스에 있는 모든 테이블의 목록을 조회한다.
- `\dv`: 현재 데이터베이스에 있는 모든 뷰의 목록을 조회한다.
- `\di`: 현재 데이터베이스에 있는 모든 인덱스의 목록을 조회한다.
- `\q`: psql 클라이언트를 종료하고 터미널로 빠져나간다.





`sPQSQL.5432 관련 에러 ` 

unix소켓통신이 되지 않은것 

`user/pgsql-16/bin/psotgres`를 집적 실행 시켜 자세한 로그 파악가능

- 파일 권한 문제

- pg_hba.conf 에서 접속 여부, 인증 방법 문제

- postgresql.conf 








