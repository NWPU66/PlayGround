# Google Test

## 基本概念

使用 GoogleTest 时，首先要编写断言，即检查条件是否为真的语句。 断言的结果可以是成功、非致命失败或致命失败。 如果发生致命失败，则会中止当前函数；否则程序会正常继续。

测试使用断言来验证被测代码的行为。 如果测试崩溃或断言失败，则测试失败；否则测试成功。

一个测试套件包含一个或多个测试。 应根据测试代码的结构，将测试组合成测试套件。 当测试套件中的多个测试需要共享共同对象和子程序时，可以将它们放入一个测试夹具类中。

一个测试程序可以包含多个测试套件。

接下来，我们将讲解如何编写测试程序，从单个断言级别开始，直至测试和测试套件。