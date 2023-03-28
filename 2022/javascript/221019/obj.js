const myInfo = {
    name: 'jack',
    phone: '1234-5678',
    'samsung product': {
        buds: 'Buds pro',
        galaxy: 's99',
    },
}

console.log(myInfo.name)
console.log(myInfo['name'])
console.log(myInfo['samsung product'])
console.log(myInfo['samsung product'].galaxy)


const obj = {
    name: 'jack',
    greet()  {
        console.log('hi')
    }
}
obj.greet()

const a = [1,2,3]
const b = [0, ...a,4,5]
console.log(b)