# VUE

### Node.js

- 자바스크립트를 브라우저가 아닌 환경에서도 구동할수있게함

- Node Package Mange
  
  - pip같은 패키지관리자

### VUE CLI

- vue 개발을 위한 표준도구

- 프로젝트의 구성을 도와주는 역활

- 프로젝트생성
  
  - 설치: `npm install -g @vue/cli`
  
  - `vue create vue-cli`
  
  - vue2 설치
  
  - 프로젝트 디렉토리로 이동후`run serve`
  
  - `git init`이 되어있으므로 주의
  
  - node_modukes: venv랑 비슷
  
  - babel: 자바스크립트의 ES6+ 코드를 구버전으로 번역하는 도구
  
  - node- modules-webpack
    
    - static module bundler
    
    - 모듈간의 의존성문제 해결
    
    - 내부적으로 종속성 그래프를 빌드함
    
    - 무겁다! :(
  
  - package.json: 지원되는 브라우저에 대한 구성 옵션 포함
  
  - package-lock.json: requirements.txt. 수정 필요x
  
  - src  (source)
    
    - 정적파일 저장 디렉토리

### SFC

- cs에서는 재사용 가능한 범용성을 위해 개발된 소프트웨어 구성요소를 의미

- 하나의 앱을 구성할 때 중첩된 컴포넌트둘의 트리로 구성하는 것이 보편적임
  
  - 유지보수 쉬음

- Vue instance ==> new Vue()

- single file component
  
  - 하나의 .vue 파일이 하나의 뷰 인스턴스이고 하나의 컴포넌트
  
  - 하나의 컴포넌트가 하나의 기능을 하자

### VUE Component

- 템플릿

- 다른 컴포넌트 사용가능

- 스크립트(JS)
  
  - 자바스크립트 코드 작성
  
  - 컴포넌트 정보., 데이터 ,메소드 
  
  - 뷰 인스턴스 구성하는 대부분이 작성됨

- style
  
  - css 작성

### 실습

- 컴포넌트폴더에 파일추가

- name등록

- 최상위 태그 하나 존재

- App에 등록하기
  
  - 불러오기
    
    - .vue 생략 가능
    
    - instance name은 instance생성시 작성한 name
    
    - '@'는 src의 숏컽
  
  - 등록하기
  
  - 보여주기

### DATA in components

- 페이지들이 컴포넌트로 구분 되어있음

- 하위 컴포넌트에서 데이터를 쓰려면?
  
  - 필요컴포넌트 끼리 데이터를 주고받으면?
    
    - 데이터 흐름 파악 힘듬
    
    - 유지보수 난이도 ↑
    
    - ***부모자식 관계만 데이터를 주고 받자!***
  
  - pass props: 부모→ 자식
  
  - emit event: 자식→ 부모

- **pass props**
  
  - 요소의 속성을 사용하여 데이터 전달
  
  - 자식 컴포넌트는 props옵션을 사용하여 수신하는 props를 명시적으로 선언해야
  
  - props in helloworld
    
    - 어떤 타입의 pros를 받는지 명시
    
    - 정적인 데이터 전달: static props
    
    - `prop-data-name="value"` 케밥케이스로 보내야 한다.
      
      - html에선 대소문자 구분할수 없기때문
      
      - 받는곳은 카멜케이스로 `props: { propDataName: String }`(JS)
      
      - ★부모(케밥)→자식(카멜)★
  
  - Dynamic props
    
    - `:` v-bind를 사용하자
    
    - vue-cli에선 데이터를 함수 형태로 저장
      
      ```vue
        data: function () {
          return {
          }
        }
      ```
    
    - 각 vue 인스턴스는 같은 데이터를 공유,  각각 사용하기 위해 return으로 커버

- **emit Event**
  
  - 부모 컴포넌트에서 자식컴포넌트로 데이터 전달시 이벤트를 발생시켜
  
  - 인자로 데이터 보냄
  
  - ```vue
        methods: {
            childToparent() {
                this.$emit('child to parent','data from child')
            }
        },
    ```
  
  - `$emit(event-name)` 
    
    - event-name이라는 이벤트가 발생함을 알림
    
    - 여러단계 올리기
      
      - 함수내에서 다시 `this.$emit('~', 'data')`



# ETC

- component 요소
  @click.native  = 클릭
  @click = 클릭emit을 기대

- 

- btn에선 @click = 클릭
