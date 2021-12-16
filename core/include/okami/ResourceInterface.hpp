#pragma once

#include <filesystem>
#include <unordered_map>
#include <atomic>

#include <entt/entt.hpp>
#include <okami/Resource.hpp>
#include <okami/Async.hpp>

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
        virtual Future<Handle<T>> Load(
            const std::filesystem::path& path, 
            const LoadParams<T>& params, 
            resource_id_t newResId) = 0;

        virtual Future<Handle<T>> Add(T&& obj, 
            resource_id_t newResId) = 0;
        virtual Future<Handle<T>> Add(T&& obj, 
            const std::filesystem::path& path, 
            resource_id_t newResId) = 0;

        entt::meta_type GetType() const override {
            return entt::resolve<T>();
        }
    };

    class ResourceInterface {
    private:
        std::unordered_map<entt::id_type, IResourceManagerAbstract*> mLoaderInterfaces;
        std::atomic<resource_id_t> mCurrentId = 0;
       
    public:
        inline void Register(IResourceManagerAbstract* loader) {
            mLoaderInterfaces.emplace(loader->GetType().id(), loader);
        }

        template <typename T>
        Future<Handle<T>> Load(const std::filesystem::path& path, 
            const LoadParams<T>& params = LoadParams<T>()) {
            auto type = entt::resolve<T>();
            auto it = mLoaderInterfaces.find(type.id());

            if (it == mLoaderInterfaces.end()) {
                throw std::runtime_error("No registered interface for this type!");
            }

            auto lookup = dynamic_cast<IResourceManager<T>*>(it->second);

            auto newId = mCurrentId.fetch_add(1);
            return lookup->Load(path, params, newId);
        }

        template <typename T>
        Future<Handle<T>> Add(T&& obj) {
            auto type = entt::resolve<T>();
            auto it = mLoaderInterfaces.find(type.id());

            if (it == mLoaderInterfaces.end()) {
                throw std::runtime_error("No registered interface for this type!");
            }

            auto lookup = dynamic_cast<IResourceManager<T>*>(it->second);

            auto newId = mCurrentId.fetch_add(1);
            return lookup->Add(std::move(obj), newId);
        }

        template <typename T>
        Future<Handle<T>> Add(T&& obj, const std::filesystem::path& path) {
            auto type = entt::resolve<T>();
            auto it = mLoaderInterfaces.find(type.id());

            if (it == mLoaderInterfaces.end()) {
                throw std::runtime_error("No registered interface for this type!");
            }

            auto lookup = dynamic_cast<IResourceManager<T>*>(it->second);

            auto newId = mCurrentId.fetch_add(1);
            return lookup->Add(std::move(obj), path, newId);
        }
    };
}