function add(num1, num2) { //선언 호이스팅 있어요
    return num1 + num2
}

console.log(add(3,5))

const sub = function(num1, num2) { //표현 호이스팅x 권장
    return num1 + num2
}
console.log(sub(3,4))

const greeting = function(name='naaame') {
    return `name= ${name}`
}
console.log(greeting())

//받는 인자보다 많이 들어와도 동작함

const rest0pr = function(arg1,arg2, ...restArgs) {}
/* 
... 나머지 args 배열로 들어감
*/


// arrow function  function 생략 매개변수가 하나면 ()생략, 함수가 한줄이면 {}, return 생략
const greeting2 = name =>`name= ${name}`
console.log(greeting('bosung'))

console.log(((num) => num**3)(2))