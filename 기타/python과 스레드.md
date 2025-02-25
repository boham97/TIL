### **GIL(Global Interpreter Lock, 전역 인터프리터 락)란?**  
**GIL**은 **Python 인터프리터에서 한 번에 하나의 스레드만 실행되도록 제한하는 락(Lock)**입니다.  
즉, 멀티스레딩 환경에서도 **동시에 여러 개의 Python 바이트코드를 실행할 수 없도록** 막아둔 장치입니다.  

---

### **GIL이 필요한 이유**  
1. **파이썬의 메모리 관리 방식 (Reference Counting)**  
   - Python 객체는 **참조 카운트(Reference Count)** 를 사용하여 메모리를 관리합니다.  
   - 여러 스레드에서 같은 객체를 수정하면, 참조 카운트가 충돌할 수 있습니다.  
   - GIL이 이를 방지하여 **안전한 메모리 관리**를 보장합니다.  

2. **Python 인터프리터 구현 (CPython 기준)**  
   - CPython은 **Thread-Safe한 가비지 컬렉터(GC)** 가 없어서, GIL을 사용하여 **GC의 안정성을 유지**합니다.  

---

### **GIL의 영향**  
✅ **I/O-bound 작업에서는 효과적**  
   - GIL이 있어도, 네트워크 요청, 파일 I/O 같은 작업 중에는 다른 스레드가 실행될 수 있음.  
   - → `threading` 모듈을 사용한 병렬 I/O 작업은 성능 향상 가능!  

❌ **CPU-bound 작업에서는 병목 발생**  
   - CPU를 많이 사용하는 연산(예: 수학 계산, 데이터 처리)은 GIL이 **한 번에 하나의 스레드만 실행**하게 막아서,  
     멀티스레드로 실행해도 성능이 오히려 떨어질 수도 있음.  
   - → `multiprocessing` 모듈을 사용하여 GIL을 우회해야 함.  

---

### **GIL을 우회하는 방법**
1. **멀티프로세싱(`multiprocessing` 모듈) 사용**  
   - GIL은 **프로세스 간에는 적용되지 않음!**  
   - → 여러 개의 **프로세스**를 만들어 병렬 처리를 하면 GIL의 영향을 받지 않음.  
   - 예제:
     ```python
     from multiprocessing import Pool

     def square(n):
         return n * n

     with Pool(4) as p:  # 4개의 프로세스를 사용
         print(p.map(square, [1, 2, 3, 4, 5]))
     ```
   
2. **C 확장 모듈 사용 (NumPy, Cython 등)**  
   - NumPy 같은 라이브러리는 **내부적으로 C에서 GIL을 해제**하고 실행하므로,  
     CPU 연산이 많은 작업도 병렬로 수행할 수 있음.  
   - `Cython`을 사용해 직접 GIL을 해제할 수도 있음.  

3. **PyPy 사용 (일부 GIL 제한 해결 가능)**  
   - PyPy도 GIL이 있지만, JIT 컴파일러 덕분에 CPython보다 **스레딩 성능이 개선**됨.  
   - 하지만 **완전한 GIL 제거는 아님!**  

---

### **정리**
| 구분 | GIL 영향 | 해결 방법 |
|------|---------|----------|
| **I/O-bound 작업** | 영향 적음 | `threading` 사용 가능 |
| **CPU-bound 작업** | 성능 저하 (병목 발생) | `multiprocessing`, NumPy, Cython 활용 |

즉, **I/O 작업은 `threading`, CPU 작업은 `multiprocessing`을 사용하면 GIL 문제를 피할 수 있습니다!** 🚀