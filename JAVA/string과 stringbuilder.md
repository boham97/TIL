### String, StringBuffer, StringBuilder

----

| 분류  | String    | StringBuffer                  | StringBuilder     |
| --- | --------- | ----------------------------- | ----------------- |
| 변경  | Immutable | Mutable                       | Mutable           |
| 동기화 |           | Synchronized 가능 (Thread-safe) | Synchronized 불가능. |



시간 복잡도

`string`의 `+`는 O(N+K)

N번 하므로 N(N+1)/2  = N^2

sb는 O(1) 

---

#### 1. String 특징

* new 연산을 통해 생성된 인스턴스의 메모리 공간은 변하지 않음 (Immutable)
* Garbage Collector로 제거되어야 함.
* 문자열 연산시 새로 객체를 만드는 Overhead 발생
* 객체가 불변하므로, Multithread에서 동기화를 신경 쓸 필요가 없음. (조회 연산에 매우 큰 장점)

*String 클래스 : 문자열 연산이 적고, 조회가 많은 멀티쓰레드 환경에서 좋음*

#### 2. StringBuffer, StringBuilder 특징

- 공통점
  
  - new 연산으로 클래스를 한 번만 만듬 (Mutable)
  - 문자열 연산시 새로 객체를 만들지 않고, 크기를 변경시킴
  - StringBuffer와  StringBuilder 클래스의 메서드가 동일함.

- 차이점
  
  - StringBuffer는 Thread-Safe함 / StringBuilder는 Thread-safe하지 않음 (불가능)
  
  <br>

*StringBuffer 클래스 : 문자열 연산이 많은 Multi-Thread 환경*

*StringBuilder 클래스 : 문자열 연산이 많은 Single-Thread 또는 Thread 신경 안쓰는 환경*

### 주요 메소드

**sb.append(값)**

- StringBuffer, StringBuilder 뒤에 값을 붙인다

**sb.insert(인덱스, 값)**

- 특정 인덱스부터 값을 삽입한다

**sb.delete(인덱스, 인덱스)**

- 특정 인덱스부터 인덱스까지 값을 삭제한다

**sb.indexOf(값)**

- 값이 어느 인덱스에 들어있는지 확인한다

**sb.substring(인덱스, 인덱스)**

- 인덱스부터 인덱스까지 값을 잘라온다

**sb.length()**

- 길이 확인

**sb.replace(인덱스, 인덱스, 값)**

- 인덱스부터 인덱스까지 값으로 변경

**sb.reverse()**

출처

https://github.com/gyoogle/tech-interview-for-developer/blob/master/Language/%5Bjava%5D%20String%20StringBuilder%20StringBuffer%20%EC%B0%A8%EC%9D%B4.md

[자바 StringBuffer, StringBuilder 개념부터 사용법까지](https://wakestand.tistory.com/245)
