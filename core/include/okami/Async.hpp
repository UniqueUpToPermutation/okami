#pragma once

#include <memory>
#include <queue>

#include <marl/event.h>
#include <okami/Resource.hpp>

namespace okami::core {
    template <typename T>
    struct PromiseUnderlying {
        marl::Event mEvent;
        T mData;
    };
    
    template <typename T>
    class Future;

    template <typename T>
    class Promise {
    private:
        std::shared_ptr<PromiseUnderlying<T>> mPtr;

    public:
        inline Promise() : mPtr(std::make_shared<PromiseUnderlying<T>>()) {
        }

        inline Promise(T&& t) : Promise() {
            Set(std::move(t));       
        }

        inline const T& Get() const {
            mPtr->mEvent.wait();
            return mPtr->mData;
        }

        inline void Set(T&& obj) {
            mPtr->mData = std::move(obj);
            mPtr->mEvent.signal();
        }

        inline void Set(const T& obj) {
            mPtr->mData = obj;
            mPtr->mEvent.signal();
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

        inline const T& Get() const {
            mPtr->mEvent.wait();
            return mPtr->mData;
        }

        inline operator bool() const {
            return mPtr;
        }
    };

    template <typename T>
    class MessagePipe {
    private:
        std::queue<T> mProducerMessages;
        std::atomic<bool> bHasMessages = false;
        std::queue<T> mConsumerMessages;
        marl::mutex mProducerMutex;

    public:
        // Tries to collect all messages. Called by the consumer!
        inline void ConsumerTryCollect() {
            assert(mConsumerMessages.empty());

            if (mProducerMutex.try_lock()) {
                std::swap(mProducerMessages, mConsumerMessages);
                bHasMessages = false;
                mProducerMutex.unlock();
            }
        }

        // Collects all messages. Called by the consumer! May lock.
        inline void ConsumerCollect(bool bBlock = true) {
            if (!bBlock) {
                ConsumerTryCollect();
            } else {
                assert(mConsumerMessages.empty());

                marl::lock lock(mProducerMutex);
                std::swap(mProducerMessages, mConsumerMessages);
                bHasMessages = false;
            }
        }

        inline bool ConsumerIsEmpty() const {
            return mConsumerMessages.empty();
        }

        inline T& ConsumerFront() {
            return mConsumerMessages.front();
        }

        inline void ConsumerPop() {
            mConsumerMessages.pop();
        }

        inline void ProducerEnqueue(T item) {
            marl::lock lock(mProducerMutex);
            mProducerMessages.push(std::move(item));
        }

        template <typename Iterator>
        inline void ProducerEnqueue(Iterator it1, Iterator it2) {
            marl::lock lock(mProducerMutex);
            for (auto it = it1; it != it2; it++) {
                mProducerMessages.push(std::move(*it));
            }
        }

        inline bool HasMessages() const {
            return bHasMessages;
        }
    };
}