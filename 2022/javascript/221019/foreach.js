const colors = ['r','b','g']

colors.forEach(function (color) {
    console.log(color)
})

colors.forEach(color =>console.log(color))

console.log()
console.log()
nums = [0,1,2,3,4]
newarr = nums.map(num => num ** 2)
console.log(newarr)

biggerThanTwo = nums.filter(num => num > 2)
console.log(biggerThanTwo)

biggerThanTwo2 = nums.filter( function (num) {num > 2} )
console.log(biggerThanTwo)

const sumNum = nums.reduce( function (result, number) {
    return result + number
},100)

const sumNum2 = nums.reduce((result, number) => {
    return result + number
},100)


console.log(sumNum)
console.log(sumNum2)

console.log(nums.reduce((result,num) => result + num, 0)/nums.length)

console.log()


const fruits = [
    {name: 'apple', num: 10},
    {name: 'orange', num: 20},
]
console.log(fruits.find((fruit) =>  fruit.num === 20))

console.log(nums.some( num => num%2 === 0 )) //빈배열 false
console.log(nums.every( num => num%2 === 0 )) // 빈배열 TRUE