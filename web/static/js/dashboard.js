// 全局变量，用于存储 Vis.js 网络实例
let network = null;
let networkData = null; // 存储原始数据
let isRealTimeMode = false;
let pollingInterval = null;

// 页面加载完成后执行
document.addEventListener('DOMContentLoaded', function() {
    // 初始化空图或加载默认数据
    // loadGraph(); // 可选：自动加载默认数据
});

/**
 * 核心功能：从后端 API 获取数据并渲染
 */
function loadGraph() {
    const loading = document.getElementById('loading-overlay');
    if (loading) loading.style.display = 'flex';

    fetch('/api/graph')
        .then(response => {
            if (!response.ok) throw new Error("Network response was not ok");
            return response.json();
        })
        .then(data => {
            if (data.error) {
                alert("Error from backend: " + data.error);
                return;
            }
            
            networkData = data; // 保存数据以备后用
            drawNetworkTopology(data);
            updateThreatPanel(data.nodes);
            updateStatsPanel(data); // 更新统计信息
        })
        .catch(error => {
            console.error('Error fetching graph data:', error);
            alert("Failed to load graph data. See console for details.");
        })
        .finally(() => {
            if (loading) loading.style.display = 'none';
        });
}

/**
 * 使用 Vis.js 绘制网络拓扑图
 */
function drawNetworkTopology(data) {
    const container = document.getElementById('network-graph');

    // 1. 数据预处理：将后端 JSON 转换为 Vis.js 格式
    const nodes = data.nodes.map(node => {
        // 基础样式
        let color = getColorByGroup(node.group);
        let shape = 'dot';
        let size = 15 + Math.log(node.value + 1) * 3; // 对数缩放大小
        let label = node.label;
        
        // 可疑节点特殊样式 (红色菱形 + 闪烁边框效果模拟)
        if (node.suspicious === "true" || node.suspicious === true) {
            color = { 
                background: '#ff4d4d', 
                border: '#8b0000',
                highlight: { background: '#ff0000', border: '#8b0000' }
            };
            shape = 'diamond';
            size = size * 1.5; // 放大
            label = "⚠️ " + label; // 加个图标
        }

        return {
            id: node.id,
            label: label,
            title: generateTooltip(node), // 鼠标悬停提示
            value: node.value,
            group: node.group,
            color: color,
            shape: shape,
            size: size,
            shadow: { enabled: true, color: 'rgba(0,0,0,0.2)', size: 5, x: 2, y: 2 },
            // 自定义数据属性
            _location: node.location,
            _suspicious: node.suspicious
        };
    });

    const edges = data.edges.map(edge => ({
        from: edge.from,
        to: edge.to,
        width: 1 + Math.log(edge.value + 1) * 0.5, // 边粗细
        color: { color: '#bdc3c7', highlight: '#2c3e50', hover: '#34495e' },
        arrows: 'to', // 有向图
        title: `Traffic: ${edge.value} bytes`
    }));

    // 2. Vis.js 配置选项
    const options = {
        nodes: {
            borderWidth: 2,
            font: { color: '#343a40', size: 14, face: 'arial' }
        },
        edges: {
            smooth: { type: 'continuous' } // 平滑曲线
        },
        physics: {
            stabilization: true, // 启用稳定化（预计算布局）
            barnesHut: {
                gravitationalConstant: -2000,
                centralGravity: 0.3,
                springLength: 95,
                springConstant: 0.04,
                damping: 0.09
            }
        },
        interaction: { 
            hover: true, 
            tooltipDelay: 200,
            zoomView: true 
        },
        layout: {
            randomSeed: 42, // 保证每次布局一致
            improvedLayout: false // 禁用改进的布局算法，提高性能
        }
    };

    // 3. 实例化网络
    const visData = { nodes: new vis.DataSet(nodes), edges: new vis.DataSet(edges) };
    if (network) network.destroy(); // 清理旧实例
    network = new vis.Network(container, visData, options);

    // 4. 绑定事件：点击节点显示详情
    network.on("click", function (params) {
        if (params.nodes.length > 0) {
            const nodeId = params.nodes[0];
            const node = nodes.find(n => n.id === nodeId);
            showNodeDetails(node);
        }
    });
    
    // 绑定事件：悬停节点
    network.on("hoverNode", function (params) {
        container.style.cursor = 'pointer';
    });
    network.on("blurNode", function (params) {
        container.style.cursor = 'default';
    });
}

/**
 * 辅助：生成 Tooltip HTML
 */
function generateTooltip(node) {
    let status = (node.suspicious === true || node.suspicious === "true") 
        ? "<span style='color:red; font-weight:bold;'>SUSPICIOUS</span>" 
        : "<span style='color:green;'>Normal</span>";
    // 使用 Element 构造而非字符串，Vis.js 对 DOM Element 支持更好
    const container = document.createElement("div");
    container.style.padding = "10px";
    container.style.background = "#fff";
    container.style.border = "1px solid #ccc";
    container.innerHTML = `
        <strong>IP:</strong> ${node.label.replace("⚠️ ", "")}<br>
        <strong>Loc:</strong> ${node.location || "Unknown"}<br>
        <strong>Traffic:</strong> ${node.value} bytes<br>
        <strong>Status:</strong> ${status}
    `;
    return container;  
}

/**
 * 更新左侧“威胁情报”面板
 */
function updateThreatPanel(nodes) {
    const list = document.getElementById('threat-list');
    if (!list) return; // 添加空值检查
    list.innerHTML = ''; // 清空

    // 过滤出可疑节点
    const threats = nodes.filter(n => n.suspicious === "true" || n.suspicious === true);

    if (threats.length === 0) {
        list.innerHTML = '<li class="list-group-item text-muted"><i class="fas fa-check-circle text-success"></i> No threats detected. System is secure.</li>';
    } else {
        threats.forEach(t => {
            const ip = t.label.replace("⚠️ ", "");
            const li = document.createElement('li');
            li.className = 'list-group-item list-group-item-danger d-flex justify-content-between align-items-center';
            li.innerHTML = `
                <div>
                    <strong style="font-family:monospace;">${ip}</strong><br>
                    <small class="text-muted">${t.location || 'Unknown'}</small>
                </div>
                <span class="badge rounded-pill badge-danger-blink">Botnet?</span>
            `;
            // 点击列表项也能聚焦到节点
            li.style.cursor = 'pointer';
            li.onclick = () => focusOnNode(t.id);
            list.appendChild(li);
        });
    }
}

/**
 * 更新“节点详情”面板
 */
function showNodeDetails(node) {
    const details = document.getElementById('node-details');
    if (!details) return; // 添加空值检查
    const isSuspicious = (node._suspicious === true || node._suspicious === "true");
    
    details.innerHTML = `
        <h5 class="card-title">${node.label.replace("⚠️ ", "")}</h5>
        <h6 class="card-subtitle mb-2 text-muted">${node._location || "Unknown Location"}</h6>
        <hr>
        <p class="card-text">
            <strong>Total Traffic:</strong> ${node.value.toLocaleString()} bytes<br>
            <strong>Community ID:</strong> ${node.group}<br>
            <strong>Status:</strong> 
            <span class="${isSuspicious ? 'text-danger fw-bold' : 'text-success'}">
                ${isSuspicious ? '⚠️ THREAT DETECTED' : '✅ Normal'}
            </span>
        </p>
    `;
}

/**
 * 辅助：聚焦到特定节点
 */
function focusOnNode(nodeId) {
    network.focus(nodeId, {
        scale: 1.5,
        animation: { duration: 1000, easingFunction: 'easeInOutQuad' }
    });
    // 也可以手动触发详情显示
    const nodeData = networkData.nodes.find(n => n.id === nodeId);
    if(nodeData) {
        // 构造一个简单的对象传给 showNodeDetails，因为原始数据结构可能稍有不同
        showNodeDetails({
            label: nodeData.label,
            _location: nodeData.location,
            value: nodeData.value,
            group: nodeData.group,
            _suspicious: nodeData.suspicious
        });
    }
}

/**
 * 辅助：根据社区 ID 生成颜色
 */
function getColorByGroup(group) {
    // 一组柔和的配色方案
    const colors = [
        { background: '#4D80CC', border: '#2B4C7E' }, // Blue
        { background: '#50C878', border: '#2E7D42' }, // Emerald Green
        { background: '#E6E6FA', border: '#9370DB' }, // Lavender
        { background: '#FFD700', border: '#DAA520' }, // Gold
        { background: '#FF7F50', border: '#CD5C5C' }, // Coral
        { background: '#40E0D0', border: '#20B2AA' }, // Turquoise
        { background: '#FF69B4', border: '#C71585' }  // Hot Pink
    ];
    return colors[Math.abs(group) % colors.length];
}

/**
 * 可选：更新统计面板 (如果你在 index.html 里加了 Dashboard Cards)
 */
function updateStatsPanel(data) {
    // 简单的统计逻辑
    const totalNodes = data.nodes.length;
    const totalEdges = data.edges.length;
    // 假设 HTML 里有 id="stat-nodes" 的元素
    const statNodes = document.getElementById('stat-nodes');
    if (statNodes) statNodes.innerText = totalNodes;
}

/**
 * 处理文件上传
 */
async function uploadAndAnalyze() {
    const fileInput = document.getElementById('fileUpload');
    const file = fileInput.files[0];
    
    if (!file) {
        alert("Please select a file first!");
        return;
    }

    const formData = new FormData();
    formData.append('file', file);

    const btn = document.querySelector('button[onclick="loadGraph()"]'); // 获取按钮
    if(btn) btn.disabled = true;
    
    try {
        const response = await fetch('/upload', {
            method: 'POST',
            body: formData
        });
        
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const result = await response.json();
        if (result.error) {
            alert("Upload failed: " + result.error);
        } else {
            console.log("Upload success:", result.filename);
            // 上传成功后，立即加载图数据
            loadGraph();
        }
    } catch (error) {
        console.error("Upload error:", error);
        if (error.message.includes('ERR_UPLOAD_FILE_CHANGED')) {
            alert("Upload error: The file was changed during upload. Please try again.");
        } else {
            alert("Upload error: " + error.message);
        }
    } finally {
        if(btn) btn.disabled = false;
    }
}

// --- 新增功能函数 ---

/**
 * 1. 获取排序数据并弹窗显示
 */
function fetchSort(mode) {
    fetch(`/api/sort?mode=${mode}`)
        .then(res => res.json())
        .then(data => {
            if (data.error) { alert(data.error); return; }
            showModalTable(data, "Traffic Ranking (" + mode + ")");
        });
}

/**
 * 2. 路径查找与高亮
 */
function findPath(mode) {
    const src = document.getElementById('path-src').value.trim();
    const dst = document.getElementById('path-dst').value.trim();
    if (!src || !dst) { alert("Please enter both IPs"); return; }

    fetch(`/api/path?src=${src}&dst=${dst}&mode=${mode}`)
        .then(res => res.json())
        .then(data => {
            if (data.error) { alert(data.error); return; }
            
            // 在图上高亮路径
            if (data.path) {
                highlightPath(data.path);
                // 显示详细信息
                const info = mode === 'HOP' ? `Hops: ${data.hops}` : `Congestion: ${data.congestion}`;
                alert(`Path Found!\n${info}\nPath: ${data.path.join(' -> ')}`);
            }
        });
}

/**
 * 3. 星型结构检测
 */
function fetchStar() {
    fetch('/api/star')
        .then(res => res.json())
        .then(data => {
            // STAR 命令返回的是一个 List，如果空则说明没检测到
            if (data.status) { alert(data.status); return; } // No star detected
            
            // 构造显示的 HTML
            let html = '<div class="list-group">';
            data.forEach(star => {
                html += `
                    <div class="list-group-item">
                        <h6 class="mb-1 text-danger">Center: ${star.center}</h6>
                        <p class="mb-1 small">Connected Edges: ${star.edge_count}</p>
                        <small class="text-muted">${star.edges.join(', ')}</small>
                    </div>`;
                // 在图上高亮中心
                // network.selectNodes([star.center_id]); // 如果有ID的话
            });
            html += '</div>';
            showModal("Botnet / Star Topology Detected", html);
        });
}

/**
 * 4. 规则检查
 */
function checkRule() {
    const payload = {
        addr1: document.getElementById('rule-addr1').value,
        addr2: document.getElementById('rule-start').value,
        addr3: document.getElementById('rule-end').value,
        mode: document.getElementById('rule-mode').value
    };
    
    fetch('/api/rule', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify(payload)
    })
    .then(res => res.json())
    .then(data => {
        if (data.length === 0) { alert("No violations found."); return; }
        showModalTable(data, "Security Rule Violations");
    });
}

// --- 辅助函数 ---

// 在图上高亮路径
function highlightPath(ipPath) {
    if (!networkData) return;
    
    // 找到 IP 对应的 ID
    const pathIds = [];
    ipPath.forEach(ip => {
        const node = networkData.nodes.find(n => n.label === ip);
        if (node) pathIds.push(node.id);
    });

    // Vis.js 选择节点
    network.selectNodes(pathIds);
    network.fit({ nodes: pathIds, animation: true }); // 聚焦视角
}

// 显示通用表格模态框
function showModalTable(dataArray, title) {
    if (!dataArray || dataArray.length === 0) return;
    
    // 动态生成 Table Header
    const keys = Object.keys(dataArray[0]);
    let table = '<table class="table table-striped table-hover table-sm"><thead><tr>';
    keys.forEach(k => table += `<th>${k.toUpperCase()}</th>`);
    table += '</tr></thead><tbody>';
    
    // Table Body
    dataArray.forEach(row => {
        table += '<tr>';
        keys.forEach(k => table += `<td>${row[k]}</td>`);
        table += '</tr>';
    });
    table += '</tbody></table>';
    
    showModal(title, table);
}

// 显示模态框
function showModal(title, content) {
    const modalTitle = document.getElementById('modalTitle');
    const modalContent = document.getElementById('modalContent');
    const resultModal = document.getElementById('resultModal');
    if (!modalTitle || !modalContent || !resultModal) return; // 添加空值检查
    modalTitle.innerText = title;
    modalContent.innerHTML = content;
    const modal = new bootstrap.Modal(resultModal);
    modal.show();
}

// 还要记得更新一下 updateStatsPanel
function updateStatsPanel(data) {
    const statNodes = document.getElementById('stat-nodes');
    const statEdges = document.getElementById('stat-edges');
    if (statNodes) statNodes.innerText = data.nodes.length;
    if (statEdges) statEdges.innerText = data.edges.length;
}

/**
 * 启动/停止实时监控
 */
function toggleRealTime() {
    const btn = document.getElementById('btn-realtime');
    isRealTimeMode = !isRealTimeMode;
    
    if (isRealTimeMode) {
        // 开启模式
        btn.classList.remove('btn-outline-secondary');
        btn.classList.add('btn-danger', 'pulse-animation'); // 红色脉冲动画
        btn.innerText = "🔴 Live Monitoring ON";
        
        // 立即检查一次，然后每 3 秒检查一次，确保及时响应
        console.log('Real-time monitoring started');
        checkForUpdates();
        pollingInterval = setInterval(checkForUpdates, 3000);
        
    } else {
        // 关闭模式
        btn.classList.remove('btn-danger', 'pulse-animation');
        btn.classList.add('btn-outline-secondary');
        btn.innerText = "⚪ Live Monitoring OFF";
        
        if (pollingInterval) {
            clearInterval(pollingInterval);
            console.log('Real-time monitoring stopped');
        }
    }
}

/**
 * 检查后端是否有新数据
 */
async function checkForUpdates() {
    try {
        console.log('Checking for updates...');
        const response = await fetch('/api/status');
        
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const data = await response.json();
        
        if (data.has_new_data) {
            console.log("New data detected! Refreshing graph...");
            showToast("New traffic data detected. Refreshing...");
            loadGraph(); // 重新加载全图
        } else {
            console.log('No new data available');
        }
    } catch (e) {
        console.error("Polling error:", e);
    }
}

// 简单的提示框 (Toast)
function showToast(message) {
    // 输出到控制台
    console.log('Toast:', message);
    
    // 创建一个临时的提示元素
    const toast = document.createElement('div');
    toast.className = 'position-fixed top-20 right-20 bg-info text-white p-2 rounded shadow-lg z-50';
    toast.style.top = '20px';
    toast.style.right = '20px';
    toast.style.position = 'fixed';
    toast.style.zIndex = '1000';
    toast.style.padding = '10px';
    toast.style.borderRadius = '5px';
    toast.style.backgroundColor = '#17a2b8';
    toast.style.color = 'white';
    toast.style.boxShadow = '0 2px 5px rgba(0,0,0,0.2)';
    toast.innerText = message;
    
    document.body.appendChild(toast);
    
    // 2秒后移除
    setTimeout(() => {
        toast.style.opacity = '0';
        toast.style.transition = 'opacity 0.5s';
        setTimeout(() => {
            document.body.removeChild(toast);
        }, 500);
    }, 2000);
    
    // 检查并更新状态徽章
    const statusBadge = document.getElementById('live-status-badge');
    if(statusBadge) {
        statusBadge.innerText = "Updating...";
        setTimeout(() => statusBadge.innerText = "Live", 2000);
    }
}