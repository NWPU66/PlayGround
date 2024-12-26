#!E:\Lua\5.1\lua.exe

-- print("Hello World!")
-- print("www.runoob.com")
print("--------------------------------------------------------------")
print(type("Hello world")) -- > string
print(type(10.4 * 3)) -- > number
print(type(print)) -- > function
print(type(type)) -- > function
print(type(true)) -- > boolean
print(type(nil)) -- > nil
print(type(type(X))) -- > string

print("--------------------------------------------------------------")
tab1 = {
    key1 = "val1",
    key2 = "val2",
    "val3"
}
for k, v in pairs(tab1) do
    print(k .. " - " .. v)
end

tab1.key1 = nil
for k, v in pairs(tab1) do
    print(k .. " - " .. v)
end

print("--------------------------------------------------------------")
print(type(X) == nil)
print(type(X) == "nil")

if 0 then
    print("数字 0 是 true")
else
    print("数字 0 为 false")
end

print("--------------------------------------------------------------")
string1 = "this is string1"
string2 = 'this is string2'
html = [[
<html>
<head></head>
<body>
    <a href="http://www.runoob.com/">菜鸟教程</a>
</body>
</html>
]]
print(html)

-- print("error" + 1)

print("a" .. 'b')
print(157 .. 428)
print(#"www.runoob.com")

print("--------------------------------------------------------------")
-- 创建一个空的 table
local tbl1 = {}
-- 直接初始表
local tbl2 = {"apple", "pear", "orange", "grape"}
print(tbl2[1])

-- table_test.lua 脚本文件
a = {}
a["key"] = "value"
key = 10
a[key] = 22
a[key] = a[key] + 11
for k, v in pairs(a) do
    print(k .. " : " .. v)
end

-- 不同于其他语言的数组把 0 作为数组的初始索引，在 Lua 里表的默认初始索引一般以 1 开始。

-- table_test2.lua 脚本文件
local tbl = {"apple", "pear", "orange", "grape"}
for key, val in pairs(tbl) do
    print("Key", key)
end

-- table_test3.lua 脚本文件
a3 = {}
for i = 1, 10 do
    a3[i] = i
end
a3["key"] = "val"
print(a3["key"])
print(a3["none"])

print("--------------------------------------------------------------")
-- function_test.lua 脚本文件
function factorial1(n)
    if n == 0 then
        return 1
    else
        return n * factorial1(n - 1)
    end
end
print(factorial1(5))
factorial2 = factorial1
print(factorial2(5))

-- 匿名函数（anonymous function）
-- function_test2.lua 脚本文件
function testFun(tab, fun)
    for k, v in pairs(tab) do
        print(fun(k, v));
    end
end

tab = {
    key1 = "val1",
    key2 = "val2"
};
testFun(tab, function(key, val) -- 匿名函数
    return key .. "=" .. val;
end);

print("--------------------------------------------------------------")
--[[
在 Lua 里，最主要的线程是协同程序（coroutine）。它跟线程（thread）差不多，
拥有自己独立的栈、局部变量和指令指针，可以跟其他协同程序共享全局变量和其他大部分东西。

线程跟协程的区别：线程可以同时多个运行，而协程任意时刻只能运行一个，
并且处于运行状态的协程只有被挂起（suspend）时才会暂停。
]]

print("--------------------------------------------------------------")
--[[
userdata 是一种用户自定义数据，用于表示一种由应用程序或 C/C++ 语言库所创建的类型，
可以将任意 C/C++ 的任意数据类型的数据（通常是 struct 和 指针）存储到 Lua 变量中调用。
]]

print("--------------------------------------------------------------")
-- 遇到赋值语句Lua会先计算右边所有的值然后再执行赋值操作，所以我们可以这样进行交换变量的值
-- x, y = y, x                     -- swap 'x' for 'y'
-- a[i], a[j] = a[j], a[i]         -- swap 'a[i]' for 'a[j]'

-- 要注意的是Lua中 0 为 true

print("--------------------------------------------------------------")
function max(num1, num2)
    if (num1 > num2) then
        result = num1;
    else
        result = num2;
    end

    return result;
end
print("两值比较最大值为 ", max(10, 4))
print("两值比较最大值为 ", max(5, 6))

myprint = function(param)
    print("这是打印函数 -   ##", param, "##")
end

function add(num1, num2, functionPrint)
    result = num1 + num2
    -- 调用传递的函数参数
    functionPrint(result)
end

myprint(10)
-- myprint 函数作为参数传递
add(2, 5, myprint)

s, e, captured = string.find("www.runoob.com", "runoob")
print(s, e, captured)

function maximum(a)
    local mi = 1 -- 最大值索引
    local m = a[mi] -- 最大值
    for i, val in ipairs(a) do
        if val > m then
            mi = i
            m = val
        end
    end
    return m, mi
end

print(maximum({8, 10, 23, 12, 5}))

print("--------------------------------------------------------------")
-- 可变参数
function add(...)
    local s = 0
    for i, v in ipairs {...} do -- > {...} 表示一个由所有变长参数构成的数组  
        s = s + v
    end
    return s
end
print(add(3, 4, 5, 6, 7)) --->25

function average(...)
    result = 0
    local arg = {...} -- > arg 为一个表，局部变量
    for i, v in ipairs(arg) do
        result = result + v
    end
    print("总共传入 " .. #arg .. " 个数")
    return result / #arg
end
print("平均值为", average(10, 5, 3, 4, 5, 6))
-- 我们也可以通过 select("#",...) 来获取可变参数的数量

function fwrite(fmt, ...) ---> 固定的参数fmt
    return io.write(string.format(fmt, ...))
end
fwrite("runoob\n") --->fmt = "runoob", 没有变长参数。  
fwrite("%d%d\n", 1, 2) --->fmt = "%d%d", 变长参数为 1 和 2

function f(...)
    a = select(3, ...) -- >从第三个位置开始，变量 a 对应右边变量列表的第一个参数
    print(a)
    print(select(3, ...)) -- >打印所有列表参数
end
f(0, 1, 2, 3, 4, 5)

do
    function foo(...)
        for i = 1, select('#', ...) do -- >获取参数总数
            local arg = select(i, ...); -- >读取参数，arg 对应的是右边变量列表的第一个参数
            print("arg", arg);
        end
    end

    foo(1, 2, 3, 4);
end

print("--------------------------------------------------------------")
print(5 / 2)

local myString = "Hello, 世界!"
-- -- 计算字符串的长度（字符个数）
-- local length1 = utf8.len(myString)
-- print(length1) -- 输出 10
-- string.len 函数会导致结果不准确
local length2 = string.len(myString)
print(length2) -- 输出 14

print(string.format("%6.3s", "abc"))

s = "Deadline is 30/05/1999, firm"
date = "%d%d/%d%d/%d%d%d%d"
print(string.sub(s, string.find(s, date))) -- > 30/05/1999

print("--------------------------------------------------------------")
-- 创建一个数组
local myArray = {10, 20, 30, 40, 50}

-- 删除第三个元素
-- table.remove(myArray, 3)
myArray[3] = nil

-- 循环遍历数组
for i = 1, #myArray do
    print(myArray[i])
end

for k, v in ipairs(myArray) do
    print(k, v)
end

function square(iteratorMaxCount, currentNumber)
    if currentNumber < iteratorMaxCount then
        currentNumber = currentNumber + 1
        return currentNumber, currentNumber * currentNumber
    end
end
for i, n in square, 3, 0 do
    print(i, n)
end

print("--------------------------------------------------------------")
array = {"Google", "Runoob"}
function elementIterator(collection)
    local index = 0
    local count = #collection
    return function()
        index = index + 1
        if index <= count then
            --  返回迭代器的当前元素
            return collection[index]
        end
    end
end
for element in elementIterator(array) do
    print(element)
end

print("--------------------------------------------------------------")
-- 当我们获取 table 的长度的时候无论是使用 # 还是 table.getn 其都会在索引中断的地方停止计数，
-- 而导致无法正确取得 table 的长度。
-- 可以使用以下方法来代替：
function table_leng(t)
    local leng = 0
    for k, v in pairs(t) do
        leng = leng + 1
    end
    return leng;
end

print("--------------------------------------------------------------")
local m = require "helloLuaMoudle"
print(m.constant)
m.func3()
print(package.path)
package.path = package.path .. ";/home/runner/LuaTest/?.lua"
print(package.path)

print("--------------------------------------------------------------")
other = {
    foo = 3
}
t = setmetatable({}, {
    __index = other
})
print(t.foo)

mymetatable = {}
mytable = setmetatable({
    key1 = "value1"
}, {
    __newindex = mymetatable
})

print(mytable.key1)
mytable.newkey = "新值2"
print(mytable.newkey, mymetatable.newkey)
mytable.key1 = "新值1"
print(mytable.key1, mymetatable.key1)

print("--------------------------------------------------------------")
function foo()
    print("协同程序 foo 开始执行")
    local value = coroutine.yield("暂停 foo 的执行")
    print("协同程序 foo 恢复执行，传入的值为: " .. tostring(value))
    print("协同程序 foo 结束执行")
end

-- 创建协同程序
local co = coroutine.create(foo)

-- 启动协同程序
local status, result = coroutine.resume(co)
print(result) -- 输出: 暂停 foo 的执行

-- 恢复协同程序的执行，并传入一个值
status, result = coroutine.resume(co, 42)
-- 输出: 协同程序 foo 恢复执行，传入的值为: 42

print("--------------------------------------------------------------")
-- local newProductor

-- function productor()
--     local i = 0
--     while true do
--         i = i + 1
--         send(i) -- 将生产的物品发送给消费者
--     end
-- end

-- function consumer()
--     while true do
--         local i = receive() -- 从生产者那里得到物品
--         print(i)
--     end
-- end

-- function receive()
--     local status, value = coroutine.resume(newProductor)
--     return value
-- end

-- function send(x)
--     coroutine.yield(x) -- x表示需要发送的值，值返回以后，就挂起该协同程序
-- end

-- -- 启动程序
-- newProductor = coroutine.create(productor)
-- consumer()

print("--------------------------------------------------------------")
-- debug.debug()

function myfunction()
    print(debug.traceback("Stack trace"))
    print(debug.getinfo(1))
    print("Stack trace end")
    return 10
end
myfunction()
print(debug.getinfo(1))

print("--------------------------------------------------------------")
-- 元类
Rectangle = {
    area = 0,
    length = 0,
    breadth = 0
}

-- 派生类的方法 new
function Rectangle:new(o, length, breadth)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    self.length = length or 0
    self.breadth = breadth or 0
    self.area = length * breadth;
    return o
end

-- 派生类的方法 printArea
function Rectangle:printArea()
    print("矩形面积为 ", self.area)
end

-- 创建对象
myshape = Rectangle:new(nil, 10, 20)

myshape:printArea()

print("--------------------------------------------------------------")
