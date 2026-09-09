#pragma once

#include "model.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace tour {

// 图的只读视图，算法层只跟它打交道。
// 顶点一直按 id 升序放着，顺序和 /api/attractions 返回的一致
class Graph {
public:
    static constexpr double NO_EDGE = -1.0;  // weight() 没有边时的返回值

    int vertexCount() const { return (int)vertices_.size(); }
    const std::vector<Attraction>& attractions() const { return vertices_; }
    const std::vector<Edge>& edges() const { return edges_; }

    std::vector<int> vertexIds() const;      // 升序
    bool hasVertex(int id) const;
    const Attraction* findAttraction(int id) const;  // 没有返回 nullptr

    int indexOf(int id) const;  // id -> 升序序列里的下标，没有返回 -1
    int idAt(int index) const;

    bool hasEdge(int u, int v) const;   // 无向
    double weight(int u, int v) const;  // 无向，没有边返回 NO_EDGE
    std::vector<std::pair<int, double>> neighbors(int id) const;  // 已按邻点 id 升序

    // 增删方法只改结构不做校验，校验都在 GraphStore 里做
    void clear();
    void addAttraction(Attraction a);   // 插入后仍保持 id 升序
    bool removeAttraction(int id);      // 相连的道路一起删
    void setEdge(int u, int v, double distance);
    bool removeEdge(int u, int v);      // 没有这条边返回 false

private:
    std::vector<Attraction> vertices_;  // 按 id 升序
    std::vector<Edge> edges_;           // 按加入的顺序
};

// 从数据文件读图（data/*.json 的格式），读不出来抛 ApiError。
// 预设名（文件里的 name 或 city 字段）通过 presetName 带出来，可传空指针
std::shared_ptr<Graph> loadGraphFromFile(const std::string& path, std::string* presetName);

// 从一个解析好的 JSON 对象构建图，校验 attractions/edges（同上），失败抛 ApiError
std::shared_ptr<Graph> loadGraphFromJson(const nlohmann::json& root, std::string* presetName);

// 线程安全的图存储。httplib 是多线程处理请求的，改图之前要加锁；
// 读的时候在锁内拷一份快照出来，算法在快照上跑，不用一直占着锁
class GraphStore {
public:
    // dataFilePath 为空 = 稍后用 setEmbeddedDefault 注入内嵌默认数据（单文件可执行模式）
    explicit GraphStore(std::string dataFilePath);

    void reload();  // 重新加载数据源（POST /api/graph/reset）

    std::shared_ptr<const Graph> snapshot() const;
    std::string name() const;  // 当前预设名（默认数据叫"合肥"）

    void setEmbeddedDefault(const std::string& json);  // 注入内嵌默认数据并加载
    void importGraph(const nlohmann::json& preset);    // 用导入的预设整体替换当前图

    Attraction addAttraction(const Attraction& tpl);  // id 自动分配
    void removeAttraction(int id);
    Edge addEdge(const Edge& e);
    void removeEdge(int u, int v);  // u v 顺序随便

private:
    std::string dataFilePath_;      // 为空 = 使用内嵌默认数据
    std::string embeddedDefault_;   // 内嵌的默认数据 JSON
    mutable std::mutex mutex_;
    Graph graph_;
    std::string name_;
};

}  // namespace tour