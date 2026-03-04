# 网络流量分析系统 - 工程经验总结

## 1. 常用指令

### 1.1 Windows 命令行
- **查看目录内容**：`dir`（替代 Linux 的 `ls`）
- **复制文件**：`copy source destination`（替代 Linux 的 `cp`）
- **切换目录**：`cd directory`
- **创建目录**：`mkdir directory`
- **删除文件**：`del file`
- **运行 Python 脚本**：`python script.py`
- **运行 C 程序**：`program.exe`

### 1.2 Git 命令
- **查看状态**：`git status`
- **添加文件**：`git add file`
- **提交更改**：`git commit -m "message"`
- **推送到远程**：`git push`
- **设置远程仓库**：`git remote set-url origin git@github.com:username/repo.git`（SSH 协议）

### 1.3 项目相关命令
- **启动 Web 服务器**：`cd web && python app.py`
- **启动自动抓包脚本**：`cd scripts && python capture_scheduler.py`
- **编译 C 代码**：`gcc -Iinclude src/*.c -o build/program.exe`
- **解析 PCAP 文件**：`python python/pcap_parser.py input.pcap output.csv`

## 2. 功能与工具库

### 2.1 核心功能
- **数据读取**：从 CSV 文件读取网络流量数据
- **图构建**：构建网络拓扑图，使用 CSR/CSC 压缩稀疏矩阵
- **流量排序**：根据不同条件（总流量、HTTPS 流量、单向流量占比）排序节点
- **路径查找**：基于 BFS 的最小跳数路径和基于 Dijkstra 的最小拥塞路径
- **星形结构检测**：检测潜在的僵尸网络
- **安全规则检查**：检查 IP 范围规则违规
- **实时监控**：自动抓包、解析和更新数据
- **Web 可视化**：交互式网络拓扑图展示

### 2.2 工具库
- **C 语言**：标准库、自定义数据结构
- **Python**：
  - `scapy`：解析 PCAP 文件
  - `flask`：Web 后端框架
  - `werkzeug`：文件上传处理
- **前端**：
  - `bootstrap`：UI 组件和样式
  - `vis-network`：网络拓扑图可视化
  - `echarts`：数据图表

## 3. 项目文件格式规范

### 3.1 目录结构
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

### 3.2 文件命名规范
- **C 源文件**：`snake_case.c`（例如 `graph_builder.c`）
- **头文件**：`snake_case.h`（例如 `graph_builder.h`）
- **Python 文件**：`snake_case.py`（例如 `pcap_parser.py`）
- **JavaScript 文件**：`camelCase.js`（例如 `dashboard.js`）
- **HTML 文件**：`kebab-case.html`（例如 `index.html`）
- **CSS 文件**：`kebab-case.css`（例如 `style.css`）

### 3.3 数据文件格式
- **CSV 文件**：
  - 格式：`Source,Destination,Protocol,SrcPort,DstPort,DataSize,Duration`
  - 示例：`192.168.1.1,10.0.0.1,6,80,443,1024,0.123456`
- **PCAP 文件**：标准网络抓包格式，可由 Wireshark 或 `scapy` 生成

## 4. 工程经验

### 4.1 跨平台开发
- **路径处理**：使用 `os.path.join()` 构建路径，避免硬编码路径分隔符
- **命令执行**：使用 `subprocess.run()` 执行外部命令，确保跨平台兼容性
- **文件存在性检查**：使用 `os.path.exists()` 检查文件是否存在

### 4.2 性能优化
- **数据结构**：使用 CSR/CSC 压缩稀疏矩阵存储图结构，提高内存利用率和访问速度
- **算法选择**：根据问题特性选择合适的算法（BFS 用于最小跳数，Dijkstra 用于最小拥塞）
- **内存管理**：使用 `realloc()` 实现动态内存分配，避免内存浪费
- **布局算法**：在 Vis.js 中禁用 `improvedLayout` 以提高大型图的渲染性能

### 4.3 错误处理
- **前端错误处理**：添加空值检查，避免因 DOM 元素不存在导致的错误
- **后端错误处理**：使用 try-except 捕获异常，返回友好的错误信息
- **文件上传错误**：处理文件类型错误、文件大小限制等异常情况
- **网络错误处理**：添加网络请求的错误处理，提高应用的健壮性

### 4.4 实时监控实现
- **自动抓包**：使用 `scapy` 库实时抓包，定期保存为 PCAP 文件
- **数据转换**：将 PCAP 文件转换为 CSV 文件，方便后续处理
- **轮询机制**：前端定期检查后端数据是否更新，实现实时数据刷新
- **状态管理**：使用全局变量记录数据更新时间戳，避免重复加载数据

### 4.5 安全性
- **文件上传安全**：使用 `secure_filename()` 处理上传文件名，避免路径遍历攻击
- **输入验证**：对用户输入进行验证，防止恶意输入
- **IP 地址处理**：将 IP 地址转换为整数进行存储和比较，提高安全性和效率

### 4.6 开发工具配置
- **VS Code 配置**：
  - `c_cpp_properties.json`：配置 C/C++ 插件，指定包含路径和编译器
  - `tasks.json`：配置构建任务，指定编译命令和参数
- **依赖管理**：使用 `requirements.txt` 管理 Python 依赖

## 5. 示例应用

### 5.1 实时监控示例
**场景**：监控网络流量变化，及时发现异常

**步骤**：
1. 启动自动抓包脚本：
   ```cmd
   cd scripts
   python capture_scheduler.py
   ```
2. 启动 Web 服务器：
   ```cmd
   cd web
   python app.py
   ```
3. 在浏览器中访问 `http://localhost:5000`
4. 点击 "⚪ Live Monitoring OFF" 按钮开启实时监控
5. 观察网络拓扑图自动更新，显示实时流量数据

### 5.2 PCAP 文件分析示例
**场景**：分析已有的 PCAP 抓包文件

**步骤**：
1. 运行 PCAP 解析脚本：
   ```cmd
   python python/pcap_parser.py input.pcap output.csv
   ```
2. 在 Web 界面上传生成的 CSV 文件
3. 查看网络拓扑图，分析流量模式
4. 使用 "Traffic Ranking" 功能查看流量排名
5. 使用 "Path Finding" 功能查找路径

### 5.3 安全规则检查示例
**场景**：检查网络流量是否违反安全规则

**步骤**：
1. 在 Web 界面的 "Advanced Detection" 部分
2. 输入目标 IP、起始 IP、结束 IP
3. 选择 "Deny" 模式
4. 点击 "Check Violations" 按钮
5. 查看违规的会话记录

## 6. 总结

本项目是一个全栈网络流量分析系统，融合了 C 语言的高性能计算、Python 的灵活性和 Web 技术的可视化能力。通过本项目的开发，我们积累了以下工程经验：

- **跨平台开发**：解决了 Windows 和 Linux 命令差异，确保代码在不同平台上正常运行
- **性能优化**：使用高效的数据结构和算法，处理大规模网络数据
- **实时监控**：实现了从抓包到可视化的端到端实时监控系统
- **错误处理**：添加了全面的错误处理机制，提高应用的健壮性
- **安全性**：实现了基本的安全措施，保护系统和数据安全
- **开发工具配置**：合理配置开发工具，提高开发效率

这些经验不仅适用于网络分析系统，也可以应用于其他类似的工程项目中，为我们的软件开发能力打下了坚实的基础。