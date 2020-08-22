#pragma once
#include <conduit/continuation.hpp>

namespace conduit::mixin {
template <class Derived>
class Resumable {
    constexpr static size_t buffer_size = noop_continuation_size;
    alignas(void*) mutable char buffer[buffer_size]{};
    continuation<Resumable> run() { co_return; }

    void* alloc(size_t size) {
        if (size <= buffer_size) {
            return buffer;
        } else {
            constexpr size_t offset = sizeof(Resumable*);
            char* data = new char[offset + size];
            new (data) Resumable*(this);
            return offset + data;
        }
    }
    static void dealloc(void* addr, size_t size) {
        if (size <= buffer_size) {
            Resumable* pointer_to_allocator = (Resumable*)addr;
            static_cast<Derived*>(pointer_to_allocator)->resume();
        } else {
            constexpr size_t offset = sizeof(Resumable*);
            Resumable* pointer_to_allocator =
                *(Resumable**)((char*)addr - offset);
            delete[]((char*)addr - offset);
            static_cast<Derived*>(pointer_to_allocator)->resume();
        }
    }
    friend class mixin::NewAndDelete<Resumable>;

   public:
    constexpr bool await_ready() noexcept { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) {
        static_cast<Derived*>(this)->set_caller(caller);
        return run();
    }
    constexpr void await_resume() noexcept {}
};
} // namespace conduit::mixin