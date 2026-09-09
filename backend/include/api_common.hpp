#pragma once

#include <nlohmann/json.hpp>

#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

namespace tour {

// 业务错误（参数不对、景点不存在之类），最后统一转成 {code!=0, msg} 返回
struct ApiError : std::runtime_error {
    int code;
    explicit ApiError(std::string msg, int code = 1)
        : std::runtime_error(std::move(msg)), code(code) {}
};

// 统一响应格式（API.md 第 1 节）
inline nlohmann::json okJson(nlohmann::json data = nullptr) {
    return nlohmann::json{{"code", 0}, {"msg", "ok"}, {"data", std::move(data)}};
}

inline nlohmann::json failJson(int code, const std::string& msg) {
    return nlohmann::json{{"code", code}, {"msg", msg}, {"data", nullptr}};
}

// 保留两位小数（和前端 Math.round(x*100)/100 一样）。
// 只在最后转 JSON 时用，算法内部保持全精度
inline double round2(double v) { return std::round(v * 100.0) / 100.0; }

// 解析请求体，不是合法 JSON 或不是对象就报错
inline nlohmann::json parseJsonBody(const std::string& body) {
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(body.empty() ? "{}" : body);
    } catch (const nlohmann::json::exception&) {
        throw ApiError("请求体不是合法 JSON");
    }
    if (!j.is_object()) throw ApiError("请求体必须是 JSON 对象");
    return j;
}

}  // namespace tour