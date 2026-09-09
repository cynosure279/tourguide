#!/bin/bash
# 接口冒烟测试：把 API.md 里的 14 个接口都请求一遍，正常情况和报错情况都有。
# 先启动后端再跑：./smoke.sh（默认 8080，可用 BASE 环境变量改）

B=${BASE:-http://localhost:8080}

echo "== 健康检查 =="
curl -s $B/api/health; echo

echo "== 景点列表（应有 24 个）=="
curl -s $B/api/attractions | python3 -c 'import json,sys; d=json.load(sys.stdin); print("code:", d["code"], "| 景点数:", len(d["data"]))'

echo "== 新增景点 =="
curl -s -X POST $B/api/attractions -H 'Content-Type: application/json' \
     -d '{"code":"A99","name":"测试景点","intro":"临时数据","posx":50,"posy":50}'; echo
echo "== 新增景点，代号重复（应报错）=="
curl -s -X POST $B/api/attractions -H 'Content-Type: application/json' \
     -d '{"code":"A01","name":"xxx"}'; echo

echo "== 删除刚加的景点（id 24）=="
curl -s -X DELETE $B/api/attractions/24; echo
echo "== 删除不存在的景点（应报错）=="
curl -s -X DELETE $B/api/attractions/99; echo

echo "== 道路列表（应有 48 条）=="
curl -s $B/api/edges | python3 -c 'import json,sys; d=json.load(sys.stdin); print("code:", d["code"], "| 道路数:", len(d["data"]))'

echo "== 新增道路 0-9 =="
curl -s -X POST $B/api/edges -H 'Content-Type: application/json' \
     -d '{"from":0,"to":9,"distance":5.5}'; echo
echo "== 新增重复道路（应报错）=="
curl -s -X POST $B/api/edges -H 'Content-Type: application/json' \
     -d '{"from":0,"to":1,"distance":3.0}'; echo
echo "== 删除道路 9-0（反着写）=="
curl -s -X DELETE $B/api/edges/9/0; echo
echo "== 删除不存在的道路（应报错）=="
curl -s -X DELETE $B/api/edges/3/5; echo

echo "== 邻接矩阵（看第一行）=="
curl -s $B/api/graph/matrix | python3 -c 'import json,sys; d=json.load(sys.stdin)["data"]; print(d["matrix"][0])'

echo "== 导游线路（DFS）=="
curl -s "$B/api/route/tour?start=0" | head -c 150; echo "..."
echo "== 回路检测 =="
curl -s "$B/api/route/cycle" | head -c 150; echo "..."
echo "== 最短路径 0 到 11 =="
curl -s "$B/api/path/shortest?from=0&to=11"; echo
echo "== Floyd 全源（看第一行）=="
curl -s $B/api/path/all | python3 -c 'import json,sys; d=json.load(sys.stdin)["data"]; print(d["matrix"][0])'
echo "== 最小生成树 prim / kruskal =="
curl -s "$B/api/roads/mst?algo=prim" | python3 -c 'import json,sys; d=json.load(sys.stdin)["data"]; print("prim totalCost:", d["totalCost"], "| spanning:", d["spanning"])'
curl -s "$B/api/roads/mst?algo=kruskal" | python3 -c 'import json,sys; d=json.load(sys.stdin)["data"]; print("kruskal totalCost:", d["totalCost"], "| spanning:", d["spanning"])'

echo "== 几个容易出错的输入（都应该报"参数非法"，而不是崩掉）=="
curl -s -X POST $B/api/edges -H 'Content-Type: application/json' \
     -d '{"from":4294967296,"to":1,"distance":1}'; echo
curl -s -X DELETE $B/api/attractions/99999999999999999999; echo
curl -s -X POST $B/api/edges -H 'Content-Type: application/json' \
     -d '{"from":0,"to":5,"distance":1e300}'; echo
curl -s -X POST $B/api/attractions -H 'Content-Type: application/json' \
     -d '{"code":"A99","name":"x","posx":200}'; echo

echo "== 恢复默认数据 =="
curl -s -X POST $B/api/graph/reset; echo
echo "== done =="