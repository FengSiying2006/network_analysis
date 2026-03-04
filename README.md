# NetSentinel 网络流量分析系统

## 项目简介
NetSentinel 是一个高性能的网络流量分析系统，能够从 CSV 或 PCAP 文件中提取数据，构建网络拓扑图，进行流量分析和路径查找，并提供实时监控和可视化功能。

## 目录结构
```
network_analysis/
├── build/           # 编译输出目录
├── data/            # 数据文件目录
│   └── auto_captured/  # 自动抓包生成的数据
├── include/         # 头文件目录
├── python/          # Python 脚本目录
├── scripts/         # 辅助脚本目录
├── src/             # C 源代码目录
└── web/             # Web 应用目录
    ├── static/      # 静态资源
    │   ├── css/     # CSS 文件
    │   └── js/      # JavaScript 文件
    ├── templates/   # HTML 模板
    └── app.py       # Flask 应用入口
```

## 环境搭建

### 1. 安装依赖
```bash
# 安装 Python 依赖
pip install flask scapy

# 安装前端依赖（通过 CDN 加载，无需本地安装）
# Bootstrap, Vis.js, ECharts
```

### 2. 编译 C 代码
```bash
# 在项目根目录执行
gcc -Iinclude src/*.c -o build/program.exe
```

## 功能操作指南

### 主要功能实现情况（60分）

#### 1. 数据读取 (5分)
**操作指令**：
1. 准备 CSV 数据文件，格式为：`Source,Destination,Protocol,SrcPort,DstPort,DataSize,Duration`
2. 将文件保存为 `data/network_data.csv`
3. 运行 Web 应用：
   ```bash
   cd web
   python app.py
   ```
4. 在浏览器中访问 `http://localhost:5000`
5. 点击 "Load Full Graph" 按钮，系统会自动读取 CSV 文件并构建图结构

**核心代码**：`src/csv_reader.c` -> `read_csv()`

#### 2. 构建图 (13分)
**操作指令**：
1. 完成数据读取步骤
2. 查看图数据结构：
   - 打开 `include/graph_builder.h` 文件，查看 `Graph` 结构体定义
   - 关注 `csr_row_ptr`/`csr_col_idx`（CSR 格式）和 `csc_col_ptr`/`csc_row_idx`（CSC 格式）
3. 查看图构建过程：
   - 打开 `src/graph_builder.c` 文件，查看 `build_graph()` 函数

**核心代码**：`src/graph_builder.c` -> `build_graph()`

#### 3. 流量排序 (21分)
**操作指令**：
1. 完成数据读取步骤
2. 在 Web 界面点击 "Traffic Ranking" 展开菜单
3. 点击 "Top Traffic Nodes"：对所有节点的流量值进行排序输出
4. 点击 "Top HTTPS Nodes"：筛选包含 HTTPS 连接的节点，并按流量排序输出
5. 点击 "High Outbound Ratio"：筛选单向流量占比 > 80% 的节点，并按流量排序输出

**核心代码**：`src/sorting.c`

#### 4. 路径查找 (21分)
**操作指令**：
1. 完成数据读取步骤
2. 在 Web 界面点击 "Path Finding" 展开菜单
3. 在 "Source IP" 和 "Dest IP" 输入框中输入源 IP 和目标 IP
4. 点击 "Min Hops"：输出跳数最小的路径
5. 点击 "Min Congest"：输出拥塞最小的路径
6. 观察两条路径的区别（跳数少但拥塞，或绕路但通畅）

**核心代码**：`src/path.c`

### 扩展功能实现情况（10分）

#### 5. 星形结构 & 安全规则
**操作指令**：
1. 完成数据读取步骤
2. 在 Web 界面点击 "Advanced Detection" 展开菜单
3. 点击 "Detect Botnet (Star)"：查找图结构中存在的星型拓扑结构，输出中心节点和相连节点
4. 在 "Security Rule Check" 部分：
   - 输入目标 IP、起始 IP、结束 IP
   - 选择 "Deny" 或 "Allow" 模式
   - 点击 "Check Violations"：输出违反规则的会话信息

**核心代码**：
- 星形结构：`src/advanced_analysis.c` -> `star_detection()`
- 安全规则：`src/security.c`

### 升级功能实现情况（16分）

#### 6. 实时数据、图形化、可视化
**操作指令**：
1. **从 PCAP 文件提取数据**：
   ```bash
   python python/pcap_parser.py input.pcap output.csv
   ```
2. **实时监控**：
   - 启动自动抓包脚本：
     ```bash
     cd scripts
     python capture_scheduler.py
     ```
   - 在 Web 界面点击 "⚪ Live Monitoring OFF" 按钮开启实时监控
   - 观察数据自动刷新
3. **构建用户友好界面**：
   - 打开浏览器访问 `http://localhost:5000`
   - 体验手风琴菜单、模态框等交互元素
4. **可视化图结构**：
   - 在 Web 界面查看右侧的网络拓扑图
   - 拖动节点、缩放地图、鼠标悬停查看详情
   - 尝试输入 IP 和跳数，查看子图展示

**核心代码**：
- PCAP 解析：`python/pcap_parser.py`
- 前端可视化：`web/static/js/dashboard.js`

## 程序规范性检查

### 7. 程序规范性
- **模块化设计**：项目分为 `include` (头文件), `src` (C源码), `python` (脚本), `web` (前端) 四大目录
- **数据结构合理**：使用 CSR + CSC 压缩稀疏矩阵，内存占用少，缓存命中率高
- **注释完整**：所有关键函数均有详细注释

### 8. 设计特色和创意
- **全栈架构**：C (高性能计算) + Python Flask (Web后端) + Vis.js (前端可视化)
- **算法创新**：实现了社区发现和僵尸网络启发式检测
- **实时性**：自动抓包 + 实时轮询的端到端监控系统
- **地理位置增强**：结合 IP 地理位置库，展示节点的物理位置

## 运行示例

### 1. 基本分析
1. 编译 C 代码
2. 启动 Web 应用
3. 上传 CSV 或 PCAP 文件
4. 查看网络拓扑图
5. 使用各项分析功能

### 2. 实时监控
1. 编译 C 代码
2. 启动自动抓包脚本
3. 启动 Web 应用
4. 开启实时监控
5. 观察数据实时更新

## 注意事项

1. **网络接口配置**：在 `scripts/capture_scheduler.py` 中，需要根据实际网络接口名称修改 `INTERFACE` 变量
2. **文件路径**：确保所有文件路径正确，特别是数据文件和可执行文件的路径
3. **权限**：确保有足够的权限读取和写入文件
4. **依赖**：确保安装了所有必要的依赖包

## 技术栈

- **后端**：C, Python, Flask
- **前端**：HTML, CSS, JavaScript, Bootstrap, Vis.js, ECharts
- **数据处理**：Scapy (PCAP 解析)
- **可视化**：Vis.js (网络拓扑图), ECharts (数据图表)

## 项目亮点

1. **高性能**：使用 CSR/CSC 压缩稀疏矩阵，处理百万级数据
2. **实时性**：自动抓包和实时数据更新
3. **可视化**：交互式网络拓扑图，直观展示网络结构
4. **安全性**：内置安全规则检查和僵尸网络检测
5. **用户友好**：直观的 Web 界面，操作简单

## 作者

- **名称**：NetSentinel 开发团队
- **日期**：2026-03-04