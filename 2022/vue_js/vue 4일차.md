# vue 4일차

### intro

- 비슷한 것 끼리 묶어야 편함 => 체계적인 설계를 통해 기획

- UX
  
  - user experience
  
  - 데이터를 기반으로 유저를 조사하고 분석ㅎ서 개발자, 디자이너가 소통
  
  - 유저 리서치, 데이터 설계 및 정제, 유저 시나리오, 프로토타입 설계 등이 필요
  
  - 직무
    
    - 사용자 경험 이해 통계모델
    
    - 리서치 기획, 지표 정의
    
    - 사용자 조사

- UI
  
  - User Interface
  
  - 유저에게 보여지는 화면 디자인
  
  - UX를 고려한 디자인 반영, 기능개선=> 프론트 엔드와 소통
  
  - interface
    
    - 서로 다은 두개의 시스템이 ..?
  
  - 가장 중요한 것은 **협업**

### UX UI 디자인

https://cantunsee.space/

[UI 디자인 기본 원칙 - Apple Developer](https://developer.apple.com/kr/design/tips/)

### protityping

- Figma
  
  - 인터페이스 디자인 협업 애플리케이션
  
  - **협업** 중점
  
  - 웹 기반
  
  - 다양한 플러그인 존재
  
  - 대부분 무료

- 개발전 충분한 기획을 거칠것

- 대략적인 모습을 그려보는 과정이 필요

# vue router

## routing

- 네트워크에서 경로를 선택하는 프로세스

- 웹 서비스에서 라우팅: 유저가 방문한 url에 대해 적절한 결과를 응답하는것

- 서버가 모든 라우팅 통제 `views.py`, SSR

- client side rendering, ignle page application
  
  - 서버는 하나의 html제공
  
  - 이후 모든 동작 JS
  
  - **하나의 URL 가짐**

- 라우팅이 없으면
  
  - 뒤로가기 불가
  
  - 링크 공유시 처음 페이지만 공유가능

### vue router

- vue 공식 라우터

- SPA 상에서 라우팅으 쉽게 개발할 수 있는 기능제공

- spa를 mutiple page application처럼 사용가능

- spa의 단점인 url을 변경되지않는점을 해결

- `vue add router`로 설치

- history mode 사용하기
  
  - history api를 활용하는 방식
  
  - 새로고침없이 url이동기록 남김 (익숙한 구조)
  
  - 사용안하면 hash mode로 설정됨('#'으로 url 구분하는 방식)

- router-link
  
  - a태그와 비슷한 기능=> URL을 읻동
  
  - 클릭이벤트가 차단, 브라우저가 페이지를 다시 로드
  
  - 목표 경로 'to'    속서으로 지정됨

- router-view
  
  - 주어진 URL에 대해 일치하는 컴포넌트를 렌더링 하는 컴포넌트

- router/index.js
  
  - 라우터 정보및 설정
  
  - routes에 url과 컴포넌트를 매핑

- src/views
  
  - router-view에 들어갈 component작성 (router와 매핑 되는 컴포넌트만)
    
    - 이름이 view끝나도록 작성
  
  - components에 작성하는 것과 다름없음

### vue router 실습

- 주소 이동
  
  - 선언적
    
    - router -link 의 `to`속성으로 주소 전달
    
    - `:to="{name: 'home'}"`
  
  - 프로그래밍 방식
    
    - vue 인스턴스 내부에서 라우터 인스턴스에 `$router`로 접근
    
    - `this.$router.push`
    
    - history stack에 push하는 방식
    
    - 기록이 남아서 뒤로가기 가능

- dynamic route matching
  
  - 동적인자 전달
  
  - path를 통해 원하는 인자 받고  data에 저장후 사용 

```vue
  {
    path: '/hello/:userName',
    name: 'hello',
    component: HelloView
  }
```

```vue
<template>
  <div>
    <h1>hello, {{ userName }}</h1>
  </div>
</template>

<script>
export default {
    name: 'HelloView',
    data() {
        return {
            userName: this.$route.params.userName,
        }
    }
}
</script>
```

- `this.$router.push({ name:'hello', params: (userName: this.sending} })`
  
  - 인자를 받아 프로그래밍 방식으로 주소이동

- lazy-loading 방식
  
  - 최초에 렌더링 하지 않고 동작시 컴포넌트 렌더링

```vue
    path: '/about',
    name: 'about',
    component: () => import('../views/AboutView.vue')
  },
```

### 네비게이션 가드

- 전역가드
  
  - <mark>다른 url주소</mark>로 이동할 때 항상 실행
  
  - `router.beforeEach(to, from, next)` 를 사용  at `router/index.js`
  
  - to 이동할 url정보가 담긴 route객체
  
  - from 현재 url정보
  
  - next w지정한 url로 이동하기 위해 호출하는 함수
    
    - 콜백 함수 내부에서 반드시 한번만 호출
    
    - 기본적으로 to에 지정한 url로 이동
    
    - next가 호출되지 않으면 전환X

- 라우터 가드
  
  - `beforeEnter(to, from,next)` 특정 route에 대해 실행하고싶을 때
  
  - 라우터를 등록한 위치에 추가
  
  - **다른 경로에서 올 때 실행**

- 컴포넌트 가드
  
  - 특정 컴포넌트 내에서
  
  - `beforeRouteUpdate()`





### 실습

- `?.` 객체가 있을때 출력 없으면 undefined
  
  - `article?.title`
