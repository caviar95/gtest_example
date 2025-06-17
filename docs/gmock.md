如果你想**打桩一个“非虚成员函数”，且该类具有构造/析构函数**，那么你需要注意以下关键点：

---

## ✅ 问题背景再明确一下：

你遇到的是如下场景：

```cpp
class SomeClass {
public:
    SomeClass();
    ~SomeClass();

    int RealFunc(int x);  // ❌ 非虚，无法直接用 gmock 打桩
};
```

目标是：**在单元测试中替换 `RealFunc(int)` 的实现（打桩），同时保留构造/析构行为**。

---

## ✅ 可行方案：**封装虚接口 + 委托（推荐）**

你需要：

1. 定义一个接口类（包含需要打桩的函数为虚函数）；
2. 原类实现接口，委托真实逻辑；
3. 测试时用 mock 类实现接口并打桩。

---

## 🔨 示例：完整工程结构

### ✅ 1️⃣ 业务代码

#### `interface.h`

```cpp
#pragma once
class ISomeInterface {
public:
    virtual ~ISomeInterface() = default;
    virtual int RealFunc(int x) = 0;
};
```

#### `some_class.h`

```cpp
#pragma once
#include "interface.h"

class SomeClass : public ISomeInterface {
public:
    SomeClass();
    ~SomeClass();

    int RealFunc(int x) override;  // 虚函数 now
};
```

#### `some_class.cpp`

```cpp
#include "some_class.h"
#include <iostream>

SomeClass::SomeClass() {
    std::cout << "Constructed\n";
}

SomeClass::~SomeClass() {
    std::cout << "Destructed\n";
}

int SomeClass::RealFunc(int x) {
    return x * 2;
}
```

---

### ✅ 2️⃣ gmock 打桩类

#### `mock_some_class.h`

```cpp
#pragma once
#include "interface.h"
#include <gmock/gmock.h>

class MockSomeClass : public ISomeInterface {
public:
    MOCK_METHOD(int, RealFunc, (int), (override));
};
```

---

### ✅ 3️⃣ 被测业务代码

#### `logic.h`

```cpp
#pragma once
#include "interface.h"

class Logic {
public:
    Logic(ISomeInterface* obj) : obj_(obj) {}

    int Run(int x) {
        return obj_->RealFunc(x) + 1;
    }

private:
    ISomeInterface* obj_;
};
```

---

### ✅ 4️⃣ 单元测试代码

#### `test.cpp`

```cpp
#include <gtest/gtest.h>
#include "mock_some_class.h"
#include "logic.h"

TEST(LogicTest, MockRealFuncWorks) {
    MockSomeClass mock;

    EXPECT_CALL(mock, RealFunc(10))
        .Times(1)
        .WillOnce(::testing::Return(100));

    Logic logic(&mock);
    EXPECT_EQ(logic.Run(10), 101);
}
```

---

### ✅ 5️⃣ CMakeLists.txt 示例

```cmake
cmake_minimum_required(VERSION 3.10)
project(GMockTest)

enable_testing()

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(test
    test.cpp
    some_class.cpp
)

target_link_libraries(test
    GTest::gtest
    GTest::gmock
    GTest::gtest_main
    pthread
)

add_test(NAME MockTest COMMAND test)
```

---

## 🧠 总结：类具有构造/析构函数 + 成员函数打桩 → 推荐方法

| 方法                    | 构造/析构支持 | 成员函数打桩 | 推荐   |
| --------------------- | ------- | ------ | ---- |
| ✅ 虚接口封装 + Mock类       | ✅       | ✅      | ⭐⭐⭐⭐ |
| 链接替换法（Link-time hack） | ❌ 不安全   | ✅ 但危险  | ⭐    |
| 内联宏替换                 | ❌ 不支持   | ✅（不稳定） | ⭐    |

非常好的问题！你问的是：

> ✅「`SomeClass` 是原本的实现类，怎么在测试中用 `MockSomeClass` 替代它？」

---

## ✅ 回答核心：

我们不是让 `SomeClass` 的对象**调用** `MockSomeClass`，而是通过**面向接口编程（基于虚函数）**，让业务逻辑 `Logic` 类只依赖接口 `ISomeInterface`，从而在运行时注入 `MockSomeClass` 实例来实现打桩。

---

## 👇 举个类比说明：

```cpp
class ISomeInterface {
public:
    virtual int RealFunc(int x) = 0;
};

class SomeClass : public ISomeInterface {
public:
    int RealFunc(int x) override {
        return x * 2;
    }
};

class MockSomeClass : public ISomeInterface {
public:
    MOCK_METHOD(int, RealFunc, (int), (override));
};

class Logic {
public:
    Logic(ISomeInterface* obj) : obj_(obj) {}

    int Run(int x) {
        return obj_->RealFunc(x);  // 只调用接口方法
    }

private:
    ISomeInterface* obj_;
};
```

---

### ➤ 在生产环境中：

```cpp
SomeClass real;
Logic logic(&real);
logic.Run(10);  // 调用的是 real.RealFunc(10)
```

---

### ➤ 在测试环境中：

```cpp
MockSomeClass mock;
EXPECT_CALL(mock, RealFunc(10)).WillOnce(Return(100));
Logic logic(&mock);
logic.Run(10);  // 调用的是 mock.RealFunc(10)
```

---

## ✅ 关键点总结

| 项          | 说明                                                      |
| ---------- | ------------------------------------------------------- |
| 🧩 接口抽象    | `Logic` 类依赖的是抽象接口 `ISomeInterface`，而不是具体实现 `SomeClass`。 |
| 🔄 运行时替换   | 你可以在测试中注入 `MockSomeClass` 实例，生产中注入 `SomeClass`。         |
| ✅ 不需要转换    | 不需要把 `SomeClass` 强行转换为 `MockSomeClass`，而是使用接口指针实现替换。    |
| 🧪 Mock 实现 | `MockSomeClass` 是专门为测试构建的 mock 类，支持 `EXPECT_CALL()` 打桩。 |

---

### ✅ Diagram 简图理解：

```
           +----------------+
           | ISomeInterface |
           +----------------+
              ^           ^
              |           |
     +----------------+  +----------------+
     |   SomeClass    |  | MockSomeClass  |
     | Real implement |  | MOCK_METHOD... |
     +----------------+  +----------------+
              ^                ^
              |                |
              +----------------+
                       |
                 +------------+
                 |   Logic    |  ---> 调用 ISomeInterface::RealFunc()
                 +------------+
```

---

## ✅ 最终一句话总结：

> 测试时替代 `SomeClass` 的不是让它“调用 mock”，而是将整个 `SomeClass` 替换为实现同一接口的 `MockSomeClass`，这就是**依赖注入 + 接口多态**的威力。

如果你希望的是 “在不改 `Logic` 类结构的情况下 mock `SomeClass` 的成员函数”，我们可以探讨其它更 hack 的方式，比如链接时替换函数或 hook，但在可维护性和测试设计角度，**依赖接口注入 + mock** 是最佳实践。

非常好的问题，你提到的关键点是：

> 🔍 **SomeClass 是一个具体类，它实现了 `ISomeInterface`，那为什么测试时能传入 `MockSomeClass` 而不是 `SomeClass` 的对象？**

这背后的机制就是 **多态与依赖注入**。

---

## 🧩 回顾关键结构

我们设计时有这样的类关系：

```
     ISomeInterface           <-- 抽象接口
     /          \
SomeClass     MockSomeClass   <-- 实现了相同接口
```

---

## 🧠 核心设计思想：**面向接口编程（而非具体类）**

### 正确的设计方式是：

```cpp
class Logic {
public:
    Logic(ISomeInterface* obj) : obj_(obj) {}  // 接口指针，而不是具体类型

    int Run(int x) {
        return obj_->RealFunc(x);  // 通过接口调用，实现解耦
    }

private:
    ISomeInterface* obj_;
};
```

### ✅ 好处是：

* 你可以在正式运行时传入 `SomeClass`（业务实现）
* 在测试时传入 `MockSomeClass`（测试桩）
* 两者都实现了同一个接口 `ISomeInterface`

---

## ✅ 测试代码解读（你提到的）：

```cpp
MockSomeClass mock;
EXPECT_CALL(mock, RealFunc(10)).WillOnce(Return(100));

Logic logic(&mock);      // mock 被当作 ISomeInterface 使用
EXPECT_EQ(logic.Run(10), 101);  // 实际调用 mock.RealFunc(10)
```

这相当于：

* 用 `MockSomeClass` 替代了原本应该是 `SomeClass` 的真实对象。
* `Logic` 并不在乎你传入的是哪个实现，只要是 `ISomeInterface*` 就能用。

---

## 💡 用图来理解依赖关系：

```
[Logic]
   |
   | 接收 ISomeInterface*（多态）
   |
-----------------------------
|           生产环境         |   测试环境
|     new SomeClass();      |   new MockSomeClass();
|                           |
-----------------------------
```

---

## 🧩 如果你问的是：“如何让原始 SomeClass 对象使用 mock 行为？”

答案是：

> \*\*不能直接做到！\*\*因为 `SomeClass` 是一个具体类、成员函数是非虚的，它调用的是自己编译时绑定的函数实现。
> Mock 的机制是通过虚函数和多态实现的，只有通过接口抽象、间接调用才能生效。

---

## ✅ 结论

| 问题点                            | 解释                                       |
| ------------------------------ | ---------------------------------------- |
| 为啥能传 `MockSomeClass` 给 `Logic` | 因为 `Logic` 接受的是 `ISomeInterface*`，它支持多态  |
| 为啥能触发 mock 行为                  | 因为调用的是虚函数 `RealFunc()`，通过 gmock 接管了该函数实现 |
| 能不能让 `SomeClass` 本身调用 mock     | ❌ 不行，除非改成通过接口或把它变成虚函数                    |

在 `gmock` 中，**只能直接 Mock “虚函数”**。这意味着：

> ❌ 你不能直接用 `MOCK_METHOD` 模拟 **非虚成员函数**（普通成员函数、静态函数、构造函数、final 函数等）。

---

### ✅ 但如果你确实想“打桩”非虚函数（即：在测试中替换真实实现），有几种常见方案：

---

## 🧩 方法一：**封装重构法（间接打桩）** ✅

### ➤ 适用场景：你可以控制/修改被测代码结构。

通过将非虚函数封装到一个虚接口中，再用 gmock 模拟接口。

---

### 🔨 示例：目标类中存在一个不可 Mock 的非虚函数

#### 1️⃣ 原始类（不能直接 mock）：

```cpp
class FileSystem {
public:
    bool Exists(const std::string& path) {
        // 实际是个非虚函数，无法直接 mock
        std::ifstream f(path);
        return f.good();
    }
};
```

#### 2️⃣ 封装为接口类：

```cpp
class IFileSystem {
public:
    virtual ~IFileSystem() = default;
    virtual bool Exists(const std::string& path) = 0;
};

class RealFileSystem : public IFileSystem {
public:
    bool Exists(const std::string& path) override {
        std::ifstream f(path);
        return f.good();
    }
};
```

#### 3️⃣ gmock 打桩类：

```cpp
#include <gmock/gmock.h>

class MockFileSystem : public IFileSystem {
public:
    MOCK_METHOD(bool, Exists, (const std::string& path), (override));
};
```

#### 4️⃣ 被测代码依赖于接口：

```cpp
class FileManager {
public:
    FileManager(IFileSystem* fs) : fs_(fs) {}

    bool CheckFile(const std::string& path) {
        return fs_->Exists(path);
    }

private:
    IFileSystem* fs_;
};
```

#### 5️⃣ 测试代码（打桩替换非虚函数）：

```cpp
#include <gtest/gtest.h>

TEST(FileManagerTest, FileExistsReturnsTrue) {
    MockFileSystem mock_fs;
    EXPECT_CALL(mock_fs, Exists("file.txt")).WillOnce(::testing::Return(true));

    FileManager mgr(&mock_fs);
    EXPECT_TRUE(mgr.CheckFile("file.txt"));
}
```

---

## 🧩 方法二：**Link-time 替换法（强制打桩）** ⚠️

### ➤ 适用场景：不能改代码结构，但能控制链接行为。

例如你想在测试时“替换”非虚函数，可用链接替换法（如下所示）。

### ✅ 思路：

1. 用相同签名在测试文件中定义一个替代函数。
2. 链接时优先链接测试版本，替代原函数。

### 🧨 示例：替代非虚全局函数或类外定义函数

```cpp
// 原代码（foo.cpp）
int ReadDisk() {
    return 42;
}

// 被测函数
int Process() {
    return ReadDisk() + 1;
}
```

```cpp
// 测试代码（test.cpp）
int ReadDisk() {
    return 100;  // mock 替代逻辑
}

TEST(MyTest, ReplaceReadDisk) {
    EXPECT_EQ(Process(), 101);
}
```

> ⚠️ 注意：这种方式对类的“成员函数”适用性较差，需确保该函数未被 `inline`，可链接替换。

---

## 🧩 方法三：**宏或编译器注入重定义（侵入式）** ⚠️

对部分非虚函数可以用 `#define` 或 `LD_PRELOAD` 方式注入重定义。由于可维护性差，不推荐。

---

## ✅ 总结：哪种方法适合你？

| 方案    | 可行性   | 推荐程度 | 说明                |
| ----- | ----- | ---- | ----------------- |
| 封装重构法 | ✅ 高   | ⭐⭐⭐⭐ | 推荐，设计友好，适合长期维护    |
| 链接替代法 | ⚠️ 有限 | ⭐⭐   | 适合快速 mock，结构不可控场景 |
| 宏注入法  | ❌ 危险  | ⭐    | 不推荐，侵入性强，难以维护     |

---
你提到的需求是：

> 在 gtest 中，**打桩另一个模块的接口（例如网络库/定时器等）**，当被测代码调用这个接口时，**模拟在一段时间后自动调用回调函数**，来测试异步行为。

这是一个典型的**异步依赖打桩场景**，可以使用 **gmock 的 `Invoke` + `std::thread` + `std::this_thread::sleep_for`** 实现“延迟回调”的效果。

---

## ✅ 场景示例：打桩异步回调模块

假设你的系统依赖某个模块，如：

```cpp
// async_module.h
#pragma once
#include <functional>

class AsyncModule {
public:
    virtual ~AsyncModule() = default;

    // 异步接口：异步处理后会调用 cb 回调
    virtual void DoSomethingAsync(std::function<void(int)> cb) = 0;
};
```

被测类如下：

```cpp
// worker.h
#pragma once
#include "async_module.h"

class Worker {
public:
    Worker(AsyncModule* async) : async_(async) {}

    void StartWork() {
        async_->DoSomethingAsync([this](int result) {
            this->OnResult(result);
        });
    }

    int GetResult() const { return result_; }

private:
    void OnResult(int val) { result_ = val; }

    AsyncModule* async_;
    int result_ = 0;
};
```

---

## ✅ 测试中模拟“延迟触发回调”

```cpp
// mock_async.h
#pragma once
#include "async_module.h"
#include <gmock/gmock.h>

class MockAsyncModule : public AsyncModule {
public:
    MOCK_METHOD(void, DoSomethingAsync, (std::function<void(int)> cb), (override));
};
```

### ✅ 测试用例：调用后延迟触发回调

```cpp
#include <gtest/gtest.h>
#include <thread>
#include "worker.h"
#include "mock_async.h"

TEST(WorkerTest, AsyncCallbackIsTriggeredLater) {
    MockAsyncModule mock;

    Worker worker(&mock);

    EXPECT_CALL(mock, DoSomethingAsync(testing::_))
        .WillOnce([](std::function<void(int)> cb) {
            // 模拟延迟 100ms 后异步回调
            std::thread([cb]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                cb(42);
            }).detach();
        });

    worker.StartWork();

    // 等待异步回调完成
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_EQ(worker.GetResult(), 42);
}
```

---

## ✅ 说明

| 技术点                              | 说明                            |
| -------------------------------- | ----------------------------- |
| `EXPECT_CALL(...).WillOnce(...)` | 自定义函数体来打桩行为                   |
| `std::thread + sleep_for`        | 模拟异步延迟触发回调                    |
| `detach()`                       | 避免阻塞主线程，可视为异步 fire-and-forget |

---

## ✅ 可选改进（推荐）

1. ✅ 用 **`std::promise + future`** 等待异步结果完成，避免盲目 `sleep`
2. ✅ 或使用 **条件变量** 等更可靠同步机制
3. ✅ 引入 **事件循环/调度器 mock**，对 timer 更精确控制

---

## 🔚 总结

你可以通过以下方式模拟 “异步模块在延迟后调用回调”：

| 技术                         | 用途        |
| -------------------------- | --------- |
| `gmock + WillOnce(Invoke)` | 替换真实行为    |
| `std::thread + sleep_for`  | 模拟异步等待    |
| `EXPECT_CALL` + 参数断言       | 检查回调参数    |
| `std::promise/future`      | 更准确等待异步结果 |

是的，**gtest 本身并不直接支持“函数级别的 stub 替换”**，但你**可以通过 C++ 的链接机制（Linker 技巧）或编译单位划分**来实现：

---

## ✅ 方法一：编写一个 **Stub 文件**，重新定义函数（适用于非虚函数、C 函数等）

### 💡 原理：

如果你在测试中 **提前提供了同名函数定义**，**链接器会优先用你的 stub 替代真实实现**（只要链接顺序正确，或者真实实现不参与链接）。

---

### 🔧 示例：被测代码调用内部函数

```cpp
// lib/foo.cpp （被测模块）
#include <iostream>

int InternalCompute(int x) {
    std::cout << "Real InternalCompute called\n";
    return x * 2;
}

int DoWork(int a) {
    return InternalCompute(a) + 1;
}
```

---

### ✅ 测试中 stub 内部函数

```cpp
// test/foo_stub.cpp
int InternalCompute(int x) {
    return 1000 + x;  // mock/stub implementation
}
```

---

### ✅ 测试代码

```cpp
// test/test_foo.cpp
#include <gtest/gtest.h>

int DoWork(int a); // 来自 lib/foo.cpp

TEST(FooTest, StubInternalCompute) {
    EXPECT_EQ(DoWork(1), 1001 + 1);
}
```

---

### ✅ CMakeLists 示例

```cmake
add_library(foo STATIC foo.cpp)
add_library(foo_stub STATIC foo_stub.cpp)

add_executable(test_foo test_foo.cpp)
target_link_libraries(test_foo PRIVATE foo_stub)  # stub 优先
target_link_libraries(test_foo PRIVATE gtest gtest_main)
```

> ⚠️ 注意：**不要链接真实的 `foo.cpp`（包含原始 `InternalCompute` 实现）**，否则链接时可能报“duplicate symbol”或使用真实逻辑。

---

## ✅ 方法二：使用 `-Wl,--wrap=symbol` （更底层方式）

这是 GCC/Clang 支持的链接器参数，用于“重定向函数调用”。

### 示例：

```sh
g++ main.o -Wl,--wrap=InternalCompute -o test
```

并实现：

```cpp
int __wrap_InternalCompute(int x) {
    return 999;  // stub 实现
}

int __real_InternalCompute(int x);  // 可选，调用真实逻辑
```

适合大项目中临时替换 C 风格函数（如 `malloc`, `send`, `fopen`）。

---

## ✅ 方法三：将内部函数提取为虚函数或注入依赖（推荐方式）

如果你经常要 mock 某个函数，考虑：

| 方法               | 优点       | 示例                                   |
| ---------------- | -------- | ------------------------------------ |
| 虚函数 + mock 类     | 支持 gmock | `virtual int Compute()`              |
| 接口注入             | 更灵活      | `ICompute* compute`                  |
| std::function 注入 | 最轻量      | `std::function<int(int)> internalFn` |

---

## ✅ 总结

| 方法              | 场景                   | 是否推荐        |
| --------------- | -------------------- | ----------- |
| stub .cpp 文件    | 测试 C 函数或内部函数逻辑       | ✅ 简单、有效     |
| `--wrap`        | 控制所有调用、底层依赖（如系统 API） | ⚠️ 对构建系统要求高 |
| 接口/虚函数替换        | 设计良好的 C++ 模块         | ✅ 最推荐       |
| gmock + Adapter | 无法 mock 的封闭类         | ✅           |

