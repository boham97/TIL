https://school.programmers.co.kr/learn/courses/30/lessons/59045

보호소에서 중성화한 동물

```
SELECT a.ANIMAL_ID, a.ANIMAL_TYPE, a.NAME
from animal_outs a
left join animal_ins b
on a.animal_id = b.animal_id
where a.SEX_UPON_OUTCOME != b.SEX_UPON_INTAKE
```

  



#### 식품분류별 가장 비싼 식품의 정보 조회하기

https://school.programmers.co.kr/learn/courses/30/lessons/131116

```
-- 코드를 입력하세요
select a.CATEGORY, a.PRICE MAX_PRICE, a.PRODUCT_NAME
from food_product a
left join (select b.category, max(b.price) bb from food_product b
          group by category) c
on a.category = c.category
where bb = a.price
and a.category in ( '과자', '국', '김치', '식용유')
order by a.price desc  
```

```
-- 코드를 입력하세요
select CATEGORY, PRICE MAX_PRICE, PRODUCT_NAME
from food_product
where (category, price) in (
    select category, max(price)
    from food_product
    group by category
    having category in ('과자', '국', '김치', '식용유'))
order by price desc
```





#### 5월 식품들의 총매출 조회

https://school.programmers.co.kr/learn/courses/30/lessons/131117

```
-- 코드를 입력하세요
SELECT a.PRODUCT_ID, a.PRODUCT_NAME	, sum(b.amount * a.price) TOTAL_SALES
from food_product a
inner join (select * from food_order
           where PRODUCE_DATE like '2022-05-%') b
on a.product_id = b.product_id
group by a.product_name 
order by total_sales desc, product_id
```

ㅁㄴㅇ

#### 서울에 위치한 식당목록 조회

https://school.programmers.co.kr/learn/courses/30/lessons/131118

```
-- 코드를 입력하세요
SELECT a.REST_ID, a.REST_NAME, a.FOOD_TYPE, a.FAVORITES, a.ADDRESS, round(avg(b.review_score), 2) score
from rest_INfo a
inner join rest_review b
on a.rest_id = b.rest_id
group by a.rest_id having score is not null and a.address like '서울%'
order by score desc, a.favorites desc  

```

```

```


