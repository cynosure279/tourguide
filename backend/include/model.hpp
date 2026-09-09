#pragma once

#include <nlohmann/json.hpp>

#include <string>

namespace tour {

// 景点（API.md 第 2 节）。id 是顶点号；坐标只给前端画图用，不参与算法
struct Attraction {
    int id = 0;
    std::string code;    // 代号，比如 "A01"
    std::string name;
    std::string intro;
    double posx = 50.0;  // 0~100
    double posy = 50.0;
};

// 道路：无向带权边
struct Edge {
    int from = 0;
    int to = 0;
    double distance = 0.0;  // 里程 km
};

// 给 nlohmann/json 用的序列化函数，定义在 graph.cpp 里
void to_json(nlohmann::json& j, const Attraction& a);
void from_json(const nlohmann::json& j, Attraction& a);  // intro/posx/posy 可以缺省

void to_json(nlohmann::json& j, const Edge& e);
void from_json(const nlohmann::json& j, Edge& e);

}  // namespace tour