// 기존 방식
int max = (a > b) ? a : b;

// 비트 연산으로 대체
int diff = a - b;
int mask = -(diff >> 31);  // a > b이면 mask = 0, a < b이면 mask = -1
int max = a - (diff & mask);