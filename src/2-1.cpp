#include <stdio.h>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

/*
1 : 創建std::thread: uniform initialization, Lambda
2 : 結束thread: detach()
3 : 結束thread: join()
4 : 結束thread: RAII
5 : detach()使用時機範例 (開新檔案處理)
6 : Passing arguments to a thread function
7 : 
*/
#define test_idx 7
#if test_idx == 1

void do_something() {
  std::cout << "do_something()" << std::endl;
};

void do_something_else() {
  std::cout << "do_something_else()" << std::endl;
};

class background_task {
 public:
  void operator()() const {
    do_something();
    do_something_else();
  }
};

int main() {
  std::cout << "main() start" << std::endl;
  // 1
  // background_task f;
  // std::thread my_thread(f);
  // 2
  // std::thread my_thread(background_task());
  // 3 : using the new uniform initialization syntax
  std::thread my_thread{background_task()};
  // 4 : Lambda
  // std::thread my_thread([] {
  //   do_something();
  //   do_something_else();
  // });

  std::this_thread::sleep_for(std::chrono::seconds(1));
  if (my_thread.joinable()) {
    std::cout << "joinable()" << std::endl;
    my_thread.join();
  }
  std::cout << "main() end" << std::endl;
  return 0;
}

#elif test_idx == 2
/**
 * oops()函式因為使用了detach()，所以在oops()執行完成時，
 * thread中的變數可能還在運行。如果thread還在運行，就會去
 * 調用do_something()就會訪問已經釋放的記憶體。
 * 
 * 持續執行func::operator()，可能會在do_something()中調用
 * some_local_state的引用，進而導致未定義行為。
**/
void do_something(int &i) {
  ++i;
  std::cout << i << std::endl;
}

struct func {
  int &i;  // 左值
  func(int &i_) : i(i_) {}
  void operator()() {
    for (int j = 0; j < 1000000; ++j)
      do_something(j);
  }
};

void oops() {
  int some_local_state = 0;
  func my_func(some_local_state);
  std::thread my_thread(my_func);
  my_thread.detach();
}

int main() {
  std::cout << "main() start" << std::endl;
  oops();
  std::this_thread::sleep_for(std::chrono::seconds(20));
  std::cout << "main() end" << std::endl;
  return 0;
}
#elif test_idx == 3
void do_something(int &i) {
  ++i;
  std::cout << i << std::endl;
}

void do_something_in_current_thread() {
  //...
}

struct func {
  int &i;
  func(int &i_) : i(i_) {}
  void operator()() {
    for (int j = 0; j < 1000; ++j)
      do_something(j);
  }
};

void f() {
  int some_local_state = 0;
  func my_func(some_local_state);
  std::thread t(my_func);
  try {
    do_something_in_current_thread();
  } catch (...) {
    t.join();
    std::cout << "throw" << std::endl;
    throw;
  }
  t.join();
}

int main() {
  f();
  return 0;
}

#elif test_idx == 4
/**
 * 使用RAII等待線程完成
**/
class thread_guard {
  std::thread &t;

 public:
  // explicit 表示這個構建方式需要明確宣告
  explicit thread_guard(std::thread &t_) : t(t_) {}
  // 解構子 使thread結束
  ~thread_guard() {
    if (t.joinable()) {  // 1
      t.join();          // 2
    }
  }
  // 刪除建構子，防止建構子被賦值
  thread_guard(thread_guard const &) = delete;  //3
  // 防止建構子被copy
  thread_guard &operator=(thread_guard const &) = delete;
};

void do_something(int &i) {
  ++i;
  std::cout << i << std::endl;
}

void do_something_in_current_thread() {
  //...
}

struct func {
  int &i;
  func(int &i_) : i(i_) {}
  void operator()() {
    for (int j = 0; j < 1000; ++j)
      do_something(j);
  }
};

void f() {
  int some_local_state = 0;
  func my_func(some_local_state);
  std::thread t(my_func);
  thread_guard g(t);
  do_something_in_current_thread();
}  // 4

int main() {
  std::cout << "main() start" << std::endl;
  f();
  std::cout << "main() end" << std::endl;
  return 0;
}
#elif test_idx == 5
void open_document_and_display_gui(const std::string& s) {
}
bool done_editing() {
  return true;
}
enum command_type {
  open_new_document
};
struct user_command {
  command_type type;
  user_command() : type(open_new_document) {}
};
user_command get_user_input() {
  return user_command();
}
std::string get_filename_from_user() {
  return std::string("user_document");
}
void process_user_input(user_command cmd_) {
}
void edit_document(std::string const& filename) {
  std::cout << "edit_document() start" << std::endl;
  open_document_and_display_gui(filename);
  while (!done_editing()) {
    user_command cmd = get_user_input();
    if (cmd.type == open_new_document) {
      std::string const new_name = get_filename_from_user();
      // std::thread也可以傳遞參數
      std::thread t(edit_document, new_name);  // 1
      t.detach();                              // 2
    } else {
      process_user_input(cmd);
    }
  }
  std::cout << "edit_document() end" << std::endl;
}
int main() {
  std::cout << "main() start" << std::endl;
  edit_document("my_document");
  edit_document("my_document");
  std::cout << "main() end" << std::endl;
  return 0;
}
#elif test_idx == 6
// std::thread 傳參數
/*
void f(int i, std::string const &s) {
  std::cout << "int : " << i << std::endl;
  std::cout << "string : " << s << std::endl;
};
void not_oops(int some_param) {
  char buffer[1024];
  // sprintf int to string
  sprintf_s(buffer, "%i", some_param);
  // 無法保證隱式轉換與std::thread建構子的順序，這是有機會造成未定義行為
  // std::thread t(f, 3, buffer);
  std::thread t(f, 3, std::string(buffer));
  t.detach();
}
*/
// std::thread 傳參考
/*
void update_data_for_widget(widget_id w, widget_data& data);  // 1
void oops_again(widget_id w) {
  widget_data data;
  // 右值
  std::thread t(update_data_for_widget, w, data);  // 2
  // std::ref
  // std::thread t(update_data_for_widget, w, std::ref(data));
  // &
  // std::thread t(update_data_for_widget,w, &data));
  display_status();
  t.join();
  process_widget_data(data);
}
*/
// std::thread傳遞第一個成員可以是某class下的function，使此線程只會調用這個class的某個function
/*
class X {
 public:
  void do_lengthy_work();
};
X my_x;
std::thread t(&X::do_lengthy_work, &my_x);  // 1
*/
// std::thread第三個參數是do_lengthy_work()所需的引數
class X {
 public:
  void do_lengthy_work(int);
};
X my_x;
int num(0);
std::thread t(&X::do_lengthy_work, &my_x, num);
#elif test_idx == 7

// scoped_thread
/*
// 直接在建構子中檢查thread是否可以匯入
void do_something(int &i) {
  ++i;
  std::cout << i << std::endl;
}

void do_something_in_current_thread() {
  //...
}
struct func {
  int &i;
  func(int &i_) : i(i_) {}
  void operator()() {
    for (int j = 0; j < 1000; ++j)
      do_something(j);
  }
};

class scoped_thread {
  std::thread t;
 public:
  explicit scoped_thread(std::thread t_) :  // 1
                                           t(std::move(t_)) {
    if (!t.joinable())  // 2
      throw std::logic_error("No thread");
  }
  ~scoped_thread() {
    t.join();  // 3
  }
  scoped_thread(scoped_thread const &) = delete;
  scoped_thread &operator=(scoped_thread const &) = delete;
};

void f() {
  int some_local_state;
  scoped_thread t(std::thread(func(some_local_state)));  // 4
  do_something_in_current_thread();
}  // 5
int main() {
  std::cout << "main() start" << std::endl;
  f();
  std::cout << "main() end" << std::endl;
  return 0;
}
*/

// joining_thread
// class joining_thread{
//   std::thread t;
//   public:
//     joining_thread() noexcept=default;
//     template<typename Callable, typename ... Args>
//     explicit joining_thread
// }

// class string {
//   char* data;

//  public:
//   // copy constructor
//   string(const char* p) {
//     size_t size = std::strlen(p) + 1;
//     data = new char[size];
//     std::memcpy(data, p, size);
//   }
//   // move constructor
//   string(string&& rhs) {
//     data = rhs.data;
//     rhs.data = nullptr;
//   }
//   ~string() {
//     delete[] data;
//   }
// };

// struct FishData {
//   std::string str = "aaa";
//   // Much heavy data structure
// };

// struct Fish {
//   Fish(const FishData& a) {
//     // puts("fish copy conversion ctor");
//     std::cout << a.str << std::endl;
//   }
//   Fish(FishData&& a) {
//     // puts("fish move conversion ctor");
//     std::cout << a.str << std::endl;
//   }
// };

// template <typename T>
// Fish MakeFish(T&& fd) {
//   return Fish{fd};
// }

// int main() {
//   FishData fd;
//   FishData fd2;
//   std::cout << fd.str << std::endl;
//   // FishData(fd);              // #1: l-value
//   FishData(FishData{});      // #2: r-value
//   FishData(std::move(fd2));  // #3: r-value

//   // FishData fd3{fd};
//   FishData fd4(fd);
// }
// https://blog.csdn.net/wangshubo1989/article/details/50485951
struct A {
  A(int&& n) { std::cout << "rvalue overload, n=" << n << "\n"; }
  A(int& n) { std::cout << "lvalue overload, n=" << n << "\n"; }
};

class B {
 public:
  template <class T1, class T2, class T3>
  B(T1&& t1, T2&& t2, T3&& t3) : a1_{std::forward<T1>(t1)},
                                 a2_{std::forward<T2>(t2)},
                                 a3_{std::forward<T3>(t3)} {
  }

 private:
  A a1_, a2_, a3_;
};

template <class T, class U>
std::unique_ptr<T> make_unique1()

#endif