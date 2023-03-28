# AJAX

### AJAX

- 비동기 통신으로 화면을 새로고치지 않고 일부분 수정

- asynchronous javascript and xml

- 페이지 새로 고침 없이 서버 요청

- 서버로 부터 응답을 받아 작업을 수행 (DOM)

### 좋아요

- 내가 누른 버튼 DOM 조작하기

- axios요청으로 좋아요 보내기

- 응답 받은 데이터로 DOM 변경

- 과정
  
  - `querlySelectorAll` 모든 좋아요 버튼 찾기
  
  - `foreach`로 모든 버튼에서
  
  - `eventListener` submit이 일어나면
  
  - `event.targetdataset`에서 `article-id`받아오기
  
  - html: article-id, js: artucleId(자동변환)
  
  - js에선 ``/articles/${articleId}/likes/`  '`로 표현 fstring처럼
  
  - `redirect`되는것을 막기위해  `preventdefault()` 를 사용해서 `csrf token` 전달이 안됨
  
  - axios에 `csrf token `전달 필요
  
  - `document.querySelector('[name=csrfmiddlewaretoken]').value`  `csrf token`값 추출
  
  - `axios`의 `headers`에 값 전달
  
  - `views.py`에서 좋아요 여부를 체크하고 딕셔너리 형태로 저장해서
  
  - `JsonResponse`형태로 `return`




