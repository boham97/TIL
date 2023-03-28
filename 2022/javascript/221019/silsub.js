// 1번
const nums = [1,2,3,4,5,6,7,8]

for (let i = 0; i < nums.length; i++) {
  console.log()
}

// for (const i = 0; i < nums.length; i++) {
//                                     ^

// TypeError: Assignment to constant variable.

// 2번
for (let i = 0; i< nums.length; i ++) {
    num = nums[i]
    console.log(num, typeof num)
}

console.log('*\n**\n***\n****\n*****')

function palindrome(str) {
    temp = 'true'
    for (let i = 0; i < str.length; i ++) {
        if (str[i] === str[str.length-i-1] ) {

        }else {
            temp = 'false'
            break
        }
    }
    return temp
  }
  
  // 출력
  console.log( palindrome('level'))
  console.log( palindrome('hi'))
  // palindrome('hi') => false


const p1 = ['rock', 'paper', 'scissors', 'scissors', 'rock', 'rock', 'paper', 'paper', 'rock', 'scissors']
const p2 = ['paper', 'paper', 'rock', 'scissors', 'paper', 'scissors', 'scissors', 'rock', 'rock', 'rock']

const playGame = (p1_choice, p2_choice) => {
	// 작성해 주세요
	if (p1_choice === p2_choice) {
		return 0
	} else if ((p1_choice === 'rock'&&p2_choice === 'scissors')||(p1_choice === 'paper'&&p2_choice === 'rock')||(p1_choice === 'scissors'&&p2_choice === 'paper')) {
		return 1
	} else {
	    return 2
	}
}
for (let i = 0; i< p1.length; i++) {
    console.log(playGame(p1[i],p2[i]))
}
// 2
// 0
// 2
// 0
// 2
// 1
// 2
// 1
// 0
// 2


const participantNums =  [[1, 3, 2, 2, 1], 
[3, 7, 100, 21, 13, 6, 5, 7, 5, 6, 3, 13, 21],
[9, 1, 8, 7, 71, 33, 62, 35, 11, 4, 7, 71, 33, 8, 9, 1, 4, 11, 35]
]

// 3
// 100
// 62


for (participantNum of participantNums) {
    for (num in participantNum) {
        let temp = participantNum.filter()
    }
    console.log(flag)
}

