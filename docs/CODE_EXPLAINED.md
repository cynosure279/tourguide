# 后端代码说明

按文件过一遍后端的代码，讲清楚每部分在干什么、为什么这么写。
接口约定看 docs/API.md，整体设计看 docs/BACKEND_DESIGN.md。

## main.cpp —— 程序入口

启动流程很简单：先找数据文件（优先用命令行参数，否则用编译时记录的
项目根目录去找 data/hefei.json），然后构造 GraphStore（构造函数里
就会读文件，文件有问题直接启动失败退出），再把 Server 装配好监听
8080 端口。

firstExisting 就是把几个候选路径挨个试一遍，返回第一个存在的。找
frontend/dist 也是同一个函数，找不到就不托管静态站点，不影响接口。

## model.hpp —— 景点和道路

两个结构体 Attraction 和 Edge，字段和 API.md 第 2 节一一对应。

to_json / from_json 是 nlohmann/json 的约定写法：在自己的命名空间里
提供这两个函数之后，`json j = attraction` 和 `j.get<Attraction>()`
就都能用了。from_json 里 id/code/name 用 at()（必须有），intro 和
坐标用 value(..., 默认值)（可以没有），因为新增景点的请求体不一定
带全。

## api_common.hpp —— 统一响应

- ApiError：业务错误。路由里到处都可能报"景点不存在"，统一抛这个
  异常，最后在一个地方转成 JSON；
- okJson / failJson：{code, msg, data} 的两种情况；
- round2：保留两位小数。只在最后转 JSON 时用，算法内部一直是全精度，
  不然多段路各自取整再相加会有误差；
- parseJsonBody：请求体的统一入口，空 body 当 {} 处理。

## graph.hpp / graph.cpp —— 图和存储

Graph 是图的只读视图，存两个 vector：顶点按 id 升序（所以
findAttraction / indexOf 可以二分，idAt(0) 就是最小的 id），边按
加入顺序。neighbors() 现场扫一遍边表再按邻点 id 排序——DFS 要求
邻点按 id 升序访问，在这里排好，算法层就不用管了。

GraphStore 包着 Graph 加了把互斥锁。所有写操作（增删、reload）在
锁内"先检查再修改"；读操作用 snapshot() 在锁内拷一份整个图出去，
算法都在快照上跑。图就几十个点，拷贝没什么开销，换来的是代码简单
而且绝对不会读到改了一半的数据。

loadGraphFromFile 逐条校验（id 重复、端点不存在、自环、里程、重边），
任何一条不过就抛错，此时还没碰旧数据，所以 reset 传个坏文件也弄不坏
正在跑的系统。

buildWeightMatrix 在文件末尾，给算法层用：n×n 矩阵，对角线 0，无边
无穷大。Dijkstra 和 Floyd 都拿它当初始数据。

## algo/ —— 四个算法

### dfs.cpp

递归 DFS。用 std::function 是因为 lambda 要能调用到它自己。进入 u 时
push 一次，每递归一个邻点回来再 push 一次 u（原路返回也记一步），
walk 就是含回溯的行走序列。edges 用 set<pair> 按"第一次出现"去重，
insert 返回的第二个值是 bool，插进去说明是第一次见到这条边。

### cycle.cpp

内部用下标（0..n-1）而不是景点 id，数组可以直接拿来用，进出用
indexOf / idAt 换。先建导游线路图的入度表和邻接表（保持边的出现
顺序），Kahn 排序：入度 0 的按下标升序入队、FIFO 出队。没入序的
顶点标记出来，在这些顶点组成的子图里 DFS 找环：显式栈记当前路径，
posOf 记每个点在栈里的位置，走到"栈上"的顶点 v 时，从 posOf[v]
截到栈顶就是一条环。回溯时记得把 onStack 和 posOf 清掉，不然会
把交叉边误判成环。

### shortest.cpp

Dijkstra 是 O(n^2) 朴素版：每轮扫一遍找 dist 最小的未确定点（用
严格小于，并列时取到的是 id 较小的那个），松弛也用严格小于，这和
前端参考实现完全一样，路径还原出来才一致。不可达时 dist 一直是
无穷大，reachable=false。路径沿 prev 从终点走回起点再倒过来。

Floyd 三重循环，k 放在最外层（中转点只能用前 k 个，k 放里面结果
就错了）；无穷大参与加法之前先跳过。

### mst.cpp

Prim：treeOrder 按入树顺序记树内的点（对应前端用 Set 迭代时的顺序），
每轮按"入树顺序 × 边顺序"扫一遍挑最短的跨界边，严格小于（并列取先
遍历到的）。挑不到边说明这个连通分量完了，break 出来 spanning 就是
false——不连通的图得到的是生成森林。

Kruskal：stable_sort 只按里程排序（相等的保持原顺序，这点很重要，
普通 sort 不保证稳定），并查集的 find 写成递归 lambda，顺手做路径
压缩。凑够 n-1 条边就停。

两种算法遍历顺序都刻意对齐了前端 mst.js，这是结果能和模拟模式完全
一致的关键。

## server.cpp —— HTTP 层

结构：前面一堆小工具函数（取参数、取字段、矩阵转 JSON），setupServer
里先配 CORS / 日志 / 异常处理 / 静态托管，然后 14 个路由一个个注册。

几个值得说的点：

- 异常处理器把路由里抛的异常统一转 JSON，注意 catch 的顺序是从具体
  到一般（ApiError 要在 std::exception 前面，不然会被基类截住）；
- DELETE 路由用正则捕获组拿路径参数，\d+ 顺便保证了是纯数字。id 的
  转换先查范围再转，因为 stoi 遇到超长数字会抛异常，前端就会看到
  "内部错误"而不是"参数非法"；
- getIntField 里先 get<long long> 再查 int 范围。nlohmann 的
  get<int>() 对超出 int 的 JSON 整数是直接截断的（2^32 变成 0），
  不查的话一个非法请求就可能变成对 0 号景点的操作；
- 静态托管：把整个 dist 目录挂上去；对非 /api 的 GET 404，如果路径
  最后一段没有"."（不是请求静态文件），就回 index.html——前端是
  history 路由，刷新 /shortest 要靠这个回退；
- 路由 lambda 都是同一个套路：拿快照 → 解析校验参数 → 调算法 →
  拼 JSON。round2 和 null 的转换都收在路由层（matrixJson 一处管
  两个矩阵接口），算法层保持干净。

## CMakeLists.txt

源文件列表、C++17、include 路径就 include 和 third_party 两处，
链接 Threads（httplib 内部要开线程）。TOUR_PROJECT_ROOT 把
${CMAKE_SOURCE_DIR} 在编译时写死进代码，运行时找数据文件和前端
产物用的，所以二进制放哪都能找到资源。

## scripts/smoke.sh

curl 把 14 个接口挨个请求一遍，正常情况和报错情况都有，最后 reset
恢复。联调的时候跑一遍心里有底。

## third_party/

httplib.h（HTTP 服务器，用了路由注册、几个 set_xxx 回调、线程池）
和 nlohmann/json.hpp（JSON 解析序列化），都是 MIT 协议的头文件库，
连带 LICENSE 一起放进来了，这样别的机器不用装任何东西。
