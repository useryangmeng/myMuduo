#pragma once

//该类的拷贝构造函数，赋值构造函数被delete
//派生类的实例化时调用默认构造和析构
//这样操作之后，派生类对象可以进行正常的构造和析构，但是派生类对象无法进行拷贝构造和赋值操作
class noncopyable
{
public:
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;

protected:
    noncopyable() = default;
    ~noncopyable() = default;
private:

};