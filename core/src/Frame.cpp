#include <okami/Frame.hpp>
#include <okami/Embed.hpp>

using namespace entt;

namespace okami::core {
    bool EmbeddedFileLoader::Exists(const std::filesystem::path& source) const {
		auto searchSource = std::filesystem::relative(source, "");

		auto it = mInternalShaders.find(searchSource);
		if (it != mInternalShaders.end()) {
			return true;
		} else {
			return false;
		}
	}

	bool EmbeddedFileLoader::TryFind(const std::filesystem::path& source, std::string* contents) const {
		auto searchSource = std::filesystem::relative(source, "");
		
		// Search internal shaders first
		auto it = mInternalShaders.find(searchSource);
		if (it != mInternalShaders.end()) {
			*contents = it->second;
			return true;
		}
		return false;
	}

	void EmbeddedFileLoader::Add(const embedded_file_loader_t& factory) {
		factory(&mInternalShaders);
	}

    EmbeddedFileLoader::EmbeddedFileLoader(const embedded_file_loader_t& factory) {
        Add(factory);
    }

    entt::meta_type Frame::GetType() const {
        return entt::resolve<Frame>();
    }

    void Frame::Register() {
        entt::meta<Frame>()
            .type("Frame"_hs);
    }

    Frame::Frame() {
		mRoot = mRegistry.create();
		mRegistry.emplace<HierarchyData>(mRoot);
	}

    void HierarchyData::Orphan(entt::registry& registry, entt::entity ent) {
        HierarchyData& data = registry.get<HierarchyData>(ent);

        if (data.mParent != entt::null) {
            HierarchyData& parentData = registry.get<HierarchyData>(data.mParent);

            if (parentData.mFirstChild == ent) {
                parentData.mFirstChild = data.mNext;
                if (data.mNext != entt::null) {
                    HierarchyData& nextData = registry.get<HierarchyData>(data.mNext);
                    nextData.mPrevious = entt::null;
                }
            }

            if (parentData.mLastChild == ent) {
                parentData.mLastChild = data.mPrevious;
                if (data.mPrevious != entt::null) {
                    HierarchyData& prevData = registry.get<HierarchyData>(data.mPrevious);
                    prevData.mNext = entt::null;
                }
            }
        }

        data.mParent = entt::null;
    }

    void HierarchyData::AddChild(entt::registry& registry, entt::entity parent, entt::entity newChild) {
        auto& childData = registry.get<HierarchyData>(newChild);

        if (childData.mParent != entt::null) {
            // Make this node into an orphan
            Orphan(registry, newChild);
        }

        auto& selfData = registry.get<HierarchyData>(parent);
        
        // Add to the end of linked child list
        if (selfData.mLastChild != entt::null) {
            auto& prevLastData = registry.get<HierarchyData>(selfData.mLastChild);

            prevLastData.mNext = newChild;
            childData.mPrevious = selfData.mLastChild;
            selfData.mLastChild = newChild;

        } else {
            selfData.mFirstChild = newChild;
            selfData.mLastChild = newChild;
        }

        childData.mParent = parent;
    }

    entt::entity Frame::CreateEntity(entt::entity parent) {
        if (bIsUpdating) {
            throw std::runtime_error(
                "Cannot create an entity during an update!");
        }

		auto e = mRegistry.create();
		mRegistry.emplace<HierarchyData>(e);
		AddEntityChild(parent, e);
		return e;
	}

	void Frame::Destroy(entt::entity ent) {
        if (bIsUpdating) {
            throw std::runtime_error(
                "Cannot destroy an entity during an update!");
        }

		Orphan(ent);

		for (entt::entity child = GetFirstChild(ent); 
			child != entt::null; 
			child = GetNextEntity(child)) 
			Destroy(child);

		mRegistry.destroy(ent);
	}
}