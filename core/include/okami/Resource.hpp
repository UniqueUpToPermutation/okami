#pragma once
#include <atomic>

#include <entt/entt.hpp>

#include <marl/event.h>

#include <okami/PlatformDefs.hpp>

namespace okami {
    typedef int64_t ref_count_t;
	typedef int64_t resource_id_t;

	namespace core {
		class ResourceInterface;
	}

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

		friend class core::ResourceInterface;
    };

	template <typename T, bool isWeak=false>
	class Handle;

	typedef void(*resource_destructor_t)(void* owner, Handle<Resource, true> object);

	struct RefCountWrapper {
		std::atomic<ref_count_t> mRefCount = 1;
        void* const mOwner;
        resource_destructor_t const mDestructor;

		void AddRef() {
			++mRefCount;
		}

		void Release(Handle<Resource, true> object);

		RefCountWrapper(void* owner, resource_destructor_t destructor) :
			mOwner(owner), mDestructor(std::move(destructor)) {
		}
	};

    template <typename T, bool isWeak>
	class Handle {
	private:
		T* mResource;
		RefCountWrapper* mRefCounter;
		
	public:
		inline Handle(T* resource, RefCountWrapper* refCounter) :
			mResource(resource),
			mRefCounter(refCounter) {
			if constexpr (!isWeak) {
				if (mResource) {
					mRefCounter->AddRef();
				}
			}
		}

		inline Handle(T&& resource, void* owner, resource_destructor_t destructor) : 
			mResource(new T(std::move(resource))),
			mRefCounter(new RefCountWrapper(owner, std::move(destructor))) {
		}

		inline Handle() : 
			mResource(nullptr),
			mRefCounter(nullptr) {
		}

		inline Handle(std::nullptr_t) :
			Handle() {
		}

		inline Handle(const Handle<T, isWeak>& h) : 
			mResource(h.mResource),
			mRefCounter(h.mRefCounter) {
			if constexpr (!isWeak) {
				if (mResource) {
					mRefCounter->AddRef();
				}
			}
		}

		inline Handle(const Handle<T, !isWeak>& h) : 
			mResource(h.mResource),
			mRefCounter(h.mRefCounter) {
			if constexpr (!isWeak) {
				if (mResource) {
					mRefCounter->AddRef();
				}
			}
		}

		inline Handle(Handle<T, isWeak>&& h) : 
			mResource(h.mResource), 
			mRefCounter(h.mRefCounter) {
			h.mResource = nullptr;
			h.mRefCounter = nullptr;
		}

		inline void Release() {
			if constexpr (!isWeak) {
				if (mResource) {
					mRefCounter->Release(DownCast<Resource, true>());
				}
			}

			mResource = nullptr;
			mRefCounter = nullptr;
		}

		inline void Adopt(T* resource, void* owner, resource_destructor_t destructor) {
			Release();

			mResource = resource;

			if constexpr (!isWeak) {
				if (resource) {
					mRefCounter = new RefCountWrapper(owner, std::move(destructor));
				}
			}
		}

		inline void Adopt(T* resource) {
			Adopt(resource, nullptr, nullptr);
		}

		inline Handle<T, isWeak>& operator=(std::nullptr_t) {
			Release();

			return *this;
		}

		inline Handle<T, isWeak>& operator=(const Handle<T, isWeak>& h) {
			Release();

			if constexpr (!isWeak) {
				if (h.mRefCounter) {
					h.mRefCounter->AddRef();
				}
			}

			mResource = h.mResource;
			mRefCounter = h.mRefCounter;

			return *this;
		}

		inline Handle<T, isWeak>& operator=(const Handle<T, !isWeak>& h) {
			Release();

			if constexpr (!isWeak) {
				if (h.mRefCounter) {
					h.mRefCounter->AddRef();
				}
			}

			mResource = h.mResource;
			mRefCounter = h.mRefCounter;

			return *this;
		}

		inline Handle<T, isWeak>& operator=(Handle<T, isWeak>&& h) {
			Release();

			std::swap(mResource, h.mResource);
			std::swap(mRefCounter, h.mRefCounter);

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

		inline bool operator==(const Handle<T, isWeak>& other) const {
			return other.mResource == mResource;
		}

		template <typename T2, bool resultIsWeak=false>
		inline Handle<T2, resultIsWeak> TryCast() {
			return Handle<T2, resultIsWeak>(dynamic_cast<T2*>(mResource), mRefCounter);
		}

		template <typename T2, bool resultIsWeak=false>
		inline Handle<T2, resultIsWeak> DownCast() {
			return Handle<T2, resultIsWeak>(static_cast<T2*>(mResource), mRefCounter);
		}

		struct Hasher {
			inline std::size_t operator()(const Handle<T, isWeak>& h) const
			{
				using std::hash;
				return hash<T*>()(h.mResource);
			}
		};

		friend class Handle<T, !isWeak>;
	};

	// A non-owning handle that does not count towards active references
	template <typename T>
	using WeakHandle = Handle<T, true>;

	inline void RefCountWrapper::Release(WeakHandle<Resource> object) {
		auto value = mRefCount.fetch_sub(1);

		if (value == 1) {
			if (mOwner) {
				mDestructor(mOwner, object);
			} else {
				delete object.Ptr();
			}

			delete this;
		}
	}
}