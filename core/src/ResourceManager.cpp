#include <okami/ResourceManager.hpp>
#include <okami/ext/ngraph.hpp>

namespace okami::core {

    using graph_t = NGraph::tGraph<resource_id_t>;

    struct ResourceDigraph {
        graph_t mGraph;
    };

    ResourceManager::ResourceManager() : 
        mDependencies(std::make_unique<ResourceDigraph>()) {
    }

    ResourceManager::~ResourceManager() {
    }

    resource_id_t ResourceManager::MakeNode() {
        resource_id_t id = mCurrentId++;
        mDependencies->mGraph.insert_vertex(id);
        return id;
    }

    void ResourceManager::SendToGarbage(resource_id_t id) {
        mGarbage.emplace(id);

        auto& graph = mDependencies->mGraph;

        std::queue<resource_id_t> potentialGarbage;
        potentialGarbage.emplace(id);

        do {
            auto vert = potentialGarbage.front();
            potentialGarbage.pop();

            if (graph.out_degree(vert) == 0 && vert != INVALID_RESOURCE) {
                auto vertOut = graph.in_neighbors(vert);

                for (auto outIt = vertOut.begin(); outIt != vertOut.end(); ++outIt) {
                    potentialGarbage.emplace(*outIt);
                }

                mGarbage.emplace(vert);
                graph.remove_vertex(vert);
            }

        } while (!potentialGarbage.empty());
    }

    void ResourceManager::AddDependency(
        resource_id_t child, 
        resource_id_t parent) {
        auto& graph = mDependencies->mGraph;

        graph.insert_edge(child, parent);
    }

    void ResourceManager::CollectGarbage() {
        for (auto resId : mGarbage) {
            auto descIt = mResourceDescs.find(resId);
            auto resDesc = descIt->second;

            // Notify backend
            auto destroyerIt = mDestroyNotifiers.find(resDesc.mType);

            if (destroyerIt != mDestroyNotifiers.end()) {
                destroyerIt->second(resId, resDesc.mPointer);
            }

            // Erase records of this resource
            if (descIt->second.bHasLoadParams) {
                mPathToResource.erase(descIt->second.mPath);
            }
            mResourceDescs.erase(descIt);

            // Deallocate if resource is managed
            if (resDesc.bIsManaged) {
                mPools.Remove(resId, resDesc.mType);
            }
        }

        mGarbage.clear();
    }
}