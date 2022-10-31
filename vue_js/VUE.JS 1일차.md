# VUE.JS 1일차

### SPA <ㅡㅡㅡ> SSR(server side rendering)

- single page application

- 서버에서 최초 1장의 html만 전달받아 모든 요청에 대응하는 방식

- CSR (client side rendering) 방식으로 요청 처리함
  
  - 페이지를 받고 필요한 페이지를 서버에 AJAX로 요청
  
  - json 방식으로 데이터 전달
  
  - json 데이터를 JS로 처리, DOM 트리에 반영
  
  - 장점
    
    - 모든 html페이지를 서버로 받지 않아도됨
      
      - 트래픽 감소
      
      - 응답속도 감소
    
    - 요청이 끊임없이 진행
      
      - 자연스러움
    
    - 백과 프론트의 영역을 명확히 분리 할 수이씀
  
  - 단점
    
    - 첫 구동에 필요 데이터가 많을수록 오래걸림
    
    - 검색엔진 최적화가 어려움
      
      - 처음에 빈 html을 받기 때문에 상단에 노출되기 어럽따 ;(

## VUE with HTML

- vue 2 문서 보기
  
  - vue2 vs vue3
    
    - vue2 legacy code가 더 많음
    
    - 아직 vue2가 우세한 편

- `new` 생성자 함수

- 하나의 data를 변경시 DOM조작으로 한번에 변경가능!

## VUE Instance

- MVVM Pattern

- view <ㅡ> viewmodel <ㅡ> model

- (DOM)           (VUE)                (JAVE SCRIPT)

- view: 실제 눈에 보이는 부분

- MODEL: 실제 데이터

- viewModel: view를 위한 model,view와 binding 되어 action을 주고 받음

- view와 model이 독립적이지만 viewmodel로 연결됨

- `new` 생성자 함수 동일한 구조의 객체를 여러개 만들수있다

- `new` 뒤에 대문자!로 시작하기!

- `el: #app` id가 app과 연결 

- data
  
  - 반드시 기본 객체 {}
  
  - 모든 타입 가능
  
  - 정의된 속성은 `interpolation {{}}`으로 출력가능
  
  - `this`가 가르키는 객체가 무었인지 주의하기

## BASIC Syntax

### template Syntax

### Directives

- v-접두사가 있는 특수 속성애는 값을 할당가능

- `v-on:submit.prevent="onSubmit"`

- name   argument  modifiers   value

- `:`을 통해 전달읹자 받음

- `.` 특수 접미사 -directive를 특별한 방법으로 바인딩

- `v-text`출력 `{{}}`과 같은 역활

- `v-html` 사용자가 입력하거나 제공하는 컨테츠에는 사용금자!!! 

- `v-show` 표현식에 작성된값에 따라 보여줄지 결정, 레더링은 되지만 안보임
  
  - expensive inital load, cheap toggle

- `v-if` `v-show`와 사용방법은 같지만 false이면 DOM에서 사라짐(렌더링X)

- - cheap inital load expensive toggle

- `v-for`  반복, 배열
  
  - 반복은 value, key,  objects는 key, value순
  
  - `v-for`를 사용하면 key속성을 각 요소에서 작성
  
  - 키값 겹치면 경고

- `v-on` 
  
  - `< button v-on:click="number++">`
  
  - `:`을 통해 전달 받은 인자를 확인
  
  - 대기하던 이벤트가 발생하면 할당된 표현식 실행
  
  - method도 사용가능
  
  - `@` 로 약어로 사용
  
  - `@keyup.enter`, ... `vue2>>api>>on` cham jo

- `v-bind:`
  
  - 뒤에 오는 변수를 조정
  
  - `:`로 축약 가능





## VUE advanced

### computed

- 미리 계산된 값을 재호출하지않고 사용

- 종속된 값들이 변하면 재계산

- 계산된 값 `()`안 붙음!

### watch

- 특정 데이터 변화를 감지하는 기능

- 감시대상 데이터가 변하면 기존값, 변경후 값을 인자로 받는 함수 정의

- `deep =true` 배열이나 객체를 감시할때 사용

### filter

- 텍스트 형식화 를 적용할수 있는 필터

- 
