# N:M

## Intro

### manytomanyfield()

- 앱_모델_필드명으로 중개테이블 생김

- 둘중 한 클래스에 manytomanyfield() 작성

-  추가 데이타를 저장하려면 장고 중개테이블 사용 불가

- related manager
  
  - add, remove로 추가, 제거 create,clear, ...
  
  - N:M에서 관련된 두 객체 모두 사용 가능

- `through`설정 및 reservation class수정(추가데이터)
  
  - through 설정시 add, remove 사용 불가
  
  - 각각 필드에 값을 넣고 save해야함

- related_name
  
  - 역참조 옵션
  
  - 기존의 _set은 못 씀
    
    - ex) doctors.patients.all()
  
  - Foreignkey의 related_name과 같음

- symmetrical
  
  - 대칭을 원하지 않는 경우 False  (Follow)



# 
