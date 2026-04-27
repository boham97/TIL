# SIMD 실험 노트

## 예제: 이미지 밝기 조절 (brightness.c)

4096x4096 픽셀(16,777,216개)에 +50 밝기 적용.

### 핵심 코드

```c
// 스칼라: 1번에 1픽셀
for (int i = 0; i < n; i++) {
    int v = src[i] + add;
    dst[i] = v > 255 ? 255 : (uint8_t)v;
}

// AVX2: 1번에 32픽셀
__m256i vadds = _mm256_set1_epi8((int8_t)add);   // add를 32개 레인에 복제
__m256i chunk  = _mm256_loadu_si256(...);          // 32픽셀 로드
__m256i result = _mm256_adds_epu8(chunk, vadds);  // saturating add (자동 clamp)
_mm256_storeu_si256(..., result);                  // 32픽셀 저장
```

`_mm256_adds_epu8`의 `s` = saturating add → 255 초과 시 자동으로 255 고정, if문 불필요.

---

## 컴파일 옵션별 성능 비교

| 옵션 | 스칼라 | AVX2 | 배율 |
|------|--------|------|------|
| `-O0` | 52.74 ms | 3.39 ms | **15.6x** |
| `-O2 -fno-tree-vectorize` | 9.14 ms | 2.09 ms | 4.4x |
| `-O2` | 2.57 ms | 2.37 ms | 1.1x |

### 왜 -O2에서 차이가 없나?

GCC `-O2`는 **자동 벡터화(auto-vectorization)** 를 수행한다.
루프가 독립적이고 순차적이면 GCC가 알아서 AVX2 명령어(`vpaddusb %ymm`)로 변환.
→ 직접 짠 AVX2 코드와 거의 동일한 어셈블리가 생성됨.

확인 방법:
```bash
gcc -O2 -mavx2 -S brightness.c -o - | grep -E "ymm|xmm"
```

### 이론 최대치(32x)에 못 미치는 이유

AVX2는 32바이트를 한 번에 처리하므로 이론상 32x.
실제로는 **메모리 대역폭**이 병목 — 16MB 데이터를 계속 RAM에서 읽어야 해서 4~15x 수준.

---

## 컴파일 명령

```bash
# 기본 빌드
gcc -O2 -mavx2 -o brightness brightness.c

# 자동 벡터화 끄고 공정 비교
gcc -O2 -mavx2 -fno-tree-vectorize -o brightness brightness.c

# 최적화 없이 (스칼라 진짜 성능 확인)
gcc -O0 -mavx2 -o brightness brightness.c
```

- `-mavx2` : AVX2 명령어 활성화 (없으면 intrinsic 컴파일 안 됨)
- `-fno-tree-vectorize` : GCC 자동 벡터화 비활성화
- `immintrin.h` : AVX2 intrinsic 함수 정의 헤더 (`_mm256_*`)

---

## SIMD를 직접 써야 하는 경우

GCC가 자동 벡터화 못 하는 패턴:
- 간접 참조: `dst[i] = table[src[i]]`
- 반복 간 의존성이 있는 루프
- gather / shuffle / permute 같은 특수 명령어가 필요한 경우

일반 루프는 `-O2`로 컴파일러에 맡기고, 프로파일링 후 병목에만 직접 사용하는 것이 현실적.
