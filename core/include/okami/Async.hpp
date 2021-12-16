#pragma once

#include <memory>
#include <marl/event.h>
#include <okami/RefCount.hpp>

namespace okami::core {
    template <typename T>
    struct PromiseUnderlying {
        marl::Event mEvent;
        T mData;
    };

    template <typename T>
    class Promise {
    private:
        std::shared_ptr<PromiseUnderlying<T>> mPtr;

    public:
        inline const T& Get() const {
            mPtr->mEvent.wait();
            return mPtr->mData;
        }

        inline void Set(T&& obj) {
            mPtr->mData = std::move(obj);
            mPtr->mEvent.done();
        }

        inline void Set(const T& obj) {
            mPtr->mData = obj;
            mPtr->mEvent.done();
        }

        friend class Future<T>;
    };
    
    template <typename T>
    class WeakFuture;
    
    template <typename T>
    class Future {
    private:
        std::shared_ptr<PromiseUnderlying<T>> mPtr;

    public:
        inline Future() {
        }

        inline Future(const Promise<T>& promise) : 
            mPtr(promise.mPtr) {
        }

        inline Future(const WeakFuture<T>& future);

        inline const T& Get() const {
            mPtr->mEvent.wait();
            return mPtr->mData;
        }
    };

    template <typename T>
    class WeakFuture {
    private:
        std::weak_ptr<PromiseUnderlying<T>> mPtr;

    public:

        inline WeakFuture() {
        }

        inline WeakFuture(const Promise<T>& promise) : 
            mPtr(promise.mPtr) {
        }

        inline WeakFuture(const Future<T>& future) : 
            mPtr(future.mPtr) {
        }

        inline const T& Get() const {
            mPtr->mEvent.wait();
            return mPtr->mData;
        }
    };

    template <typename T>
    inline Future<T>::Future(const WeakFuture<T>& future) : mPtr(future.mPtr) {
    }
}