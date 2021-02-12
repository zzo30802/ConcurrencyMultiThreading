# ConcurrencyMultiThreading
## 2. Thread management

#### concurrency (併發) : 指兩個or兩個以上的獨立線程同時發生活動。處理來自不同執行緒的共享狀態訪問(race condition)。

#### Parallel (併行) : 利用多個CPU來提高硬體效能。
這意味著在單核CPU中可以實作concurrency，但不能Parallel。

- **[2-1~2-3](src/2.cpp) : Managing threads**
    - (1) create std::thread: uniform initialization, Lambda
    - (2) Waiting for a thread to complete: detach():當主線程結束後，子線程也會結束。
        - 使用detach()表示不等待此thread，在某些情況下可能造成讀取已經釋放的記憶體位置。也就是使線程在後台執行，不必關注何時會釋放。
    - (3) Waiting for a thread to complete: join():主線程會等待子線程結束
        - 使用了try/catch 確保退出thread後，函式才會結束。
    - (4) end thread: RAII(資源取得即初始化, Resource Acquisition Is Initialization)
        - 自定義一個thread_guard類來管理並銷毀子線程。
    - (5) daemon threads(分離線程，發後即忘): 
        - detach()使用時機範例
        - 範例: 使用detach來分離線程處理文檔
        - std::thread可以傳遞參數
    - (6) Passing arguments to a thread function
        - std::thread中傳遞參數使用std::ref
        - 利用引用可以使std::thread傳遞function所需的參數
        - std::thread傳遞第一個成員可以是某class下的function，使此線程只會調用這個class的某個function
    - (7) Transferring ownership of a thread
        - std::move()
        - scoped_thread: 直接在建構子中檢查thread是否可以匯入
        - joining_thread: 直接在建構子中檢查thread是否可以匯入
    - (8) Spawn some threads and wait for them to finish
        - std::mem_fn()
        - use std::mem_fn() && std::for_each() to create multiple threads and join it
- **[2-4](src/2-4.cpp) : Choosing the number of threads at runtime**
    - (1) 範例中實現了Parallel版本的std::accumulate()。
    - (2) std::accumulate() 累加thread所執行function運算的結果
    - (3) std::thread::hardware_concurrency()在新版C++中非常有用，其返回CPU核心的數量，當無法獲取時則會返回0。
- **[2-5](src/2-5.cpp) Identifying threads**
    - (1) std::thread::id 標示線程編號
    - (2) std::this_thread::get_id()對線程進行檢索
- **[2-6] Summary**
    - (1) starting threads
    - (2) waiting for threads to finish
    - (3) not waiting for threads to finish
    - (4) threads run in the background
    - (5) to pass arguments into the thread function
    - (6) to transfer the responsibility for managing a thread from one part of the code to another (std::move())
    - (7) how groups of threads can be used to divide work
    - (8) identifying threads in order to associate data or behavior with specific threads (id)

## 3. Sharing data between threads
    Problems with sharing data between threads
    Protecting data with mutexes
    Alternative facilities for protecting shared data

software transactional memory (STM)
- **[3-2](src/3-2.cpp) mutex**
    - (1) RAII idiom for a mutex (std::lock_guard && std::mutex)
    - (2) 