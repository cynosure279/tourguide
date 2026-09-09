#include "api_common.hpp"
#include "graph.hpp"
#include "server.hpp"

#include <httplib.h>

#ifdef EMBED_FRONTEND
#include "embedded_assets.h"
#endif

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

// 编译时由 CMake 注入的项目根目录，找不到文件时再按相对路径试
#ifndef TOUR_PROJECT_ROOT
#define TOUR_PROJECT_ROOT ""
#endif

// 返回第一个真实存在的路径，都不存在就返回空串
std::string firstExisting(std::vector<std::string> paths) {
    for (auto& p : paths) {
        if (!p.empty() && std::filesystem::exists(p)) return p;
    }
    return "";
}

#ifdef EMBED_FRONTEND
// 单文件可执行模式：把内嵌的前端产物解压到临时目录，返回该目录路径
std::string extractEmbeddedFrontend() {
    namespace fs = std::filesystem;
    fs::path dir = fs::temp_directory_path() / "tour-backend-www";
    for (const auto& file : tour::embed::files()) {
        fs::path full = dir / (file.path + 1);  // 跳过开头的 '/'
        fs::create_directories(full.parent_path());
        std::ofstream out(full, std::ios::binary);
        out.write(reinterpret_cast<const char*>(file.data),
                  static_cast<std::streamsize>(file.size));
    }
    return dir.string();
}
#endif

}  // namespace

int main(int argc, char** argv) {
    using namespace tour;

    // 数据文件：优先用命令行参数指定的
    std::string dataFile = argc > 1 ? argv[1] : firstExisting(
        {TOUR_PROJECT_ROOT "/data/hefei.json", "data/hefei.json", "../data/hefei.json",
         "../../data/hefei.json"});
#ifdef EMBED_FRONTEND
    const bool embeddedData = dataFile.empty();  // 找不到磁盘数据 → 用内嵌的
#endif
#ifndef EMBED_FRONTEND
    if (dataFile.empty()) {
        std::cerr << "找不到数据文件 data/hefei.json（也可以用命令行参数指定）\n";
        return 1;
    }
#endif

    ServerOptions opt;
#ifdef EMBED_FRONTEND
    if (embeddedData) {
        // 内嵌模式：把前端产物解压到临时目录后托管，实现单文件分发
        opt.staticDir = extractEmbeddedFrontend();
    } else
#endif
    {
        // 有前端构建产物的话顺便把静态站点也托管了
        opt.staticDir = firstExisting(
            {TOUR_PROJECT_ROOT "/frontend/dist", "frontend/dist", "../frontend/dist"});
    }

    try {
        GraphStore store(dataFile);

#ifdef EMBED_FRONTEND
        if (embeddedData) store.setEmbeddedDefault(tour::embed::defaultData());
#endif

        httplib::Server svr;
        setupServer(svr, store, opt);

        auto snap = store.snapshot();
        std::cout << "景区旅游信息管理系统后端已启动\n"
                  << "  接口地址 : http://localhost:" << opt.port << "/api\n"
                  << "  数据来源 : "
                  << (dataFile.empty() ? std::string("内嵌默认数据") : dataFile)
                  << "（" << snap->vertexCount() << " 个景点 / " << snap->edges().size()
                  << " 条道路）\n";
        if (!opt.staticDir.empty()) {
            std::cout << "  静态站点 : http://localhost:" << opt.port << "/\n";
        }
        std::cout << "  Ctrl+C 退出" << std::endl;

        if (!svr.listen(opt.bind, opt.port)) {
            std::cerr << "监听 " << opt.port << " 端口失败（是不是已经被占用了？）\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "启动失败: " << e.what() << "\n";
        return 1;
    }
    return 0;
}