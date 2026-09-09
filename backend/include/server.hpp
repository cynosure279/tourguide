#pragma once

#include "graph.hpp"

#include <httplib.h>

#include <string>

namespace tour {

struct ServerOptions {
    std::string bind = "0.0.0.0";
    int port = 8080;
    std::string staticDir;  // 不为空就把这个目录当前端静态站点托管
};

// 把 API.md 里的 14 个接口都注册到 svr 上，顺带配好 CORS、异常处理、
// 访问日志和静态文件托管。路由里可以放心抛 ApiError，会被统一转成 JSON 响应
void setupServer(httplib::Server& svr, GraphStore& store, const ServerOptions& opt);

}  // namespace tour