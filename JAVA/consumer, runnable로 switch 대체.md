자바에서 switch 문을 Map<Integer, Runnable>로 대체하는 방법은 다음과 같이 숫자 키와 실행할 작업을 매핑하여 코드의 가독성과 유연성을 높이는 방식입니다. 특히 switch-case가 복잡해질 때 유용합니다.

case가 연속적: 분기 테이블 -> O(1)
case 분연속: 이진 탐색 혹은 순차 분기 ->O(nlogn), O(n)

케이스가 많아지면 가독성 나빠짐

```
int option = 2;

switch (option) {
    case 1:
        System.out.println("Option 1");
        break;
    case 2:
        System.out.println("Option 2");
        break;
    case 3:
        System.out.println("Option 3");
        break;
    default:
        System.out.println("Invalid option");
}
```

Map<Integer, Runnable>로 변환
```
import java.util.HashMap;
import java.util.Map;

public class Main {
    public static void main(String[] args) {
        int option = 2;

        Map<Integer, Runnable> actionMap = new HashMap<>();
        actionMap.put(1, () -> System.out.println("Option 1"));
        actionMap.put(2, () -> System.out.println("Option 2"));
        actionMap.put(3, () -> System.out.println("Option 3"));

        // 실행
        actionMap.getOrDefault(option, () -> System.out.println("Invalid option")).run();
    }
}
```


Consumer<T>
Java의 함수형 인터페이스 중 하나로,
입력값 T를 받아서 아무 것도 반환하지 않고 어떤 동작(consume)만 수행하는 함수 타입

```
@FunctionalInterface
public interface Consumer<T> {
    void accept(T t);
}

```
T 타입의 값을 받아서 처리만 하고, 결과를 반환X

```
import java.util.*;

public class Main {
    public static void main(String[] args) {
        Map<Integer, Consumer<String>> commandMap = new HashMap<>();
        commandMap.put(1, name -> System.out.println("안녕, " + name));
        commandMap.put(2, name -> System.out.println(name + "님 환영합니다"));
        commandMap.put(3, name -> System.out.println(name + "님, 로그아웃"));

        int command = 2;
        String userName = "홍길동";

        commandMap
            .getOrDefault(command, n -> System.out.println("알 수 없는 명령입니다"))
            .accept(userName);
    }
}

```


반환값이 필요하다면?
다른 인터페이스 사용

| 상황             | 인터페이스                 | 시그니처                | 설명           |
| -------------- | --------------------- | ------------------- | ------------ |
| 매개변수 O, 반환값 O  | `Function<T, R>`      | `R apply(T t)`      | 입력받고 결과 반환   |
| 매개변수 X, 반환값 O  | `Supplier<R>`         | `R get()`           | 그냥 결과를 반환만 함 |
| 매개변수 2개, 반환값 O | `BiFunction<T, U, R>` | `R apply(T t, U u)` | 입력 2개 받고 반환  |


```
Map<Integer, Supplier<String>> map = new HashMap<>();
map.put(1, () -> "첫 번째");
map.put(2, () -> "두 번째");

String result = map.getOrDefault(1, () -> "기본값").get();
System.out.println(result);  // 출력: 첫 번째

```
