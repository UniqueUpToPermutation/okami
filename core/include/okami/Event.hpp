#pragma once

#include <functional>
#include <unordered_map>

#include <okami/PlatformDefs.hpp>

namespace okami::core {

    typedef uint32_t delegate_handle_t;

    template<typename... _ArgTypes>
    class Event {
    private:
        delegate_handle_t mCurrentHandle = 0;
        std::unordered_map<delegate_handle_t, std::function<void(_ArgTypes...)>> mMap;

    public:
        inline delegate_handle_t Add(std::function<void(_ArgTypes...)> function) {
            mCurrentHandle++;
            mMap.emplace(mCurrentHandle, std::move(function));
            return mCurrentHandle;
        }

        inline void Remove(delegate_handle_t handle) {
            mMap.erase(handle);
        }

        inline void operator()(_ArgTypes... __args) {
            for (auto& [key, value] : mMap) {
                value(__args...);
            }
        }
    };

    template<typename UserData, typename... _ArgTypes>
    class UserDataEvent {
    private:
        struct Entry {
            delegate_handle_t mHandle;
            std::function<void(_ArgTypes...)> mFunc;
            UserData mData;
        };

        std::unordered_multimap<UserData, Entry> mEntries;
        std::unordered_map<delegate_handle_t,
            typename std::unordered_multimap<UserData, Entry>::iterator> 
                mHandleToIt;

        delegate_handle_t mCurrentHandle = 0;

    public:
        inline delegate_handle_t Add(UserData data, std::function<void(_ArgTypes...)> function) {
            mCurrentHandle++;

            Entry e;
            e.mHandle = mCurrentHandle;
            e.mFunc = std::move(function);
            e.mData = data;

            auto it = mEntries.emplace(data, std::move(e));
            mHandleToIt.emplace(e.mHandle, it);
            return mCurrentHandle;
        }

        inline void ForEachData(std::function<void(UserData)> func) {
            for (auto& [data, entry] : mEntries) {
                func(data);
            }
        }

        inline void Remove(delegate_handle_t handle) {
            auto it = mHandleToIt.find(handle);

            if (it != mHandleToIt.end()) {
                mEntries.erase(it->second);
                mHandleToIt.erase(it);
            }
        }

        inline void RemoveAll(UserData data) {
            auto range = mEntries.equal_range(data);

            for (auto it = range.first; it != range.second;) {
                mHandleToIt.erase(it->second.mHandle);
                mEntries.erase(it++);
            }
        }

        inline void operator()(_ArgTypes... __args) {
            for (auto& [data, e] : mEntries) {
                e.mFunc(__args...);
            }
        }

        inline void InvokeOnly(UserData data, _ArgTypes... __args) {
            auto range = mEntries.equal_range(data);

            for (auto it = range.first; it != range.second; ++it) {
                auto& e = it->second;
                e.mFunc(std::forward<_ArgTypes>(__args)...);
            }
        }
    };

    template<typename UserData, typename... _ArgTypes>
    class OrderedEvent {
    private:
        struct Entry {
            delegate_handle_t mHandle;
            std::function<bool(_ArgTypes...)> mFunc;
            UserData* mData;
            double mPriority;
        };

        std::vector<Entry> mEntries;
        bool bDirty = false;
        delegate_handle_t mCurrentHandle = 0;

    public:
        delegate_handle_t Add(
            std::function<bool(_ArgTypes...)> function, 
            UserData* userData,
            double priority = 0.0) {
            
            bDirty = true;
            mCurrentHandle++;

            Entry e;
            e.mHandle = mCurrentHandle;
            e.mFunc = std::move(function);
            e.mPriority = priority;
            e.mData = userData;

            mEntries.emplace_back(std::move(e));
            return mCurrentHandle;
        }

        delegate_handle_t Add(
            std::function<bool(_ArgTypes...)> function, 
            double priority = 0.0) {
            return Add(std::move(function), nullptr, priority);
        }

        UserData* Remove(delegate_handle_t handle) {
            for (auto it = mEntries.begin(); it != mEntries.end();) {
                if (it->mHandle == handle) {
                    UserData* data = it->mData;
                    mEntries.erase(it++);
                    return data;
                }
            }

            throw std::runtime_error("Handle is invalid");
        }

        UserData* operator()(_ArgTypes... __args) {
            if (bDirty) {
                std::sort(mEntries.begin(), mEntries.end(), 
                    [](const Entry& e1, const Entry& e2) {
                    return e1.mPriority < e2.mPriority;
                });
                bDirty = false;
            }

            for (auto it = mEntries.rbegin(); it != mEntries.rend(); ++it) {
                bool val = it->mFunc(__args...);
                if (val)
                    return it->mData;
            }

            return nullptr;
        }

        bool InvokeOnly(UserData* userData, _ArgTypes... __args) {
            for (auto it = mEntries.rbegin(); it != mEntries.rend(); ++it) {
                if (it->mData == userData)
                    return it->mFunc(__args...);
            }
            return false;
        }
    };
}