

# java version



### 1.5

generics

 Enumeration

anotation

- 사용자 정의 어노테이션 생성 가능

- 언제까지 유지할지도 지정가능





### 1.6

G1 GC 사용 가능: 순환참조 찾아 메모리 회수



### 1.7

`<>` diamond operator

- 변수 선언시 동일타입 명시안할수있다



### 1.8

**lambda**

- 익명함수

- Consumer
  
  - 값받고 반환값 없는 함수형 인터페이스
    
    ```java
    Consumer<String> print = s -> System.out.println(s);
    print.accept("Hello, Consumer!");
    ```

- Spplier
  
  - 입력값X 리턴O
    
    ```java
    Supplier<Double> randomSupplier = () -> Math.random();
    System.out.println(randomSupplier.get());
    ```

- predicate
  
  - 입력값 O 리턴 bool
  
  ```java
  Predicate<Integer> isEven = n -> n % 2 == 0;
  System.out.println(isEven.test(4)); // 출력: true
  System.out.println(isEven.test(5)); // 출력: false
  ```

- Function 입력값 받아 다른 타입 리턴
  
  ```java
  Function<String, Integer> lengthFunction = s -> s.length();
  System.out.println(lengthFunction.apply("Java")); 
  ```

- Operator 입력받아 같은 타입 리턴'
  
  ```java
  UnaryOperator<Integer> square = n -> n * n;
  ```

- 메소드 참조 기존의 함수를 참조

```java
class MyClass {
    public void printMessage(String message) {
        System.out.println(message);
    }
}

public class Example {
    public static void main(String[] args) {
        MyClass myClass = new MyClass();
        
        // 객체의 메서드 참조
        BiConsumer<MyClass, String> printer = MyClass::printMessage;
        printer.accept(myClass, "Hello, Instance Method!");  // 출력: Hello, Instance Method!
    }
}

```

**optional** null 체크 코드 간결

**stream**

- 순차적, 병렬 처리 가능

#### 중간 연산 (Intermediate Operations)

중간 연산은 **데이터를 변환하거나 필터링**하는 연산으로, 다른 연산을 반환하는 연산입니다. 중간 연산은 **지연**되며, 연속적으로 연결할 수 있습니다.

- `filter()`: 조건에 맞는 요소만 필터링합니다.
- `map()`: 각 요소를 다른 형태로 변환합니다.
- `distinct()`: 중복을 제거합니다.
- `sorted()`: 정렬합니다.
- `flatMap()`: 여러 요소로 변환된 결과를 하나로 합칩니다.

#### 최종 연산 (Terminal Operations)

최종 연산은 스트림의 처리 결과를 생성하거나, 스트림을 **소모**하는 연산입니다. 최종 연산이 호출되면 스트림은 더 이상 사용할 수 없습니다.

- `forEach()`: 각 요소에 대해 작업을 수행합니다.
- `collect()`: 결과를 컬렉션으로 변환합니다.
- `reduce()`: 요소를 축소하여 하나의 값으로 만듭니다.
- `count()`: 요소의 개수를 셉니다.
- `anyMatch()`, `allMatch()`, `noneMatch()`: 조건에 맞는 요소가 있는지 확인합니다.



### 1.9

**stream api**

- takewhile() 조건이 아닌경우 멈춤

- dropwhile() 조건이 아닐떄까지 버림



### jdk10

**var**  초기화 필수



### jdk11

- lamdba 파라미터로 var 사용

- string, file api 개선





https://velog.io/@dongvelop/JDK-21%EC%9D%B4-%EC%B6%9C%EC%8B%9C%EB%90%98%EC%97%88%EB%8B%A4.-%EC%B5%9C%EC%8B%A0-%EB%AC%B8%EB%B2%95%EC%9D%80-%EC%82%B4%ED%8E%B4%EB%B4%90%EC%95%BC%EC%A7%80