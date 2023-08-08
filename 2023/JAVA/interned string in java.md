# Interned String in Java

**자바(Java)의 문자열(String)은 불변(immutable)하다.**

```java
public void func() {
    String haribo1st = new String("HARIBO");
    String copiedHaribo1st = haribo1st.toUpperCase();

    System.out.println(haribo1st == copiedHaribo1st);   //true
}
```

그 이유는 `toUpperCase()` 함수의 로직 때문이다. 해당 함수는 lower case의 문자가 발견되지 않으면 기존의 객체를 반환

아니다. 생성자를 통해 선언하게 되면 같은 문자열을 가진 새로운 객체가 생성된다. 즉, 힙(heap)에 새로운 메모리를 할당하는 것이다.

```java
public void func() {
    String haribo3rd = "HARIBO";
    String haribo4th = String.valueOf("HARIBO");

    System.out.println(haribo3rd == haribo4th);
    System.out.println(haribo3rd.equals(haribo4th));
}
```

이번에는 **리터럴(literal)** 로 선언한 객체와 `String.valueOf()`로 가져온 객체를 한번 살펴보자.
`valueOf()`함수를 들어가보면 알겠지만, 주어진 매개 변수가 null인지 확인한 후 null이 아니면 매개 변수의 `toString()`을 호출한다.
여기서 `String.toString()`은 `this`를 반환한다. 즉, 두 구문 모두 `"HARIBO"`처럼 리터럴 선언이다. 
그렇다면 리터럴로 선언한 객체는 왜 같은 객체일까?

바로 JVM에서 constant pool을 통해 문자열을 관리하고 있기 때문이다.
리터럴로 선언한 문자열이 constant pool에 있으면 해당 객체를 바로 가져온다.
만약 pool에 없다면 새로 객체를 생성한 후, pool에 등록하고 가져온다.

constant pool에서 헤쉬테이블로 관리 -> 빨라요

java 7부터 GC의 대상

### 잠깐만! 리터럴이란?

**프로그램에서 직접 표현한 값**

소스코드의 고정된 값을 대표

정수, 실수, 문자, 논리, 문자열 리터럴 존재

결론: 그냥 그 아는 모양

### References

- https://www.latera.kr/blog/2019-02-09-java-string-intern/

- https://blog.naver.com/adamdoha/222817943149

- [[JAVA] 자바_리터럴(literal)이란?](https://mine-it-record.tistory.com/100)
