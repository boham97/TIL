### js동기와 비동기

동기

- 모든일이 순서대로 하나씩 처리하는것

비동기

- 결과를 기다리지 않고 다음 작업 처리

- 병렬적 수행



### js 비동기 처리

- single thread 언어

- 한번에 하나의 일만 수행

- js runtime
  
  - 비동기 처리를 도아주는 환경이 필요
  
  - 브라우저  node에서 처리
    
    - js engine의 callstack - 함수가 stack형태로 실행
    
    - web api - 오래걸리는 작업을 별도로 처리
    
    - task queue - 오래걸리는 작업이 끝나면 que에 작업 삽입
    
    - event loop - evenloop가 callstack이 비었는지 체크하고 비었으면  call stack으로 pop
    
    - settime이 있으면 무조건 task queue로 간다



### AXIOS

- js http웹통신 라이브러리

- ```js
  <script>
      axios.get('요청 url')
      .then(성공하면 수행할 콜백함수)
      .catch(실패하면 수행 콜백함수)
  ```

- 
  

- 비동기 처리 단점- 실행순서가 불분명:(

- 콜백함수- 다른 함수에 인자로 전달되는 함수

- 비동기 작업후 실행할 작업을 명시가능  => 비동기 콜백함수

- callback hell => 연속적인 비동기 함수들 가독성 저하 유지보수 x

- Promise
  
  - 비동기 처릴를 위한 객체
  
  - type(AXIOS) === promise

- axios
  
  - then 이전작업에서 성공결과를 인자로 받음
  
  - catch 작업의 실패 객체를 인자로 받음
  
  - then, catch 모두 promise 객체 반환하므로  then을 이어나갈수 있다
  
  - ```js
    axios.get(...)
    .then(...)
    .then(...)
    .then(...)
    .catch(...)
    
    
    ```
  
  - then chain이 생기려면 `return`이 있어야 한다!
  
  - ```js
    axios({
        method:'POST'
        url: ...
        ...
    })
        .then(...)
        .then(...)
    ```
  
  - 이런 형식이 `POST`나 다른 메소드에서 편함
  
  -  공식 문서 [요청 Config | Axios Docs](https://axios-http.com/kr/docs/req_config)


