要在 CMake 中拉起基于 **GoogleTest（gtest）** 和 **GoogleMock（gmock）** 的单元测试（UT），可以使用两种方式：

---

## ✅ 一、推荐方式：使用 CMake 的 `FetchContent` 自动拉取 GoogleTest

这是 Google 官方推荐方式，适用于跨平台构建，不依赖系统安装。

### 🔧 CMake 示例

```cmake
cmake_minimum_required(VERSION 3.14)
project(MyTestProject)

enable_testing()

# 1. 拉取 GTest/GMock 源码
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
# 或者用稳定版：
# URL https://github.com/google/googletest/archive/refs/tags/release-1.14.0.zip

# 2. 下载并添加子目录
FetchContent_MakeAvailable(googletest)

# 3. 添加你的源代码 & 测试代码
add_executable(my_test
    test/main.cpp
    test/my_component_test.cpp
    src/my_component.cpp
)

# 4. 链接 GTest/GMock
target_link_libraries(my_test
    gtest
    gmock
    gtest_main
    pthread  # Linux 必须
)

# 5. 注册测试
include(GoogleTest)
gtest_discover_tests(my_test)
```

---

## ✅ 二、替代方式：使用 `add_subdirectory` 引入本地 GTest 源码

适合你已经 clone 了 googletest 源码作为第三方依赖：

```bash
git clone https://github.com/google/googletest.git third_party/googletest
```

```cmake
add_subdirectory(third_party/googletest)

add_executable(my_test
    test/my_test.cpp
    src/my_code.cpp
)

target_link_libraries(my_test
    gtest
    gmock
    gtest_main
    pthread
)

include(GoogleTest)
gtest_discover_tests(my_test)
```

---

## 🧠 使用建议与注意事项

### ✅ 最佳实践

* 使用 `gtest_main` 统一提供 `main()` 函数，简化测试程序结构。
* 使用 `gtest_discover_tests`，使得测试可以自动被 `ctest` 发现和执行。
* 单元测试建议放在 `test/` 目录下，生产代码放在 `src/` 目录下，分层清晰。

### ⚠️ 注意事项

* **不要**手动安装 GTest（如 `apt install libgtest-dev`），这样 CMake 中不便于移植和版本控制。
* CMake 版本要求：使用 `FetchContent` 要求 CMake >= 3.14。
* GTest 自身使用了 `pthread`，Linux 下必须链接它。
* 不要忘记 `enable_testing()`，否则 `ctest` 不起作用。
* 在测试中 mock 类时，记得在测试代码中 `#include <gmock/gmock.h>`。

---

## 📌 构建与执行命令

```bash
mkdir build && cd build
cmake ..
cmake --build .
ctest    # 或 ./my_test
```

这是一个 **非常关键的问题**：UT 不只是“写测试”，而是推动你**重构代码架构以便可测试性**的过程。我们分两个层次来讲：

---

## ✅ 一、**被测试代码需要全部编进来吗？**

### **1. 通常情况下：是的，要编进来**

为了测试函数/类的实现，**被测试代码需要参与链接**，否则测试无法调用它们。

常见做法如下：

```cmake
# src 中的生产代码做成库
add_library(my_component STATIC
    src/my_component.cpp
    src/helper.cpp
)

# 测试用例链接这个库
add_executable(my_test
    test/my_component_test.cpp
)
target_link_libraries(my_test
    my_component
    gtest
    gmock
    gtest_main
    pthread
)
```

### **2. 例外情况**

* 如果你只测试头文件中的模板代码，可能不需要链接 cpp。
* 如果你 mock 了全部依赖（用接口 + mock），可以只测试你关心的行为模块。

---

## ✅ 二、**如何优化原有代码架构以便于测试？**

这部分非常重要，UT 质量好坏的核心是你的架构设计 **是否“可测试”**。

---

### 🔧 1. **将实现与接口解耦**（抽象依赖）

引入抽象层：把依赖注入为接口，使其可以被 mock。

```cpp
// 非推荐写法：直接创建真实对象
class OrderProcessor {
    DB db;
public:
    void process() { db.write(); }
};

// 推荐写法：依赖接口
class IDB {
public:
    virtual void write() = 0;
    virtual ~IDB() = default;
};

class OrderProcessor {
    IDB* db_;
public:
    OrderProcessor(IDB* db) : db_(db) {}
    void process() { db_->write(); }
};
```

测试时：

```cpp
class MockDB : public IDB {
public:
    MOCK_METHOD(void, write, (), (override));
};

TEST(OrderProcessorTest, CallsWrite) {
    MockDB mock_db;
    EXPECT_CALL(mock_db, write());
    OrderProcessor op(&mock_db);
    op.process();
}
```

---

### 🔧 2. **使用依赖注入（DI）替代“全局单例”**

**坏例子：**

```cpp
Logger::instance().log("error");
```

**好例子：**

```cpp
class ILog {
    virtual void log(const std::string&) = 0;
};

class Worker {
    ILog* logger_;
public:
    Worker(ILog* log) : logger_(log) {}
    void run() { logger_->log("running"); }
};
```

你可以在生产中注入真实 logger，在测试中注入 mock。

---

### 🔧 3. **避免静态函数 / 全局状态 / 非确定行为**

这些特性会造成 **测试困难、行为不稳定、不可 mock**，建议：

* 将静态函数封装成可替换的策略类。
* 用构造函数注入时间/随机数等“非确定”模块。
* 避免直接访问环境变量、文件系统等副作用模块。

---

### 🔧 4. **使用接口/类对边界进行隔离**

比如：

* 对网络通信用 `INetworkClient` 抽象；
* 对系统 API 抽象成 `ITimeProvider`、`IFileReader`；
* 测试中 mock 它们，避免不确定性。

---

## 📦 小总结：UT 驱动架构优化

| 优化目标   | 方式                    |
| ------ | --------------------- |
| 易 mock | 提取接口、依赖注入             |
| 可控性    | 移除单例/静态函数，全 mock 外部依赖 |
| 易组织    | 将模块做成静态库，便于复用         |
| 测试覆盖   | 保持逻辑清晰，小函数，高内聚低耦合     |

---

 **典型“难以测试”的代码 -> 如何重构以适配 GTest/Mock 的过程演示**，包括：

1. 糟糕设计
2. 抽象与解耦
3. Mock 编写
4. UT 结构与 CMake

我们用一个典型业务逻辑举例：**发送用户注册邮件**。这个过程通常涉及多种“不可测试”的外部依赖，如数据库、邮件服务等。

---

## ✅ 步骤一：**原始代码（难以测试）**

```cpp
// user_service.cpp
#include "user_service.h"
#include <iostream>

void UserService::registerUser(const std::string& email) {
    // 假设这写入了数据库
    std::cout << "Saving user to database...\n";
    // 然后发送邮件
    std::cout << "Sending welcome email to " << email << std::endl;
}
```

**问题：**

* 使用 `std::cout` 模拟数据库和邮件逻辑，无抽象；
* 无法 mock、无返回值；
* 写死流程顺序、逻辑不可测试。

---

## ✅ 步骤二：**提取接口，解耦依赖**

```cpp
// interfaces.h
#pragma once
#include <string>

class IUserRepo {
public:
    virtual void saveUser(const std::string& email) = 0;
    virtual ~IUserRepo() = default;
};

class IEmailSender {
public:
    virtual void sendWelcomeEmail(const std::string& email) = 0;
    virtual ~IEmailSender() = default;
};
```

---

## ✅ 步骤三：**重写逻辑为依赖注入方式**

```cpp
// user_service.h
#pragma once
#include "interfaces.h"

class UserService {
    IUserRepo* repo_;
    IEmailSender* mailer_;
public:
    UserService(IUserRepo* repo, IEmailSender* mailer)
        : repo_(repo), mailer_(mailer) {}

    void registerUser(const std::string& email) {
        repo_->saveUser(email);
        mailer_->sendWelcomeEmail(email);
    }
};
```

---

## ✅ 步骤四：**编写 UT 使用 GMock**

### 🔧 test/user\_service\_test.cpp

```cpp
#include "user_service.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::_;
using ::testing::Exactly;
using ::testing::InSequence;

class MockUserRepo : public IUserRepo {
public:
    MOCK_METHOD(void, saveUser, (const std::string&), (override));
};

class MockEmailSender : public IEmailSender {
public:
    MOCK_METHOD(void, sendWelcomeEmail, (const std::string&), (override));
};

TEST(UserServiceTest, RegisterUser_CallsSaveAndSendEmail) {
    MockUserRepo repo;
    MockEmailSender sender;

    std::string email = "user@example.com";

    {
        // 验证调用顺序
        InSequence seq;
        EXPECT_CALL(repo, saveUser(email)).Times(Exactly(1));
        EXPECT_CALL(sender, sendWelcomeEmail(email)).Times(Exactly(1));
    }

    UserService service(&repo, &sender);
    service.registerUser(email);
}
```

---

## ✅ 步骤五：**CMake 构建配置**

```cmake
cmake_minimum_required(VERSION 3.14)
project(UserServiceTest)

enable_testing()

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/release-1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

# 库定义
add_library(user_service
    src/user_service.cpp
)
target_include_directories(user_service PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 测试目标
add_executable(user_service_test
    test/user_service_test.cpp
)
target_include_directories(user_service_test PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(user_service_test
    user_service
    gtest gmock gtest_main pthread
)

include(GoogleTest)
gtest_discover_tests(user_service_test)
```

---

## ✅ 运行测试

```bash
mkdir build && cd build
cmake ..
cmake --build .
ctest        # 或者 ./user_service_test
```

---

## 📌 总结：重构可测试性的方法

| 原始设计      | 重构方式       | 收益              |
| --------- | ---------- | --------------- |
| 写死实现      | 提取接口（IXXX） | 可 mock          |
| 直接构造依赖对象  | 构造函数依赖注入   | 可替换/解耦          |
| 打印/实际发送行为 | mock + 断言  | 快速测试、稳定性        |
| 无返回值      | 可测性差       | 引入状态检查或 mock 验证 |

---
