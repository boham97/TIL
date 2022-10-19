# STATIC FILES

- 응답할 떄 별도의 처리 없이 파일 내용을 그대로 보여주면 되는 파일

- 반드시 load 필요 {% static %} extend 밑에 위치

- 이미지, css, java script파일이 속함

- 서비스 제공자가 소유

- Media File
  
  - 사용자가 웹에서 업로드하는 정적 파일
  
  - 유자가 업로드한 모든 정적파일
  
  - 미디어 파일

- 스태틱파일 구성
  
  ![](STATIC%20FILES_assets/2022-10-11-13-02-17-image.png)

- static files 관련 core settings
  
  - STATIC_ROOT
    
    - 배포시 필요
    - 장고 프로젝트에서 사용하는 모든 정적 파일을 한곳에 모아넣는 경로
  
  - STATICFILES_DIRS 
    
    - 앱 폴더 외부에 스태틱 파일이 존재하면 경로를 등록 
    
    - 추가 파일 디렉토리에 대한 전체 경로를 포함하는 문자열 목록으로 작성되어야함(`app/static/` 외에 추가적인 경로)
  
  - STATIC_URL
    
    - 기본으로 설정되있음
    
    - 외부에서 웹으로 접근하므로 이미지도 웹주소로 접근 ->웹주소 설정해줌
    
    - 앞뒤 slash`/`로 끝나야함



### static file 가져오기

- 기본경로에 있는 스태틱 파일 가져오기

![](STATIC%20FILES_assets/2022-10-13-09-23-43-image.png)

- 추가 경로 인경우

- ![](STATIC%20FILES_assets/2022-10-13-09-24-26-image.png)



# image upload

### ImageField()

- 이미지 없로드에 사용하는 모델 필드

- 이미지 파일 경로가 저장됨

- 최대 길이 100자 문자열

- `filefield(upload_to'',..)` 
  
  - 파일 업로드에 사용하는 모델 필드
  
  - default: ''

-  설정
  
  - MDEIA_ROOT
    
    - 업로드된파일이 저장될 절대경로
    
    - `MEDIA_ROOT = BASE_DIR / 'media'`
  
  - MEDIA_URL
    
    - satic_url과 동문, 다른경로로 지정해야

- 개발단계에서 사용자가 업로드한 미디어 파일 제공받기

![](STATIC%20FILES_assets/2022-10-13-09-34-00-image.png)

- imageField 작성

![](STATIC%20FILES_assets/2022-10-13-09-35-48-image.png)

- blank
  
  - True 인경우 DB에 빈문자열 저장
  
  -  True이면 유효성 검사에 빈 값 입력 가능
  
  - null=Ture 는 유효성검사에서 걸림

- migration
  
  - pillow 라이브러리 필요함!

- 파일 업로드시
  
  - `enctype`속성을 변경해줘야함!

![](STATIC%20FILES_assets/2022-10-13-09-40-54-image.png)

- 이미지 출력

![](STATIC%20FILES_assets/2022-10-13-09-45-40-image.png)



### image resizing

- 원본 이미지는 부담이 큼

- 업로드 될때 resizing
  
  - django-imagekit 모듈 설치및 등록

- 썸네일 만들기
  
  - 원본 이미지 저장X
    
    - 외장 모듈 import
      
      - processedimagefield()
      
      - thumnail
  
  - 원본 이미지 저장O
    
    - 외장 모듈 
      
      - processedimagefield()
      
      - imagespecfield()





# queryset API advanced

### CRUD 기본

`User.objects.all()`

`User.objects.get(pk=101)`

- 인원조회

`User.objects.count()`

`len(User.objects.all())`

### sorting data

### order_by

![](STATIC%20FILES_assets/2022-10-11-15-10-17-image.png)

- 2개면 마지막꺼 호출

- 나이 오름차순, 이름 나이 조회
  
  `User,objects.order_by('age').values('first_name','age')`
  
  ```python
  User,objects.order_by('age','-balance').values('first_name','age','balance')
  ```
  
  ### values()

- 모델 인스턴스가 아닌 딕셔너리 요소들을 가진 queryset을 반환

### filtering data

- 중복없이 모든 지역 조회
  
  - `User.objects.distinct().values('country')`

- 지역순으로 오름차순 중복없이 모든 지역 조회
  
  - `User.objects.distinct().values('country').order_by('country')`

- 나이가 30인 사람 이름
  
  - `User.objects.filter(age=30).values('first_name')`

- 나이가 30 이상인 사람 이름
  
  - `User.objects.filter(age__gte=30).values('first_name')`
  
  - field lookups
    
    - filter(), exclude(), get()에 대한 키워드 인자로 사용
    
    - gt: great than
    
    - gte: grater than , equal
    
    - `__contains='호'` 호가 포함된
    
    - `__startswith='~'` '~'로 시작하는
    
    - `__endswith= '~'` `~`로 끝나는
    
    - `__in=[...]`[...]에 있는

- exclude() 부정필터

- 경기도와 강원도에 살지 않는 사람들
  
  - `User.objects.exclude(country__in=['경기도', '강원도').values('first_name,'country')`

- 슬라이싱을 이용해 제한 가능 `[:10]`

### Q object

&와 |로 결합 할수 있음

- from django.db.models import Q

- `User.objects.filter(Q(age=30) | Q(last_name='kim'))`

- `,`로 &처럼 쓸수 있음

# Aggregation(Grouping data)

### aggregate

- 딕셔너리를 반환

- 특정 필드 전체의 합 평균 개수등을 계산

- avg, count, max, min, sum...

- 30살 이상인 사람들의 평균 나이 조회
  
  - `User.objects.filter(age__gte=30).aggregate(ssafy = Avg('age'))`
  
  - -> {'ssafy': ~~}

### annotate()

쿼리의 각 항목의 요약값을 계산

- 각 지역 별로 몇 명 살고 있는지 + 지역별 계좌 잔액 평균 조회
  
  - `User.objects.values('country').annotate(Count('country'), Avg('balance')`
