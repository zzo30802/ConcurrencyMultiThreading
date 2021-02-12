#include <assert.h>
#include <stdio.h>

#include <algorithm>
#include <chrono>
#include <deque>
#include <exception>
#include <iostream>
#include <list>
#include <mutex>
#include <numeric>  // std::accumulate
#include <stack>
#include <string>
#include <thread>
#include <vector>
/**
 * 1 : 3.2.1 mutex 
 * 2 : 3.2.2 Structuring code for protecting shared data
 * 3 : 3.2.3 Spotting race conditions inherent in interfaces
 * 4 : 3.2.4 Deadlock: the problem and a solution
**/

#define test_idx 4

#if test_idx == 1
/*
std::lock_guard<std::mutex> 可以使這塊記憶體同時間只能有一種操作，
所以不會同時發生"讀取"與"處存"的操作。

* 在C++ 17中，
std::lock_guard<std::mutex> guard(some_mutex);
可以簡化為
std::lock_guard guard(some_mutex);

* 在C++ 17中有加強版的保護機制std::scoped_lock，同樣的程式可以寫成
std::scoped_lock guard(some_mutex);
*/
std::list<int> some_list;  // 1
std::mutex some_mutex;     // 2
void add_to_list(int new_value) {
  std::lock_guard<std::mutex> guard(some_mutex);  // 3
  some_list.push_back(new_value);
}

bool list_contains(int value_to_find) {
  std::lock_guard<std::mutex> guard(some_mutex);  // 4
  return std::find(some_list.begin(), some_list.end(), value_to_find) != some_list.end();
}

#elif test_idx == 2
/*
這裡展示了使用std::lock_guard<std::mutex>保護並不是萬能的，
如果使用pointer或reference就有可能失效。

1 : 傳入受保護的資料給user-supplied function
2 : 傳入惡意函式
3 : 無保護地訪問受保護的data，這時如果對data做存取都可能造成core dump

在範例中，process_data沒有問題(因為使用了std::lock_guard作保護)，
但如果呼叫了foo()就可繞過保護機制，在沒有mutex保護下調用do_something()。

這種情況下，C++無法提供解決辦法，只能由開發者使用正確的互斥鎖來保護data，
或是 "不將受保護數據的pointer || reference"傳遞到互斥鎖作用域之外"。
*/

class some_data {
  int a;
  std::string b;

 public:
  void do_something();
};

class data_wrapper {
 private:
  some_data data;
  std::mutex m;

 public:
  template <typename Function>
  void process_data(Function func) {
    std::lock_guard<std::mutex> l(m);  // 1
    func(data);
  }
};

some_data* unprotected;

// 故意傳入一個reference給func
void malicious_function(some_data& protected_data) {
  unprotected = &protected_data;
}

data_wrapper x;
void foo() {
  x.process_data(malicious_function);  // 2
  unprotected->do_something();         // 3
}

#elif test_idx == 3
/*
這個範例如果在多線程操作可能會使某些功能返回的結果會有問題，
比如push()了新元素到stack中，但同時其他線程呼叫了pop()或top()可能會返回不同結果
*/
// stack 容器實現
template <typename T, typename Container = std::deque<T> >
class stack {
 public:
  explicit stack(const Container&);
  explicit stack(Container&& = Container());
  template <class Alloc>
  explicit stack(const Alloc&);
  template <class Alloc>
  stack(const Container&, const Alloc&);
  template <class Alloc>
  stack(Container&&, const Alloc&);
  template <class Alloc>
  stack(stack&&, const Alloc&);

  bool empty() const;
  size_t size() const;
  T& top();
  T const& top() const;
  void push(T const&);
  void push(T&&);
  void pop();
  void swap(stack&&);
  template <class... Args>
  void emplace(Args&&... args);  // C++14的新特性
};
/*
為了解決這問題有三種方法 : 
  1. PASS IN A REFERENCE
    用另一個容器接收pop出來的元素，在時間與空間資源上並不划算。
    且儲存類型的限制有可能造成使用者自定義的資料型別無法使用。
    std::vector<int> result;
    some_stack.pop(result);
  
  2. REQUIRE A NO-THROW COPY CONSTRUCTOR OR MOVE CONSTRUCTOR
  3. RETURN A POINTER TO THE POPPED ITEM
    直接return被pop項目的pointer，可以利用std::shared_ptr解決memory管理的問題(new, delete)
  
  可以使用1+2或1+3
*/

/* 
範例使用 1 + 3
當stack為空時，pop()會拋出empty_stack異常
*/
// struct empty_stack : std::exception {  // #include <exception>
//   const char* what() const throw();
// };

// template <typename T>
// class threadsafe_stack {
//  public:
//   threadsafe_stack();
//   threadsafe_stack(const threadsafe_stack&);
//   threadsafe_stack& operator=(const threadsafe_stack) = delete;  // 賦值操作被刪除

//   void push(T new value);
//   std::shard_ptr<T> pop();
//   void pop(T& value);
//   bool empty() const;
// };

/*
thread safe stack
具有保護機制的stack實現
*/
struct empty_stack : std::exception {
  const char* what() const throw() {
    return "empty stack!";
  };
};
template <typename T>
class threadsafe_stack {
 private:
  std::stack<T> data;
  mutable std::mutex m;

 public:
  threadsafe_stack() {}

  // 允許複製建構子(copy constructor)
  threadsafe_stack(const threadsafe_stack& other) {
    std::lock_guard<std::mutex> lock(other.m);
    data = other.data;  // 1 copy constructure
  }

  // 不能用賦值的方式來copy stack
  threadsafe_stack& operator=(const threadsafe_stack&) = delete;  // 刪除用=做賦值的功能(删除函数)

  void push(T new_value) {
    std::lock_guard<std::mutex> lock(m);
    data.push(new_value);
  }

  std::shared_ptr<T> pop() {
    std::lock_guard<std::mutex> lock(m);
    // 調用pop前，先確認stack是否為空
    if (data.empty()) throw empty_stack();

    // 在pop前，先用指標指向要被pop的位置當作回傳值
    std::shared_ptr<T> const res(std::make_shared<T>(data.top()));
    data.pop();
    return res;
  }

  void pop(T& value) {
    std::lock_guard<std::mutex> lock(m);
    if (data.empty()) throw empty_stack();

    value = data.top();
    data.pop();
  }

  bool empty() const {
    std::lock_guard<std::mutex> lock(m);
    return data.empty();
  }

  int size() const {
    std::lock_guard<std::mutex> lock(m);
    return data.size();
  }
};
int main() {
  threadsafe_stack<int> a;
  a.push(1);
  std::cout << a.size() << std::endl;
}

#elif test_idx == 4
/*
Deadlock 鎖死
1 : 鎖死兩個互斥量
2 & 3 : 創建兩個std:lock_guard()實例，提供std::adopt_lock參數表示
        std::lock_guard可以獲取鎖之外，還將鎖交由std::lock_guard管理，就不需要
        去手動unlock了。
        std::adopt_lock可以將lock住的鎖託管給lock_guard，如此會在函式結束時自動調用unlock，
        就不會發生鎖死情況。
*/

class some_big_object;
void swap(some_big_object& lhs, some_big_object& rhs);
class X {
 private:
  some_big_object some_detail;
  std::mutex m;

 public:
  X(some_big_object const& sd) : some_detail(sd) {}

  friend void swap(X& lhs, X& rhs) {
    if (&lhs == &rhs)
      return;
    std::lock(lhs.m, rhs.m);                                     // 1
    std::lock_guard<std::mutex> lock_a(lhs.m, std::adopt_lock);  // 2
    std::lock_guard<std::mutex> lock_b(rhs.m, std::adopt_lock);  // 3
    swap(lhs.some_detail, rhs.some_detail);
  }
};

/*

*/
#endif