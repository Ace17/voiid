#pragma once

#include <memory>

template<class>
struct Delegate;

template<typename T, typename... Args>
struct Delegate<T(Args...)>
{
  // invokes the delegate
  T operator () (Args... args) { return invokable->call(args...); }

  Delegate() = default;

  Delegate(T(*f)(Args...))
  {
    invokable = std::make_unique<StaticInvokable>(f);
  }

  Delegate(Delegate<T(Args...)>&& other)
  {
    invokable.reset(other.invokable.get());
    other.invokable.release();
  }

  void operator = (Delegate<T(Args...)>&& other)
  {
    invokable.reset(other.invokable.get());
    other.invokable.release();
  }

  void operator = (T (* f)(Args...))
  {
    invokable = std::make_unique<StaticInvokable>(f);
  }

  template<typename Lambda>
  Delegate(const Lambda& func)
  {
    invokable = std::make_unique<LambdaInvokable<Lambda>>(func);
  }

  template<typename Lambda>
  void operator = (const Lambda& func)
  {
    invokable = std::make_unique<LambdaInvokable<Lambda>>(func);
  }

  operator bool () const { return invokable.ptr; }

private:
  struct Invokable
  {
    virtual ~Invokable() = default;
    virtual T call(Args... args) = 0;
  };

  std::unique_ptr<Invokable> invokable;

  // concrete invokable types
  struct StaticInvokable : Invokable
  {
    StaticInvokable(T(*f)(Args...)) : funcPtr(f) {}
    T call(Args... args) override { return (*funcPtr)(args...); }
    T (* funcPtr)(Args...) = nullptr;
  };

  template<typename Lambda>
  struct LambdaInvokable : Invokable
  {
    LambdaInvokable(Lambda f) : funcPtr(f) {}
    T call(Args... args) override { return funcPtr(args...); }
    Lambda funcPtr {};
  };
};

