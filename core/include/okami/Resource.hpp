#pragma once
#include <atomic>

#include <entt/entt.hpp>

namespace okami::core {
    typedef int32_t ref_count_t;
	typedef int64_t resource_id_t;

    class Resource {
    private:
        std::atomic<ref_count_t> mRefCount;
        void* mOwner;
        void (*mDestructor)(void* owner, Resource* object);
		resource_id_t mId = -1;

	protected:
		inline void SetId(resource_id_t value) {
			mId = value;
		}

    public:
		inline resource_id_t Id() const {
			return mId;
		}

        inline Resource() : 
            mRefCount(1), 
            mOwner(nullptr), 
            mDestructor(nullptr) {
        }

		inline Resource(const Resource& other) : 
			Resource() {
		}

		inline Resource(Resource&& other) : 
			Resource() {
			if (other.mOwner) {
				throw std::runtime_error("Cannot move managed resource!");
			}
			if (other.mRefCount > 1) {
				throw std::runtime_error("Cannot move resource with more than one reference!");
			}
		}

		inline Resource& operator=(Resource& other) {
			return *this;
		}

		inline Resource& operator=(Resource&& other) {
			if (other.mOwner) {
				throw std::runtime_error("Cannot move managed resource!");
			}
			if (other.mRefCount > 1) {
				throw std::runtime_error("Cannot move resource with more than one reference!");
			}

			return *this;
		}

        inline void SetDestructor(void (*destructor)(void*, Resource*), void* owner) {
            mDestructor = destructor;
            mOwner = owner;
        }

        inline void AddRef() {
            mRefCount++;
        }

        inline void Release() {
            auto result = mRefCount.fetch_sub(1);
        
            if (result == 1) {
                if (mDestructor) {
                    mDestructor(mOwner, this);
                } else {
                    delete this;
                }
            }
        }

		virtual entt::meta_type GetType() const = 0;
    };

    template <typename T>
	class Handle {
	private:
		T* mResource;

	public:
		inline Handle(T&& resource) : 
			mResource(new T(std::move(resource))) {
		}

		inline Handle() : mResource(nullptr) {
		}

		inline Handle(T* resource) : mResource(resource) {
			if (resource)
				resource->AddRef();
		}

		inline Handle(const Handle<T>& h) : mResource(h.mResource) {
			if (mResource)
				mResource->AddRef();
		}

		inline Handle(Handle<T>&& h) : mResource(h.mResource) {
			h.mResource = nullptr;
		}

		inline void Adopt(T* resource) {
			if (mResource)
				mResource->Release();

			mResource = resource;
		}

		inline Handle<T>& operator=(const Handle<T>& h) {
			if (mResource)
				mResource->Release();

			mResource = h.mResource;

			if (mResource)
				mResource->AddRef();

			return *this;
		}

		inline Handle<T>& operator=(Handle<T>&& h) {
			if (mResource)
				mResource->Release();

			mResource = h;
			h.mResource = nullptr;
			return *this;
		}

		inline ~Handle() {
			if (mResource)
				mResource->Release();
		}

		inline T* Ptr() const {
			return mResource;
		}

		inline T* operator->() const {
			return mResource;
		}

		inline T** Ref() {
			return &mResource;
		}

		inline operator bool() const {
			return mResource;
		}

		inline operator T*() const {
			return mResource;
		}

		inline bool operator==(const Handle<T>& other) const {
			return other.mResource == mResource;
		}

		template <typename T2>
		inline Handle<T2> TryCast() {
			return Handle<T2>(dynamic_cast<T2*>(mResource));
		}

		template <typename T2>
		inline Handle<T2> DownCast() {
			return Handle<T2>(static_cast<T2*>(mResource));
		}

		inline T* Release() {
			T* result = mResource;
			if (mResource) {
				result->Release();
				mResource = nullptr;
			}
			return result;
		}

		struct Hasher {
			inline std::size_t operator()(const Handle<T>& h) const
			{
				using std::hash;
				return hash<T*>()(h.mResource);
			}
		};
	};
}