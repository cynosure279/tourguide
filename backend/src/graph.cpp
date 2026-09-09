#include "graph.hpp"

#include "algo.hpp"
#include "api_common.hpp"

#include <algorithm>
#include <fstream>
#include <utility>

namespace tour {

// ---------------- JSON 转换（model.hpp 里声明的） ----------------

void to_json(nlohmann::json& j, const Attraction& a) {
    j = nlohmann::json{{"id", a.id},   {"code", a.code}, {"name", a.name},
                       {"intro", a.intro}, {"posx", a.posx}, {"posy", a.posy}};
}

void from_json(const nlohmann::json& j, Attraction& a) {
    j.at("id").get_to(a.id);
    j.at("code").get_to(a.code);
    j.at("name").get_to(a.name);
    a.intro = j.value("intro", "");   // 这几个字段可以没有
    a.posx = j.value("posx", 50.0);
    a.posy = j.value("posy", 50.0);
}

void to_json(nlohmann::json& j, const Edge& e) {
    j = nlohmann::json{{"from", e.from}, {"to", e.to}, {"distance", e.distance}};
}

void from_json(const nlohmann::json& j, Edge& e) {
    j.at("from").get_to(e.from);
    j.at("to").get_to(e.to);
    j.at("distance").get_to(e.distance);
}

// ---------------- Graph ----------------

std::vector<int> Graph::vertexIds() const {
    std::vector<int> ids;
    ids.reserve(vertices_.size());
    for (const auto& a : vertices_) ids.push_back(a.id);
    return ids;
}

// 顶点按 id 升序放着，直接二分找
const Attraction* Graph::findAttraction(int id) const {
    auto it = std::lower_bound(vertices_.begin(), vertices_.end(), id,
                               [](const Attraction& a, int v) { return a.id < v; });
    return it != vertices_.end() && it->id == id ? &*it : nullptr;
}

bool Graph::hasVertex(int id) const { return findAttraction(id) != nullptr; }

int Graph::indexOf(int id) const {
    auto it = std::lower_bound(vertices_.begin(), vertices_.end(), id,
                               [](const Attraction& a, int v) { return a.id < v; });
    if (it == vertices_.end() || it->id != id) return -1;
    return (int)(it - vertices_.begin());
}

int Graph::idAt(int index) const { return vertices_[index].id; }

bool Graph::hasEdge(int u, int v) const { return weight(u, v) != NO_EDGE; }

double Graph::weight(int u, int v) const {
    for (const auto& e : edges_) {
        if ((e.from == u && e.to == v) || (e.from == v && e.to == u)) return e.distance;
    }
    return NO_EDGE;
}

// 邻点列表，按邻点 id 升序。DFS 要按 id 从小到大访问邻点，
// 在这里排好序，后面算法就不用再管顺序了
std::vector<std::pair<int, double>> Graph::neighbors(int id) const {
    std::vector<std::pair<int, double>> ns;
    for (const auto& e : edges_) {
        if (e.from == id)
            ns.push_back({e.to, e.distance});
        else if (e.to == id)
            ns.push_back({e.from, e.distance});
    }
    std::sort(ns.begin(), ns.end());
    return ns;
}

void Graph::clear() {
    vertices_.clear();
    edges_.clear();
}

// 插到合适的位置，保持 id 升序
void Graph::addAttraction(Attraction a) {
    auto it = std::upper_bound(vertices_.begin(), vertices_.end(), a.id,
                               [](int v, const Attraction& x) { return v < x.id; });
    vertices_.insert(it, std::move(a));
}

bool Graph::removeAttraction(int id) {
    if (!hasVertex(id)) return false;
    vertices_.erase(std::remove_if(vertices_.begin(), vertices_.end(),
                                   [id](const Attraction& a) { return a.id == id; }),
                    vertices_.end());
    // 和这个景点相连的道路也一起删
    edges_.erase(std::remove_if(edges_.begin(), edges_.end(),
                                [id](const Edge& e) { return e.from == id || e.to == id; }),
                 edges_.end());
    return true;
}

void Graph::setEdge(int u, int v, double distance) {
    removeEdge(u, v);
    edges_.push_back(Edge{u, v, distance});
}

bool Graph::removeEdge(int u, int v) {
    for (auto it = edges_.begin(); it != edges_.end(); ++it) {
        if ((it->from == u && it->to == v) || (it->from == v && it->to == u)) {
            edges_.erase(it);
            return true;
        }
    }
    return false;
}

// ---------------- 读数据文件 / 导入预设 ----------------

// 从一个解析好的 JSON 对象构建图，attractions/edges 逐条校验。
// 导入预设和读数据文件共用这一份校验逻辑
std::shared_ptr<Graph> loadGraphFromJson(const nlohmann::json& root, std::string* presetName) {
    auto g = std::make_shared<Graph>();
    try {
        const auto& attrs = root.at("attractions");
        const auto& roads = root.at("edges");
        if (!attrs.is_array() || !roads.is_array())
            throw ApiError("数据里 attractions/edges 应该是数组");

        for (const auto& j : attrs) {
            Attraction a = j.get<Attraction>();
            if (g->hasVertex(a.id))
                throw ApiError("景点 id 重复: " + std::to_string(a.id));
            if (a.posx < 0 || a.posx > 100 || a.posy < 0 || a.posy > 100)
                throw ApiError("posx/posy 必须在 0~100 之间: " + a.code);
            g->addAttraction(a);
        }
        for (const auto& j : roads) {
            Edge e = j.get<Edge>();
            if (!g->hasVertex(e.from))
                throw ApiError("道路端点不存在: " + std::to_string(e.from));
            if (!g->hasVertex(e.to))
                throw ApiError("道路端点不存在: " + std::to_string(e.to));
            if (e.from == e.to)
                throw ApiError("存在自环: " + std::to_string(e.from));
            if (!(e.distance > 0) || e.distance > 1e9)
                throw ApiError("道路里程非法: " + std::to_string(e.from) + "-" +
                               std::to_string(e.to));
            if (g->hasEdge(e.from, e.to))
                throw ApiError("存在重复道路: " + std::to_string(e.from) + "-" +
                               std::to_string(e.to));
            g->setEdge(e.from, e.to, e.distance);
        }
    } catch (const nlohmann::json::exception& e) {
        throw ApiError("字段缺失或类型错误: " + std::string(e.what()));
    }
    if (presetName) *presetName = root.value("name", root.value("city", "未命名"));
    return g;
}

std::shared_ptr<Graph> loadGraphFromFile(const std::string& path, std::string* presetName) {
    std::ifstream in(path);
    if (!in) throw ApiError("数据文件打开失败: " + path);

    nlohmann::json root;
    try {
        in >> root;
    } catch (const nlohmann::json::exception& e) {
        throw ApiError("数据文件解析失败: " + std::string(e.what()));
    }
    return loadGraphFromJson(root, presetName);
}

// ---------------- GraphStore ----------------

GraphStore::GraphStore(std::string dataFilePath)
    : dataFilePath_(std::move(dataFilePath)) {
    if (!dataFilePath_.empty()) reload();  // 空路径 = 单文件可执行模式，稍后注入内嵌数据
}

// 重新加载数据源：优先数据文件，没有则用内嵌默认数据。
// 先在旁边读好，成功了才换掉旧数据，文件有问题的话当前数据不受影响
void GraphStore::reload() {
    std::string name;
    std::shared_ptr<Graph> fresh;
    if (!dataFilePath_.empty()) {
        fresh = loadGraphFromFile(dataFilePath_, &name);
    } else if (!embeddedDefault_.empty()) {
        fresh = loadGraphFromJson(nlohmann::json::parse(embeddedDefault_), &name);
    } else {
        throw ApiError("未配置数据源");
    }
    std::lock_guard<std::mutex> lock(mutex_);
    graph_ = std::move(*fresh);
    name_ = std::move(name);
}

// 注入内嵌的默认数据并加载（单文件可执行模式）
void GraphStore::setEmbeddedDefault(const std::string& json) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        embeddedDefault_ = json;
    }
    reload();
}

// 导入预设：把整个图换成预设里的景点和道路
void GraphStore::importGraph(const nlohmann::json& preset) {
    std::string name;
    auto fresh = loadGraphFromJson(preset, &name);
    std::lock_guard<std::mutex> lock(mutex_);
    graph_ = std::move(*fresh);
    name_ = std::move(name);
}

std::string GraphStore::name() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return name_;
}

// 锁内拷一份出去，算法在快照上跑
std::shared_ptr<const Graph> GraphStore::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::make_shared<Graph>(graph_);
}

Attraction GraphStore::addAttraction(const Attraction& tpl) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (tpl.code.empty()) throw ApiError("代号不能为空");
    if (tpl.name.empty()) throw ApiError("名称不能为空");
    for (const auto& a : graph_.attractions()) {
        if (a.code == tpl.code) throw ApiError("代号已存在: " + tpl.code);
    }
    Attraction created = tpl;
    // id 接着目前最大的往后排
    created.id = graph_.attractions().empty() ? 0 : graph_.attractions().back().id + 1;
    graph_.addAttraction(created);
    return created;
}

void GraphStore::removeAttraction(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!graph_.hasVertex(id)) throw ApiError("景点不存在: " + std::to_string(id));
    graph_.removeAttraction(id);
}

Edge GraphStore::addEdge(const Edge& e) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!graph_.hasVertex(e.from)) throw ApiError("景点不存在: " + std::to_string(e.from));
    if (!graph_.hasVertex(e.to)) throw ApiError("景点不存在: " + std::to_string(e.to));
    if (e.from == e.to) throw ApiError("不允许自环: " + std::to_string(e.from));
    // 里程大于 0，也不能大得离谱，不然多条加起来会溢出成无穷大
    if (!(e.distance > 0) || e.distance > 1e9)
        throw ApiError("里程必须大于 0 且不超过 1e9");
    if (graph_.hasEdge(e.from, e.to)) {
        int lo = std::min(e.from, e.to), hi = std::max(e.from, e.to);
        throw ApiError("道路已存在: " + std::to_string(lo) + "-" + std::to_string(hi));
    }
    graph_.setEdge(e.from, e.to, e.distance);
    return e;
}

void GraphStore::removeEdge(int u, int v) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!graph_.hasEdge(u, v)) {
        int lo = std::min(u, v), hi = std::max(u, v);
        throw ApiError("道路不存在: " + std::to_string(lo) + "-" + std::to_string(hi));
    }
    graph_.removeEdge(u, v);
}

}  // namespace tour

// ---------------- 算法层用的工具（algo.hpp 里声明的） ----------------

namespace tour::algo {

// 邻接矩阵，行列顺序和 vertexIds() 一致，没有边是 INF
std::vector<std::vector<double>> buildWeightMatrix(const Graph& g) {
    int n = g.vertexCount();
    std::vector<int> ids = g.vertexIds();
    std::vector<std::vector<double>> m(n, std::vector<double>(n, INF));
    for (int i = 0; i < n; i++) {
        m[i][i] = 0;
        for (int j = i + 1; j < n; j++) {
            double w = g.weight(ids[i], ids[j]);
            if (w != Graph::NO_EDGE) {
                m[i][j] = w;
                m[j][i] = w;  // 无向图，对称
            }
        }
    }
    return m;
}

}  // namespace tour::algo