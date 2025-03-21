`malloc`으로 할당한 배열이 가득 차면, 배열 크기를 늘리기 위해서는 **`realloc`** 함수를 사용해야 합니다. `realloc` 함수는 기존에 할당된 메모리 블록의 크기를 변경하고, 필요시 새로운 메모리 위치에 데이터를 복사하여 더 큰 메모리 블록을 할당합니다.

### `realloc` 사용 방법:

- 기존의 배열에 더 많은 요소를 저장해야 할 경우, `realloc`을 사용하여 배열의 크기를 늘릴 수 있습니다.
- 새로운 크기를 지정하여 기존 데이터를 유지한 채 메모리 크기를 조정할 수 있습니다.

### 함수 원형:

c

코드 복사

`void* realloc(void *ptr, size_t size);`

- **`ptr`**: 기존에 할당된 메모리 블록의 시작 주소.
- **`size`**: 새롭게 할당할 메모리의 크기.

### 반환값:

- **성공 시**: 크기가 조정된 새 메모리 블록의 포인터를 반환합니다.
- **실패 시**: `NULL`을 반환하며, 기존 메모리는 유지됩니다.

# 

### 주의사항:

- **`realloc`**이 실패하면 `NULL`을 반환하므로, 이를 반드시 확인해야 합니다.
- 실패 시 기존의 메모리 블록은 손실되지 않으므로, 이전 포인터로 안전하게 메모리를 해제할 수 있습니다.
