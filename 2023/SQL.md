# SQL

## select, where 연습하기

`show tables` 테이블 목록보기

`select * from orders where name like '이__' and name like '강__'` and를 이용하여 여러가지 조건을 추가 할 수 있다.

### where 자주쓰이는 문법

- 같지 않음:
  
  `select * from orders where name != '이**'`

- 범위
  
   `select * from orders where date between '2023-05-01' and '2023-06-16'`

- 포함 
  
  `select * from orders where cost in (3000, 50000)`
  
  연속적이지 않으면 쓰자? 순서가 정해져 있으면
  
  in이 빠르다
  
  [[mssql] BETWEEN..AND / IN / LIKE 사용법, 예제, 차이](https://althathatha.tistory.com/37 "https://althathatha.tistory.com/37")

- 문자열 규칙
  
  `select * from orders where email like '%com`'
  
  `_` 글자 갯수

- 제한
  
  `select * from orders limit(5)`

- 중복 제외
  
  `select distinct(*) form orders`

- 카운트
  
  `select count(*) from orders`

## Group by

동일한 범주의 데이터를 묶어주는 함수

동일한 범주 갯수 구하기

`select name, count(*) from users group by name order by count(*) desc`

`count(*)`자리에 `min(column), max(column), round(avg(column), 반올림 자리), sum(column)`등이 올수 있따

정렬은 기본적으로 오름차순

한글, 알파벳, 시간 모두 정렬가능

### as

jpa처럼 별칭을 정할수 있따

`select * from orders o ...`

`select * from orders as o ...`

### join

테이블 끼리 연결

join없이 테이블을 조회하먄 모든행이 조인 된다

`select * from users u left join point_users pu on u.user_id = pu.user_id`

null값이 있다면 매칭이 안된것

`users`테이블에 `point_users`테이블을 합친다

inner join 일치하는 값을 가진 레코드만 조회

`select e.name, e.job, e.deptno, d.name from emp e inner join dept d on e.deptno = d.deptno where e.empno = 7788;`

`where`절만으로도 가능하다

`select e.name, e.job, e.deptno, d.name from emp e, dept d where e.empno = 7788 and e.deptno = d.deptno`

self join

테이블 내에서 정보를 가공하고 싶을때

`SELECT  
 cust.customer_id,  
 cust.firstname,  
 cust.lastname,  
 cust.birthdate,  
 cust.spouse_id,  
 spouse.firstname AS spouse_firstname,  
 spouse.lastname AS spouse_lastname  
FROM **customer** AS cust  
**INNER JOIN customer** AS spouse  
   **ON** cust.spouse_id **=** spouse_id;`

비동등 조인

[SQL, 7번째 개발일지(4-4~7)](https://yoongeons.com/entry/20220706-SQL-7%EB%B2%88%EC%A7%B8-%EA%B0%9C%EB%B0%9C%EC%9D%BC%EC%A7%80)

## Subquery

포함할수 있는 sql문

- select from where having, order by

- insert values

- update  set

반드시 () 감싸주자

다중 행 비교 연산자와 함께 사용 가눙

`select d.dname`

`from emp.e, dept d` 

`where e.deptno = d.deptno` 

`and e.empno = 7788`

inner join을 한 쿼리 -> 카테시안 곱으로 속도가 느려질 수 잇다

`selec dname`

`from dept`

`where deptno = ( select deptno`

`from emp`

`where empno = 7788``

중첩 서브 쿼리

IN, ANY, ALL 연산자와 사용

`SELECT ename, sal, job`

`from emp`

`where sal > ANY (select sal `

` from emp`

` where job = 'salesman)`

`and job != 'salesman'`





### 그외 함수

IFNULL(), DATE_FORMAT(), IF(,,), DATEDIFF, YEAR(), ..
