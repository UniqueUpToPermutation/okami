#pragma once

#include <okami/System.hpp>
#include <okami/ResourceInterface.hpp>

#include <entt/entt.hpp>

#include <stack>
#include <filesystem>

namespace okami::core {
    struct HierarchyData {
		entt::entity mParent;
		entt::entity mPrevious;
		entt::entity mNext;
		entt::entity mFirstChild;
		entt::entity mLastChild;

		HierarchyData() :
			mParent(entt::null),
			mPrevious(entt::null),
			mNext(entt::null),
			mFirstChild(entt::null),
			mLastChild(entt::null) {
		}

		HierarchyData(entt::entity parent) :
			mParent(parent),
			mPrevious(entt::null),
			mNext(entt::null),
			mFirstChild(entt::null),
			mLastChild(entt::null) {	
		}

		HierarchyData(entt::entity parent,
			entt::entity previous,
			entt::entity next) :
			mParent(parent),
			mPrevious(previous),
			mNext(next),
			mFirstChild(entt::null),
			mLastChild(entt::null) {
		}

		HierarchyData(entt::entity parent,
			entt::entity previous,
			entt::entity next,
			entt::entity firstChild,
			entt::entity lastChild) :
			mParent(parent),
			mPrevious(previous),
			mNext(next),
			mFirstChild(firstChild),
			mLastChild(lastChild) {
		}

		static void Orphan(entt::registry& registry, 
			entt::entity ent);
		static void AddChild(entt::registry& registry, 
			entt::entity parent, 
			entt::entity newChild);
	};

    class DepthFirstNodeIterator {
	private:
		std::stack<entt::entity> mNodeStack;
		entt::registry* mRegistry;

	public:
		DepthFirstNodeIterator(entt::registry* registry, 
			entt::entity start) {
			mNodeStack.emplace(start);
			mRegistry = registry;
		}

		inline entt::entity operator()() {
			return mNodeStack.top();
		}

		inline explicit operator bool() const {
			return !mNodeStack.empty();
		}

		inline DepthFirstNodeIterator& operator++() {
			auto& top = mNodeStack.top();

			mNodeStack.emplace(mRegistry->get<HierarchyData>(top).mFirstChild);
			
			while (!mNodeStack.empty() && mNodeStack.top() == entt::null) {
				mNodeStack.pop();
				if (!mNodeStack.empty()) {
					top = mNodeStack.top();
					mNodeStack.pop();
					mNodeStack.emplace(mRegistry->get<HierarchyData>(top).mNext);
				}
			}

			return *this;
		}
	};

	enum class IteratorDirection {
		DOWN,
		UP
	};

	class DepthFirstNodeDoubleIterator {
	private:
		std::stack<entt::entity> mNodeStack;
		IteratorDirection mDirection;
		entt::registry* mRegistry;

	public:
		DepthFirstNodeDoubleIterator(
			entt::registry* registry, 
			entt::entity start) {

			mNodeStack.emplace(start);
			mDirection = IteratorDirection::DOWN;
			mRegistry = registry;
		}

		inline entt::entity operator()() {
			return mNodeStack.top();
		}

		inline IteratorDirection GetDirection() const {
			return mDirection;
		}

		inline explicit operator bool() const {
			return !mNodeStack.empty();
		}

		inline DepthFirstNodeDoubleIterator& operator++() {
			auto& top = mNodeStack.top();

			if (mDirection == IteratorDirection::UP) {
				mNodeStack.pop();
				mNodeStack.emplace(mRegistry->get<HierarchyData>(top).mNext);
				mDirection = IteratorDirection::DOWN;
			} else {
				mNodeStack.emplace(mRegistry->get<HierarchyData>(top).mFirstChild);
				mDirection = IteratorDirection::DOWN;
			}
			
			if (!mNodeStack.empty() && mNodeStack.top() == entt::null) {
				mNodeStack.pop();
				if (!mNodeStack.empty()) {
					mDirection = IteratorDirection::UP;
				}
			}

			return *this;
		}
	};

	class Frame final : public Resource {
	private:
		entt::registry mRegistry;
		entt::entity mRoot;
		bool bIsUpdating = false;

	public:
		inline void SetUpdating(bool value) {
			bIsUpdating = value;
		}

		Frame(Frame&&) = default;
		Frame& operator=(Frame&&) = default;

		Frame(const Frame&) = delete;
		Frame& operator=(const Frame&) = delete;

		inline entt::registry& Registry() {
			return mRegistry;
		}
		inline entt::entity GetRoot() const {
			return mRoot;
		}

		entt::entity CreateEntity(entt::entity parent);
		void Destroy(entt::entity ent);

		Frame();
		~Frame() = default;

		inline void Orphan(entt::entity ent) {
			HierarchyData::Orphan(mRegistry, ent);
		}
		inline void AddEntityChild(entt::entity parent, entt::entity newChild) {
			HierarchyData::AddChild(mRegistry, parent, newChild);
		}
		inline void SetEntityParent(entt::entity child, entt::entity parent) {
			AddEntityChild(parent, child);
		}
		inline entt::entity CreateEntity() {
			return CreateEntity(mRoot);
		}
		inline entt::entity GetEntityParent(entt::entity ent) {
			return mRegistry.get<HierarchyData>(ent).mParent;
		}
		inline entt::entity GetFirstChild(entt::entity ent) {
			return mRegistry.get<HierarchyData>(ent).mFirstChild;
		}
		inline entt::entity GetLastChild(entt::entity ent) {
			return mRegistry.get<HierarchyData>(ent).mLastChild;
		}
		inline entt::entity GetNextEntity(entt::entity ent) {
			return mRegistry.get<HierarchyData>(ent).mNext;
		}
		inline entt::entity GetPreviousEntity(entt::entity ent) {
			return mRegistry.get<HierarchyData>(ent).mPrevious;
		}
		inline DepthFirstNodeIterator GetIterator() {
			return DepthFirstNodeIterator(&mRegistry, mRoot);
		}
		inline DepthFirstNodeDoubleIterator GetDoubleIterator() {
			return DepthFirstNodeDoubleIterator(&mRegistry, mRoot);
		}
		inline DepthFirstNodeIterator GetIterator(entt::entity subtree) {
			return DepthFirstNodeIterator(&mRegistry, subtree);
		}
		inline DepthFirstNodeDoubleIterator GetDoubleIterator(entt::entity subtree) {
			return DepthFirstNodeDoubleIterator(&mRegistry, subtree);
		}
		template <typename T>
		inline T& Get(entt::entity e) {
			return mRegistry.get<T>(e);
		}
		template <typename T>
		inline T* TryGet(entt::entity e) {
			return mRegistry.try_get<T>(e);
		}
		template <typename T>
		inline T& Replace(entt::entity e, const T& obj) {
			return mRegistry.replace<T>(e, obj);
		}
		template <typename T>
		inline T& Replace(entt::entity e, T&& obj) {
			return mRegistry.replace<T>(e, std::move(obj));
		}
		template <typename T, typename... Args>
		inline T& Emplace(entt::entity e, Args&&... args) {
			return mRegistry.emplace<T>(e, std::forward<Args>(args)...);
		}

		entt::meta_type GetType() const override;
		static void Register();

		friend class FrameIO;
		friend class FrameTable;
	};

	template <>
	struct LoadParams<Frame> {
	};
}