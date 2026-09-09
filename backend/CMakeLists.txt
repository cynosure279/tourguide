# C++ 后端。httplib 和 nlohmann/json 都是头文件库，
# 放在 third_party 里，别的机器不用装任何东西

add_executable(tour-backend
    src/main.cpp
    src/graph.cpp
    src/server.cpp
    src/algo/dfs.cpp
    src/algo/cycle.cpp
    src/algo/shortest.cpp
    src/algo/mst.cpp
)

target_compile_features(tour-backend PRIVATE cxx_std_17)

target_include_directories(tour-backend PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

# 把项目根目录的绝对路径写进程序里，
# 运行时靠它找 data/hefei.json 和 frontend/dist
target_compile_definitions(tour-backend PRIVATE
    TOUR_PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
)

find_package(Threads REQUIRED)
target_link_libraries(tour-backend PRIVATE Threads::Threads)

# 编译选项按编译器区分（MSVC 不认识 -Wall）
target_compile_options(tour-backend PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
)

# 单文件可执行版：把前端构建产物与默认数据内嵌进二进制。
# 需要先构建前端（frontend/dist 必须存在），适合发布；日常开发不必开。
option(EMBED_FRONTEND "内嵌前端构建产物与默认数据（生成自包含可执行文件）" OFF)
if(EMBED_FRONTEND)
  if(NOT EXISTS "${CMAKE_SOURCE_DIR}/frontend/dist/index.html")
    message(FATAL_ERROR "EMBED_FRONTEND=ON 需要先构建前端：cd frontend && npm ci && npm run build")
  endif()
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  set(EMBED_HEADER "${CMAKE_CURRENT_BINARY_DIR}/embedded_assets.h")
  add_custom_command(OUTPUT "${EMBED_HEADER}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/embed.py"
            --dist "${CMAKE_SOURCE_DIR}/frontend/dist"
            --data "${CMAKE_SOURCE_DIR}/data/hefei.json"
            --out "${EMBED_HEADER}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/embed.py"
    COMMENT "内嵌前端构建产物与默认数据"
    VERBATIM)
  target_sources(tour-backend PRIVATE "${EMBED_HEADER}")
  target_include_directories(tour-backend PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
  target_compile_definitions(tour-backend PRIVATE EMBED_FRONTEND=1)
endif()