### pthread종료시키는 방법

1. 스레드 내부 리턴

2. pthread_exit 호출

3. pthread_cancel호출
- 같은 프로세스내 다른 스레드 취소 요청

- `int pthread_cancel(pthread_t tid)`

- 성공시 1반환



### pthread 종료시 특정함수 호출되도록 설정

`pthread_cleanup_push()`

자원 정리 함수를 **스택에 푸시(push)**

인자 1:   지정된 마무리 함수를 스택에 등록

인자 2: 마무리 함수에서 쓸 인자



`pthread_cleanup_pop(0)`

스레드를 **강제로 종료**할 때 사용되거나, **자원 정리를 건너뛰고 싶을 때** 사용

`pthread_cleanup_pop(1)`

`cleanup_handler`호출


