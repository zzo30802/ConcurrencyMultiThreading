# ConcurrencyMultiThreading
## 2 線程管理
- [2-1](src/2-1.cpp)
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