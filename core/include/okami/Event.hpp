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
            for (auto &[key, value] : mMap) {
                value(__args...);
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
            for (auto it = mEntries.begin(); it != mEntries.end(); ++it) {
                if (it->mHandle == handle) {
                    UserData* data = it->mData;
                    mEntries.erase(it);
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