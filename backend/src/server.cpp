#include "server.hpp"

#include "algo.hpp"
#include "api_common.hpp"

#include <climits>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using json = nlohmann::json;
using tour::ApiError;

// 统一用 HTTP 200 + {code, msg, data} 回复（API.md 第 1 节）
void sendJson(httplib::Response& res, const json& j) {
    res.status = 200;
    res.set_content(j.dump(), "application/json");
}

// 取 query 里的整数参数，格式不对或超出 int 范围都算参数错误
int getIntParam(const httplib::Request& req, const std::string& name) {
    if (!req.has_param(name)) throw ApiError("缺少必填参数: " + name);
    const std::string& s = req.get_param_value(name);
    long long v = 0;
    try {
        size_t pos = 0;
        v = std::stoll(s, &pos);
        if (pos != s.size()) throw std::invalid_argument("后面还有字符");
    } catch (...) {
        throw ApiError("参数非法: " + name + "=" + s);
    }
    if (v < INT_MIN || v > INT_MAX) throw ApiError("参数非法: " + name + "=" + s);
    return (int)v;
}

// 取请求体里的整数字段。get<int> 对超大的数会直接截断
// （比如 2^32 会变成 0），所以先用 long long 接住再查范围
int getIntField(const json& j, const std::string& key) {
    if (!j.contains(key) || !j.at(key).is_number_integer())
        throw ApiError("参数非法: " + key + " 必须为整数");
    long long v = j.at(key).get<long long>();
    if (v < INT_MIN || v > INT_MAX) throw ApiError("参数非法: " + key + " 超出整数范围");
    return (int)v;
}

std::string getStringField(const json& j, const std::string& key) {
    if (!j.contains(key) || !j.at(key).is_string())
        throw ApiError("参数非法: " + key + " 必须为字符串");
    return j.at(key).get<std::string>();
}

double getNumberField(const json& j, const std::string& key, double def) {
    if (!j.contains(key)) return def;
    if (!j.at(key).is_number()) throw ApiError("参数非法: " + key + " 必须为数字");
    return j.at(key).get<double>();
}

// 路径里的 id。正则 \d+ 保证是纯数字，但可能长到超出 int，
// 直接 stoi 会抛 out_of_range 变成"内部错误"，所以先查一下范围
int pathId(const httplib::Request& req, int group) {
    const std::string& s = req.matches[group].str();
    long long v = 0;
    try {
        v = std::stoll(s);
    } catch (...) {
        throw ApiError("参数非法: " + s);
    }
    if (v > INT_MAX) throw ApiError("参数非法: " + s);
    return (int)v;
}

const tour::Graph& requireVertex(const tour::Graph& g, int id) {
    if (!g.hasVertex(id)) throw ApiError("景点不存在: " + std::to_string(id));
    return g;
}

// start 参数可以不传，默认用 id 最小的景点
int resolveStart(const httplib::Request& req, const tour::Graph& g) {
    if (g.vertexCount() == 0) throw ApiError("图中没有景点");
    if (!req.has_param("start")) return g.idAt(0);
    int start = getIntParam(req, "start");
    requireVertex(g, start);
    return start;
}

// 矩阵转 JSON：INF 变 null，其它保留两位小数
json matrixJson(const std::vector<std::vector<double>>& m) {
    json out = json::array();
    for (const auto& row : m) {
        json r = json::array();
        for (double v : row) {
            if (v == tour::algo::INF)
                r.push_back(nullptr);
            else
                r.push_back(tour::round2(v));
        }
        out.push_back(r);
    }
    return out;
}

// 按 id 列表取景点名，4.8 和 4.12 两个矩阵接口都要用
json namesJson(const tour::Graph& g, const std::vector<int>& ids) {
    json names = json::array();
    for (int id : ids) {
        const tour::Attraction* a = g.findAttraction(id);
        if (!a) throw ApiError("景点不存在: " + std::to_string(id));
        names.push_back(a->name);
    }
    return names;
}

}  // namespace

namespace tour {

void setupServer(httplib::Server& svr, GraphStore& store, const ServerOptions& opt) {
    // CORS：开发时前端走 Vite 代理其实用不到，但分开部署就需要了。
    // Cache-Control: no-cache 让浏览器每次都重新校验 index.html，
    // 避免前端重新构建后浏览器还拿着旧入口 JS（会加载不到新分块）
    svr.set_default_headers({{"Access-Control-Allow-Origin", "*"},
                             {"Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS"},
                             {"Access-Control-Allow-Headers", "Content-Type"},
                             {"Cache-Control", "no-cache"}});
    svr.Options(R"(.*)", [](const httplib::Request&, httplib::Response& res) {
        res.status = 204;
    });

    // 访问日志
    svr.set_logger([](const httplib::Request& req, const httplib::Response& res) {
        std::fprintf(stderr, "%s %s -> %d\n", req.method.c_str(), req.path.c_str(),
                     res.status);
    });

    // 路由里抛出来的异常统一在这里转成 JSON，不然前端会收到裸的 500。
    // catch 顺序注意从具体到一般：ApiError 要在 std::exception 前面
    svr.set_exception_handler([](const httplib::Request&, httplib::Response& res,
                                 std::exception_ptr ep) {
        try {
            if (ep) std::rethrow_exception(ep);
        } catch (const ApiError& e) {
            sendJson(res, failJson(e.code, e.what()));
        } catch (const std::exception& e) {
            sendJson(res, failJson(1, std::string("服务器内部错误: ") + e.what()));
        } catch (...) {
            sendJson(res, failJson(1, "服务器内部错误"));
        }
    });

    // 没匹配到的路由：/api 开头的回 JSON 404；
    // 其它 GET 就当是前端的页面路由，回 index.html（不然刷新页面就 404 了）
    svr.set_error_handler([&opt](const httplib::Request& req, httplib::Response& res) {
        if (req.path.rfind("/api", 0) == 0) {
            res.status = 404;
            res.set_content(failJson(1, "接口不存在: " + req.method + " " + req.path).dump(),
                            "application/json");
            return;
        }
        if (req.method != "GET" || opt.staticDir.empty()) return;
        // 路径最后一段带点的当成静态文件请求（比如 /assets/xxx.js），不再回退
        auto lastSlash = req.path.find_last_of('/');
        auto lastDot = req.path.find_last_of('.');
        if (lastDot != std::string::npos && lastDot > lastSlash) return;

        std::ifstream in(opt.staticDir + "/index.html", std::ios::binary);
        std::string html((std::istreambuf_iterator<char>(in)),
                         std::istreambuf_iterator<char>());
        if (in) {
            res.status = 200;
            res.set_content(std::move(html), "text/html; charset=utf-8");
        }
    });

    // 有前端构建产物就把整个目录挂上（/api 路由不受影响）
    if (!opt.staticDir.empty() && std::filesystem::is_directory(opt.staticDir)) {
        svr.set_mount_point("/", opt.staticDir);
    }

    // ---- 4.1 健康检查 ----
    svr.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        sendJson(res, okJson(json{{"status", "ok"}}));
    });

    // ---- 4.2 景点列表 ----
    svr.Get("/api/attractions", [&store](const httplib::Request&, httplib::Response& res) {
        sendJson(res, okJson(store.snapshot()->attractions()));
    });

    // ---- 4.3 新增景点（id 由后端分配）----
    svr.Post("/api/attractions",
             [&store](const httplib::Request& req, httplib::Response& res) {
                 json j = parseJsonBody(req.body);
                 Attraction a;
                 a.code = getStringField(j, "code");
                 a.name = getStringField(j, "name");
                 if (j.contains("intro")) {
                     if (!j.at("intro").is_string())
                         throw ApiError("参数非法: intro 必须为字符串");
                     a.intro = j.at("intro").get<std::string>();
                 }
                 a.posx = getNumberField(j, "posx", 50.0);
                 a.posy = getNumberField(j, "posy", 50.0);
                 // 坐标限定在 0~100（前端表单也是这个范围）
                 if (a.posx < 0 || a.posx > 100)
                     throw ApiError("参数非法: posx 必须在 0~100 之间");
                 if (a.posy < 0 || a.posy > 100)
                     throw ApiError("参数非法: posy 必须在 0~100 之间");
                 sendJson(res, okJson(store.addAttraction(a)));
             });

    // ---- 4.4 删除景点（连着道路一起删）----
    svr.Delete(R"(/api/attractions/(\d+))",
               [&store](const httplib::Request& req, httplib::Response& res) {
                   store.removeAttraction(pathId(req, 1));
                   sendJson(res, okJson());
               });

    // ---- 4.5 道路列表 ----
    svr.Get("/api/edges", [&store](const httplib::Request&, httplib::Response& res) {
        sendJson(res, okJson(store.snapshot()->edges()));
    });

    // ---- 4.6 新增道路 ----
    svr.Post("/api/edges", [&store](const httplib::Request& req, httplib::Response& res) {
        json j = parseJsonBody(req.body);
        Edge e;
        e.from = getIntField(j, "from");
        e.to = getIntField(j, "to");
        e.distance = getNumberField(j, "distance", 0.0);
        sendJson(res, okJson(store.addEdge(e)));
    });

    // ---- 4.7 删除道路（u v 顺序不限）----
    svr.Delete(R"(/api/edges/(\d+)/(\d+))",
               [&store](const httplib::Request& req, httplib::Response& res) {
                   store.removeEdge(pathId(req, 1), pathId(req, 2));
                   sendJson(res, okJson());
               });

    // ---- 4.8 邻接矩阵 ----
    svr.Get("/api/graph/matrix", [&store](const httplib::Request&, httplib::Response& res) {
        auto snap = store.snapshot();
        std::vector<int> ids = snap->vertexIds();
        sendJson(res, okJson(json{{"ids", ids},
                                  {"names", namesJson(*snap, ids)},
                                  {"matrix", matrixJson(algo::buildWeightMatrix(*snap))}}));
    });

    // ---- 4.9 导游线路（DFS）----
    svr.Get("/api/route/tour", [&store](const httplib::Request& req, httplib::Response& res) {
        auto snap = store.snapshot();
        int start = resolveStart(req, *snap);
        algo::TourResult r = algo::buildTour(*snap, start);
        json edges = json::array();
        for (const auto& e : r.edges) edges.push_back(json{{"from", e.from}, {"to", e.to}});
        sendJson(res, okJson(json{{"startId", r.startId},
                                  {"walk", r.walk},
                                  {"edges", edges},
                                  {"unreachable", r.unreachable}}));
    });

    // ---- 4.10 回路检测 ----
    svr.Get("/api/route/cycle", [&store](const httplib::Request& req, httplib::Response& res) {
        auto snap = store.snapshot();
        int start = resolveStart(req, *snap);
        algo::CycleResult r = algo::detectCycle(*snap, start);
        sendJson(res, okJson(json{{"hasCycle", r.hasCycle},
                                  {"topoOrder", r.topoOrder},
                                  {"cyclicVertices", r.cyclicVertices},
                                  {"cycles", r.cycles}}));
    });

    // ---- 4.11 两点最短路径（algo 可选：dijkstra / dijkstra-heap / bellman-ford / spfa / floyd）----
    svr.Get("/api/path/shortest",
            [&store](const httplib::Request& req, httplib::Response& res) {
                auto snap = store.snapshot();
                int from = getIntParam(req, "from");
                int to = getIntParam(req, "to");
                requireVertex(*snap, from);
                requireVertex(*snap, to);
                std::string algoName =
                    req.has_param("algo") ? req.get_param_value("algo") : "dijkstra";
                if (!algo::isValidShortestAlgo(algoName))
                    throw ApiError("参数非法: algo=" + algoName);
                algo::ShortestResult r = algo::shortestPath(*snap, from, to, algoName);
                sendJson(res, okJson(json{{"reachable", r.reachable},
                                          {"distance", r.reachable ? json(round2(r.distance))
                                                                   : json(nullptr)},
                                          {"path", r.path}}));
            });

    // ---- 4.12 全源最短路（Floyd）----
    svr.Get("/api/path/all", [&store](const httplib::Request&, httplib::Response& res) {
        auto snap = store.snapshot();
        algo::AllShortestResult r = algo::allShortest(*snap);
        sendJson(res, okJson(json{{"ids", r.ids},
                                  {"names", namesJson(*snap, r.ids)},
                                  {"matrix", matrixJson(r.dist)}}));
    });

    // ---- 4.13 最小生成树 ----
    svr.Get("/api/roads/mst", [&store](const httplib::Request& req, httplib::Response& res) {
        std::string algoName = req.has_param("algo") ? req.get_param_value("algo") : "prim";
        if (algoName != "prim" && algoName != "kruskal")
            throw ApiError("参数非法: algo=" + algoName);
        auto snap = store.snapshot();
        algo::MstResult r = algo::minimumSpanningTree(*snap, algoName);
        json edges = json::array();
        for (const auto& e : r.edges) {
            edges.push_back(json{{"from", e.from}, {"to", e.to},
                                 {"distance", round2(e.distance)}});
        }
        sendJson(res, okJson(json{{"spanning", r.spanning},
                                  {"totalCost", round2(r.totalCost)},
                                  {"edges", edges}}));
    });

    // ---- 4.14 恢复默认数据 ----
    svr.Post("/api/graph/reset", [&store](const httplib::Request&, httplib::Response& res) {
        store.reload();
        sendJson(res, okJson());
    });

    // ---- 4.15 导出预设：把当前景点/道路导出成一个预设文件 ----
    svr.Get("/api/graph/export", [&store](const httplib::Request&, httplib::Response& res) {
        auto snap = store.snapshot();
        sendJson(res, okJson(json{{"name", store.name()},
                                  {"attractions", snap->attractions()},
                                  {"edges", snap->edges()}}));
    });

    // ---- 4.16 导入预设：用一个预设整体替换当前数据 ----
    svr.Post("/api/graph/import", [&store](const httplib::Request& req, httplib::Response& res) {
        json j = parseJsonBody(req.body);
        if (!j.contains("attractions") || !j.at("attractions").is_array())
            throw ApiError("参数非法: 缺少 attractions 数组");
        if (!j.contains("edges") || !j.at("edges").is_array())
            throw ApiError("参数非法: 缺少 edges 数组");
        store.importGraph(j);
        sendJson(res, okJson());
    });
}

}  // namespace tour