# STREAM

자바에서도 8버전 이상부터 람다를 사용한 함수형 프로그래밍이 가능

- 요청할 때만 요소를 계산한다. 내부 반복을 사용하므로, 추출 요소만 선언해주면 알아서 반복 처리를 진행한다.
- 스트림에 요소를 따로 추가 혹은 제거하는 작업은 불가능하다.

#### Stream 중간 연산

- filter(Predicate) : Predicate를 인자로 받아 true인 요소를 포함한 스트림 반환
- distinct() : 중복 필터링
- limit(n) : 주어진 사이즈 이하 크기를 갖는 스트림 반환
- skip(n) : 처음 요소 n개 제외한 스트림 반환
- map(Function) : 매핑 함수의 result로 구성된 스트림 반환
- flatMap() : 스트림의 콘텐츠로 매핑함. map과 달리 평면화된 스트림 반환

> 중간 연산은 모두 스트림을 반환한다.

#### Stream 최종 연산

- (boolean) allMatch(Predicate) : 모든 스트림 요소가 Predicate와 일치하는지 검사
- (boolean) anyMatch(Predicate) : 하나라도 일치하는 요소가 있는지 검사
- (boolean) noneMatch(Predicate) : 매치되는 요소가 없는지 검사
- (Optional) findAny() : 현재 스트림에서 임의의 요소 반환
- (Optional) findFirst() : 스트림의 첫번째 요소
- reduce() : 모든 스트림 요소를 처리해 값을 도출. 두 개의 인자를 가짐
- collect() : 스트림을 reduce하여 list, map, 정수 형식 컬렉션을 만듬
- (void) forEach() : 스트림 각 요소를 소비하며 람다 적용
- (Long) count : 스트림 요소 개수 반환

```
        List<NetworkResponse> networkResponseList = networkDBList.stream()
                .map(networkDB -> new NetworkResponse(networkDB.getDevice_name(), networkDB.getSetting_ip_addr(), networkDB.getSetting_ip_type()))
                .collect(Collectors.toList());
    }
```

dto 맵핑에쓰자

일반적으로 `Stream.forEach()`를 사용하면 전통적인 `for-loop`를 사용할 때보다 오버헤드가 훨씬 심각하게 발생하기 때문에, 

모든 `for-loop`를 `Stream.forEach()`로 대체하면, 애플리케이션 전체에 걸쳐 누적되는 CPU 싸이클 낭비는 무시하지 못할 수준이 될 수 있다.

원시 데이터(primitive data type)를 처리할 때는 **절대적으로** 전통적인 `for-loop`를 써야한다

참조

https://github.com/gyoogle/tech-interview-for-developer/blob/master/Language/%5Bjava%5D%20Stream.md
