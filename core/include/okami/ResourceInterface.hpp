#pragma once

#include <filesystem>
#include <unordered_map>
#include <atomic>

#include <entt/entt.hpp>

#include <okami/PlatformDefs.hpp>
#include <okami/Resource.hpp>
#include <okami/Async.hpp>
#include <okami/Hashers.hpp>

namespace okami::core {

    template <typename T>
    struct LoadParams;

    class IResourceManagerAbstract {
    public:
        virtual entt::meta_type GetType() const = 0;
    };

    template <typename T>
    class IResourceManager : public IResourceManagerAbstract {
    public:
        virtual Handle<T> Load(
            const std::filesystem::path& path, 
            const LoadParams<T>& params, 
            resource_id_t newResId) = 0;

        virtual Handle<T> Add(T&& obj, 
            resource_id_t newResId) = 0;
        virtual Handle<T> Add(T&& obj, 
            const std::filesystem::path& path, 
            resource_id_t newResId) = 0;

        entt::meta_type GetType() const override {
            return entt::resolve<T>();
        }
    };

    class ResourceInterface {
    private:
        std::unordered_map<entt::meta_type, 
            IResourceManagerAbstract*, TypeHash> mLoaderInterfaces;
        std::atomic<resource_id_t> mCurrentId = 0;
       
    public:
        template <typename T>
        inline void Register(IResourceManager<T>* loader) {
            mLoaderInterfaces.emplace(entt::resolve<T>(), loader);
        }

        template <typename T>
        Handle<T> Load(const std::filesystem::path& path, 
            const LoadParams<T>& params = LoadParams<T>()) {
            auto type = entt::resolve<T>();
            auto it = mLoaderInterfaces.find(type);

            if (it == mLoaderInterfaces.end()) {
                throw std::runtime_error("No registered interface for this type!");
            }

            auto lookup = dynamic_cast<IResourceManager<T>*>(it->second);

            auto newId = mCurrentId.fetch_add(1);
            return lookup->Load(path, params, newId);
        }

        template <typename T>
        Handle<T> Add(T&& obj) {
            auto type = entt::resolve<T>();
            auto it = mLoaderInterfaces.find(type);

            if (it == mLoaderInterfaces.end()) {
                throw std::runtime_error("No registered interface for this type!");
            }

            auto lookup = dynamic_cast<IResourceManager<T>*>(it->second);

            auto newId = mCurrentId.fetch_add(1);
            return lookup->Add(std::move(obj), newId);
        }

        template <typename T>
        Handle<T> Add(T&& obj, const std::filesystem::path& path) {
            auto type = entt::resolve<T>();
            auto it = mLoaderInterfaces.find(type);

            if (it == mLoaderInterfaces.end()) {
                throw std::runtime_error("No registered interface for this type!");
            }

            auto lookup = dynamic_cast<IResourceManager<T>*>(it->second);

            auto newId = mCurrentId.fetch_add(1);
            return lookup->Add(std::move(obj), path, newId);
        }
    };
}