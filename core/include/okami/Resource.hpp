#pragma once
#include <atomic>

#include <entt/entt.hpp>

#include <marl/event.h>

#include <okami/PlatformDefs.hpp>

namespace okami {
    typedef int64_t ref_count_t;
	typedef int64_t resource_id_t;

    class Resource {
    private:
		resource_id_t mId = -1;
		marl::Event mOnLoad;
		void* mBackend = nullptr;

	protected:
		inline void SetId(resource_id_t value) {
			mId = value;
		}

    public:
		virtual ~Resource() = default;

		inline marl::Event& OnLoadEvent() {
			return mOnLoad;
		}

		inline resource_id_t Id() const {
			return mId;
		}

		inline void* GetBackend() {
			return mBackend;
		}

		inline void SetBackend(void* value) {
			mBackend = value;
		}

        inline Resource() : 
			mOnLoad(marl::Event::Mode::Manual) {
		}
		inline Resource(const Resource& other) : 
			Resource() {
			assert(other.mBackend == nullptr);
		}

		inline Resource(Resource&& other) : 
			Resource() {
			assert(other.mBackend == nullptr);
		}

		inline Resource& operator=(Resource&& other) {
			assert(other.mBackend == nullptr);
			return *this;
		}

		inline Resource& operator=(const Resource& other) {
			assert(other.mBackend == nullptr);
			return *this;
		}

		virtual entt::meta_type GetType() const = 0;
    };

	typedef void(*resource_destructor_t)(void* owner, Resource* object);

	struct RefCountWrapper {
		std::atomic<ref_count_t> mRefCount = 1;
        void* const mOwner;
        resource_destructor_t const mDestructor;

		void AddRef() {
			++mRefCount;
		}

		void Release(Resource* object) {
			auto value = mRefCount.fetch_sub(1);

			if (value == 1) {
				if (mOwner) {
					mDestructor(mOwner, object);
				} else {
					delete object;
				}

				delete this;
			}
		}

		RefCountWrapper(void* owner, resource_destructor_t destructor) :
			mOwner(owner), mDestructor(std::move(destructor)) {
		}
	};

    template <typename T>
	class Handle {
	private:
		T* mResource;
		RefCountWrapper* mRefCounter;
		
	public:
		inline Handle(T&& resource) : 
			mResource(new T(std::move(resource))),
			mRefCounter(new RefCountWrapper(nullptr, nullptr)) {
		}

		inline Handle(T&& resource, void* owner, resource_destructor_t destructor) : 
			mResource(new T(std::move(resource))),
			mRefCounter(new RefCountWrapper(owner, std::move(destructor))) {
		}

		inline Handle() : 
			mResource(nullptr),
			mRefCounter(nullptr) {
		}

		inline Handle(T* resource) : 
			mResource(resource),
			mRefCounter(nullptr) {
			if (resource) {
				mRefCounter = new RefCountWrapper(nullptr, nullptr);
			}
		}

		inline Handle(const Handle<T>& h) : 
			mResource(h.mResource),
			mRefCounter(h.mRefCounter) {
			if (mResource) {
				mRefCounter->AddRef();
			}
		}

		inline Handle(Handle<T>&& h) : 
			mResource(h.mResource), 
			mRefCounter(h.mRefCounter) {
			h.mResource = nullptr;
			h.mRefCounter = nullptr;
		}

		inline void Release() {
			if (mResource) {
				mRefCounter->Release(mResource);
			}
		}

		inline void Adopt(T* resource, void* owner, resource_destructor_t destructor) {
			Release();

			mResource = resource;
			if (resource) {
				mRefCounter = new RefCountWrapper(owner, std::move(destructor));
			}
		}

		inline void Adopt(T* resource) {
			Adopt(resource, nullptr, nullptr);
		}

		inline Handle<T>& operator=(const Handle<T>& h) {
			Release();

			if (h.mRefCounter) {
				h.mRefCounter->AddRef();
			}

			mResource = h.mResource;
			mRefCounter = h.mRefCounter;

			return *this;
		}

		inline Handle<T>& operator=(Handle<T>&& h) {
			Release();

			mResource = h.mResource;
			mRefCounter = h.mRefCounter;
			h.mResource = nullptr;
			h.mRefCounter = nullptr;
			return *this;
		}

		inline ~Handle() {
			Release();
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

		struct Hasher {
			inline std::size_t operator()(const Handle<T>& h) const
			{
				using std::hash;
				return hash<T*>()(h.mResource);
			}
		};
	};
}