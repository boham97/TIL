```
import random
import time
import heapq


def fast_partial_sort(K, iterable):
    it = iter(iterable)  # 슬라이스 없이 iterator 사용
    heap = []

    # 먼저 K개 요소를 힙에 넣기
    for _ in range(K):
        try:
            heap.append(next(it))
        except StopIteration:
            break
    if not heap:
        return []

    heapq.heapify(heap)  # O(K)  K개 힙에 저장

    # 로컬 변수로 heapreplace 가져오기 (함수 호출 오버헤드 최소화)
    _heapreplace = heapq.heapreplace
    top = heap[0]

    # 나머지 요소 순회
    for x in it:
        if x > top:                 #root 노드값보다 크면 교체
            _heapreplace(heap, x)
            top = heap[0]

    # 마지막으로 힙 정렬
    heap.sort(reverse=True)
    return heap

def main():
    # 데이터 생성
    N = 40000
    K = 100
    data = [random.randint(1, 1_000_000) for _ in range(N)]
    
    # 방법 1: 전체 정렬 (qsort 유사)
    start = time.perf_counter()
    sorted_all = sorted(data, reverse=True)[:K]
    time_full = time.perf_counter() - start
    
    # 방법 2: heapq.nlargest (O(N log K))
    # start = time.perf_counter()
    # top_k = heapq.nlargest(K, data)
    # time_heap = time.perf_counter() - start
    
    # 방법 3: heapq
    start = time.perf_counter()
    partial_k = fast_partial_sort(K, data)
    time_part = time.perf_counter() - start


    # 결과 출력
    print(f"전체 정렬 (sorted)  : {time_full:.6f}초")
    # print(f"Top-{K} (heapq)    : {time_heap:.6f}초")
    print(f"part-{K} (heapq)    : {time_part:.6f}초")
    # print(f"속도 차이 배율     : {time_full / time_heap:.2f}배")
    print(f"속도 차이 배율     : {time_full / time_part:.2f}배")
    # print(sorted_all == top_k, sorted_all == partial_k, '\n', *sorted_all)
if __name__ == "__main__":
    main()

```
1. temp = arr[:k] 는 새로운 객체생성하므로 지향
2. `_heapreplace = heapq.heapreplace` 함수를 로컬에 바인딩해 오버헤드 줄이기
3. heap 으로 구현시 가중치가 같은 경우를 대비해 순서를 다음 가중치로 쓰기 