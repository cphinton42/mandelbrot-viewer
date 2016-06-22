#ifndef __DEFER_H__
#define __DEFER_H__

#include <utility>

namespace defer_internal {

template <typename Fun>
class ScopeGuard
{
public:
    ScopeGuard(Fun &&fn, bool exec = true) : fn(std::forward<Fun>(fn)), exec(exec) {}
    ~ScopeGuard() { if(exec) { fn(); } }
    void activate() { exec = true; }
    void deactivate() { exec = false; }
private:
    Fun fn;
    bool exec;
};

enum class ScopeGuardOnExit {};
enum class MaybeGuardOnExit {};

template <typename Fun>
ScopeGuard<Fun> operator+(ScopeGuardOnExit, Fun&& fn)
{
    return ScopeGuard<Fun>(std::forward<Fun>(fn));
}

template <typename Fun>
ScopeGuard<Fun> operator+(MaybeGuardOnExit, Fun&& fn)
{
    return ScopeGuard<Fun>(std::forward<Fun>(fn), false);
}

}

#define CONCATENATE_IMPL(s1,s2) s1##s2
#define CONCATENATE(s1,s2) CONCATENATE_IMPL(s1, s2)

#ifdef __COUNTER__
#define ANONYMOUS_VARIABLE(str) \
    CONCATENATE(str, __COUNTER__)
#else
#define ANONYMOUSE_VARIABLE(str) \
    CONCATENATE(str, __LINE__)
#endif

#define defer \
    auto ANONYMOUS_VARIABLE(SCOPE_EXIT) \
    = ::defer_internal::ScopeGuardOnExit() + [&]()

#define deferred \
    ::defer_internal::ScopeGuardOnExit() + [&]()

#define maybe \
    ::defer_internal::MaybeGuardOnExit() + [&]()

#endif // __DEFER_H__
