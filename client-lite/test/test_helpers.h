#pragma once

class TestHelpers
{
public:
    static void DisableNetwork();
    static void EnableNetwork();

private:
    // Disallow creating an instance of this object
    TestHelpers() {}
};

template <typename TLambda>
class test_lambda_call
{
public:
    test_lambda_call(const test_lambda_call&) = delete;
    test_lambda_call& operator=(const test_lambda_call&) = delete;
    test_lambda_call& operator=(test_lambda_call&& other) = delete;

    explicit test_lambda_call(TLambda&& lambda) noexcept : _lambda(std::move(lambda))
    {
        static_assert(std::is_same<decltype(lambda()), void>::value, "scope_exit lambdas must not have a return value");
        static_assert(!std::is_lvalue_reference<TLambda>::value && !std::is_rvalue_reference<TLambda>::value,
            "scope_exit should only be directly used with a lambda");
    }

    test_lambda_call(test_lambda_call&& other) noexcept :
        _lambda(std::move(other._lambda)),
        _fCall(other._fCall)
    {
        other._fCall = false;
    }

    ~test_lambda_call() noexcept
    {
        reset();
    }

    // Ensures the scope_exit lambda will not be called
    void release() noexcept
    {
        _fCall = false;
    }

    // Executes the scope_exit lambda immediately if not yet run; ensures it will not run again
    void reset() noexcept
    {
        if (_fCall)
        {
            _fCall = false;
            _lambda();
        }
    }

    // Returns true if the scope_exit lambda is still going to be executed
    explicit operator bool() const noexcept
    {
        return _fCall;
    }

protected:
    TLambda _lambda;
    bool _fCall { true };
};

// Returns an object that executes the given lambda when destroyed.
// Capture the object with 'auto'; use reset() to execute the lambda early or release() to avoid
// execution. Exceptions thrown in the lambda will fail-fast; use scope_exit_log to avoid.
template <typename TLambda>
inline auto test_scope_exit(TLambda&& lambda) noexcept
{
    return test_lambda_call<TLambda>(std::forward<TLambda>(lambda));
}
