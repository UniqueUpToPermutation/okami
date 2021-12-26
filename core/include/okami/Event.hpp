#pragma once

#include <functional>
#include <unordered_map>

namespace okami::core {

    typedef uint32_t delegate_handle_t;

    template<typename... _ArgTypes>
    class Event {
    private:
        delegate_handle_t mCurrentHandle;
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
}