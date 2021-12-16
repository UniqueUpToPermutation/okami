#pragma once

#include <filesystem>
#include <unordered_map>
#include <atomic>

#include <entt/entt.hpp>
#include <okami/RefCount.hpp>
#include <okami/Async.hpp>

namespace okami::core {

    typedef int64_t resource_id_t;

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

        virtual void Add(Handle<T> obj, resource_id_t newResId) = 0;

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
            mLoaderInterfaces.emplace(loader->GetType(), loader);
        }

        template <typename T>
        void Load(const std::filesystem::path& path, 
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
        void Add(Handle<T> obj) {
            auto type = entt::resolve<T>();
            auto it = mLoaderInterfaces.find(type.id());

            if (it == mLoaderInterfaces.end()) {
                throw std::runtime_error("No registered interface for this type!");
            }

            auto lookup = dynamic_cast<IResourceManager<T>*>(it->second);

            auto newId = mCurrentId.fetch_add(1);
            return lookup->Add(obj, newId);
        }
    };
}