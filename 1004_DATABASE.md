# DATABASE

### TABLE

- 레코드
  
  - 튜플, 행

- pk
  
  - 각 레코드의 고유한 값

### SQL Command

 DDL 데이터 정의 언어- 관계형 데이터베이스 구조를 정의

DML 데이터 조작언어 - 데이터를 조작하기 위한 명령어  

### sql syntax

모든 sql문은 SELECT,INSERT,UPDATE 등과 같은키워드로 시작

하나의 STATEMENT는 세미콜론으로 끝남

### DDL

- CREATE

- ALTER

- DROP

- CREATE TABLE statement
  
  - 데이터 베이스에 새 테이블 만듦
  
  - ```python
    CREATE TABLE table_name (
        column_1 data_type constraints
        ...    
    );
    ```
  
  - NULL  SQL에서 none
  
  - ID컬럼은 정의하지 않으면 자동으로 rowid라는 컬럼 생성
  
  - DATA TYPES
    
    - null 정보가 없가니 알수 없음
    
    - integer 정수0~8바이트
    
    - REAL 실수 8,10바이트
    
    - TEXT
    
    - BLOB(binary large obj) 입력된 그대로 저장된 데이터 덩어리
    
    - boolean 타입 없음 0,1로 저장
    
    - 날짜가 없아 text혹은 real,integer로 저장
    
    - 동적인 datatype이지만 호환성 문제 때문에 데이터 타입 지정해주자!

- constraints 종류
  
  - not null 
  
  - unique : 컬럼의 모든값이 서로 구별되는 고유한 값이 되도록함
    
    ex) 이미 가입한 이메일입니다
  
  - primary key : 행의 고유성을 식별하는데 사용하는 컬럼(not null 포함됨)
  
  - autoincrement 사용하거나 이전에 삭제된행의 값을 재사용 막음

- ALTER TABLE
  
  - 기존테이블 구조 수정
  
  - rename table
    
    - `ALTER TABLE contacts RENAME TO new_contacts`
  
  - rename column
    
    - `ALTER COLUMN name TO last_name`
  
  - add ne column to table
    
    - `ALTER COLUMN address TEXT NOT NULL`
    
    - 기존데이터가 있으면 에러 발생
    
    - `ALTER COLUMN address TEXT NOT NULL DEFAULT 'no address'`
    
    - 기본값을 지정해주면 됨
  
  - delete column
    
    - `ALTER TABLE new_contacts DROP COLUMN address`
      
      삭제 불가능인경우
      
      1. 커럼이 다른부분에서 참조중인경우
      
      2. primary key인경우
      
      3. unique제약조건이 있는경우

# DML

데이터 조작 CRUD

insert,select,update,delete

csv에서 데이터 받아오기

```python
.open mydb.sqlite3
.mod csv
.import users.csv users
```

## simple query

- 조회 하기
  
  `    SELECT first_name,age FROM users;`

- 전체 데이터

- ```python
      SELECT * FROM users;
  
      SELECT rowid, first_name FROM users;
  ```

### sorting rows

- select 문에 추가 하여 결과 정렬 

- from 절 뒤에 위치

- ASC: 오름차순(기본), DESC:내림차순

- 예시
  
  ```python
  SELECT first_name,age FROM users
  ORDER BY age ASC;
  ```

```python
SELECT first_name, age, balance FROM users
ORDER BY age ASC, balance DESC;

select DISTINCT country FROM users;
```

### filtering data

distrinct: 중복없이

```python
select DISTINCT first_name, country FROM users
ORDER BY country DESC;
```

where: from 뒤에

`where column-1 = 10`

`where column_2 LIKE 'Ko%'` Ko로 시작

`where column_3 in (1,2)`

`where column_4 between 10 and 20`

논리연산자는 1,0 return

`select first_name, age, balance FROM users WHERE age >= 30;`

like operator

% 0개 이상의 문자가 올수 있음

`'%호%'` 호로 끝나는

_호 호로 끝나는 둘이상

### grouping data

`SELECT country, COUNT(*) FROM users GROUP BY country;`

집계함수

avg(), count(), max(), min(), sum()

### changing data

`INSERT INTO TABLE_NAME (colum1,column2, ...)`

`VALUES (value1, value2, ...)`

`DELETE FROM TABLE_NAME`

조건이 없으면 모든 데이터 삭제

`update table_name`

`set column1 = new1`

   `column2 = new2`
