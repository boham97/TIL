# 0907_django_사용자 인증및 권한

## User Custom 준비

### 1.  `models.py`에 User class 정의

- `AbstractUser`를 상속받는다

### 2. Django 프로젝트에서  User를 나타내는데 사용하는 모델을 방금 생성한 커스텀 User 모델로 지정

- `settings.py`에 `AUTH_USER_MODEL`값을 설정해준다

### 3. admin.py 에 커스텀 User모델을 등록(선택사항)





## 쿠키 - 로그인, 장바구니 같은 상태를 유지시킬수 있따 (세션)

- session




























