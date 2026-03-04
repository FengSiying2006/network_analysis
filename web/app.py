import os
import json
import subprocess
import random
from flask import Flask, render_template, request, jsonify
from werkzeug.utils import secure_filename

# 全局变量：记录上次数据更新的时间戳
LAST_UPDATE_TIME = 0

app = Flask(__name__)
# 使用绝对路径，确保无论从哪个目录运行都能正确找到文件
import os
app.config['UPLOAD_FOLDER'] = os.path.join(os.path.dirname(__file__), '..', 'data')
app.config['C_EXECUTABLE'] = os.path.join(os.path.dirname(__file__), '..', 'build', 'program.exe')

@app.route('/api/status')
def get_status():
    """
    前端每隔几秒调用一次，检查数据是否有更新
    """
    global LAST_UPDATE_TIME
    # 检查 auto_captured 目录下的 network_data.csv 文件
    csv_file = os.path.join(os.path.dirname(__file__), '..', 'data', 'auto_captured', 'network_data.csv')
    
    if not os.path.exists(csv_file):
        # 如果 auto_captured 目录下的文件不存在，检查主 data 目录下的文件
        csv_file = os.path.join(app.config['UPLOAD_FOLDER'], 'network_data.csv')
        if not os.path.exists(csv_file):
            return jsonify({'status': 'no_data', 'timestamp': 0})
        
    # 获取文件的最后修改时间
    file_mtime = os.path.getmtime(csv_file)
    
    # 如果文件比上次记录的时间新，说明有新数据！
    has_new_data = file_mtime > LAST_UPDATE_TIME
    
    return jsonify({
        'status': 'ok',
        'has_new_data': has_new_data,
        'timestamp': file_mtime
    })

# 模拟 IP 地理位置数据库 (实际项目请使用 geoip2 库)
def get_geo_info(ip):
    # 简单的哈希模拟，保证同一个IP每次返回相同位置
    # 真实代码：reader = geoip2.database.Reader('GeoLite2-City.mmdb'); response = reader.city(ip)
    hash_val = sum(ord(c) for c in ip)
    locations = [
        "Beijing, China", "Shanghai, China", "New York, USA", 
        "London, UK", "Tokyo, Japan", "Moscow, Russia"
    ]
    return locations[hash_val % len(locations)]

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/upload', methods=['POST'])
def upload_file():
    if 'file' not in request.files:
        return jsonify({'error': 'No file part'})
    file = request.files['file']
    if file.filename == '':
        return jsonify({'error': 'No selected file'})
    
    # 保存上传的文件到临时位置
    temp_filepath = os.path.join(app.config['UPLOAD_FOLDER'], secure_filename(file.filename))
    file.save(temp_filepath)
    
    # 目标 CSV 文件路径
    target_filepath = os.path.join(app.config['UPLOAD_FOLDER'], 'network_data.csv')
    
    # 检查文件扩展名，如果是 PCAP 文件，调用 pcap_parser.py 进行转换
    if temp_filepath.lower().endswith('.pcap') or temp_filepath.lower().endswith('.pcapng'):
        try:
            # 构建 pcap_parser.py 的路径
            pcap_parser_path = os.path.join(os.path.dirname(__file__), '..', 'python', 'pcap_parser.py')
            
            # 调用 pcap_parser.py 转换 PCAP 文件
            result = subprocess.run(
                ['python', pcap_parser_path, temp_filepath, target_filepath],
                capture_output=True,
                text=True,
                encoding='utf-8'
            )
            
            if result.returncode != 0:
                # 转换失败，返回错误信息
                os.remove(temp_filepath)  # 清理临时文件
                return jsonify({'error': f'PCAP parsing failed: {result.stderr}'})
            
            # 转换成功，清理临时文件
            os.remove(temp_filepath)
            
            return jsonify({'message': 'PCAP file parsed and uploaded successfully', 'filename': 'network_data.csv'})
        except Exception as e:
            # 处理异常，清理临时文件
            if os.path.exists(temp_filepath):
                os.remove(temp_filepath)
            return jsonify({'error': f'Error parsing PCAP file: {str(e)}'})
    else:
        # 如果不是 PCAP 文件，直接重命名为 network_data.csv
        try:
            # 先检查目标文件是否存在，如果存在则删除
            if os.path.exists(target_filepath):
                os.remove(target_filepath)
            # 重命名临时文件为 network_data.csv
            os.rename(temp_filepath, target_filepath)
            return jsonify({'message': 'File uploaded successfully', 'filename': 'network_data.csv'})
        except Exception as e:
            # 处理异常，清理临时文件
            if os.path.exists(temp_filepath):
                os.remove(temp_filepath)
            return jsonify({'error': f'Error saving file: {str(e)}'})

@app.route('/api/graph', methods=['GET'])
def get_graph():
    # 引入全局变量，用于记录最后一次成功服务数据的时间
    # 这是实现“实时监控”的关键：记录我们当下展示的数据版本
    global LAST_UPDATE_TIME
    
    # 1. 确定数据文件路径
    # 首先检查 auto_captured 目录下的 network_data.csv 文件
    csv_file = os.path.join(os.path.dirname(__file__), '..', 'data', 'auto_captured', 'network_data.csv')
    
    # 2. 如果文件不存在，检查主 data 目录下的文件
    if not os.path.exists(csv_file):
        csv_file = os.path.join(app.config['UPLOAD_FOLDER'], 'network_data.csv')
        if not os.path.exists(csv_file):
            return jsonify({'error': 'No data file found. Please upload or capture data.'})

    # 3. 构建 C 语言执行命令: ./program.exe network_data.csv EXPORT_JSON
    cmd = [app.config['C_EXECUTABLE'], csv_file, "EXPORT_JSON"]
    
    try:
        # 4. 执行 C 程序
        process = subprocess.run(cmd, capture_output=True, text=True, encoding='utf-8')
        
        # 检查 C 程序是否报错
        if process.returncode != 0:
            return jsonify({'error': f"C Engine Failed: {process.stderr}"})
        
        # 5. 解析 JSON 输出
        graph_data = json.loads(process.stdout)
        
        # 6. [Python 层增强] 添加地理位置信息
        # 遍历节点，根据 IP (label) 计算或查询地理位置
        if 'nodes' in graph_data:
            for node in graph_data['nodes']:
                node['location'] = get_geo_info(node['label'])
            
        # 7. [实时监控关键步骤] 更新时间戳
        # 获取当前 CSV 文件的最后修改时间，并更新全局变量
        # 这样 /api/status 接口就能通过比较这个时间，判断是否有更新的文件生成
        LAST_UPDATE_TIME = os.path.getmtime(csv_file)
            
        return jsonify(graph_data)
        
    except json.JSONDecodeError:
        return jsonify({'error': 'Invalid JSON output from C engine', 'raw': process.stdout})
    except Exception as e:
        return jsonify({'error': str(e)})
    
@app.route('/api/sort', methods=['GET'])
def sort_traffic():
    mode = request.args.get('mode', 'ALL') # ALL, HTTPS, UNI
    cmd_map = {
        'ALL': 'SORT_ALL',
        'HTTPS': 'SORT_HTTPS',
        'UNI': 'SORT_UNI'
    }
    command = cmd_map.get(mode, 'SORT_ALL')
    
    # 首先检查 auto_captured 目录下的 network_data.csv 文件
    csv_file = os.path.join(os.path.dirname(__file__), '..', 'data', 'auto_captured', 'network_data.csv')
    if not os.path.exists(csv_file):
        csv_file = os.path.join(app.config['UPLOAD_FOLDER'], 'network_data.csv')
    
    cmd = [app.config['C_EXECUTABLE'], csv_file, command]
    return run_c_command(cmd)

@app.route('/api/path', methods=['GET'])
def find_path():
    src = request.args.get('src')
    dst = request.args.get('dst')
    mode = request.args.get('mode', 'HOP') # HOP, CONGEST
    
    command = 'PATH_HOP' if mode == 'HOP' else 'PATH_CONGEST'
    
    # 首先检查 auto_captured 目录下的 network_data.csv 文件
    csv_file = os.path.join(os.path.dirname(__file__), '..', 'data', 'auto_captured', 'network_data.csv')
    if not os.path.exists(csv_file):
        csv_file = os.path.join(app.config['UPLOAD_FOLDER'], 'network_data.csv')
    
    cmd = [app.config['C_EXECUTABLE'], csv_file, command, src, dst]
    return run_c_command(cmd)

@app.route('/api/star', methods=['GET'])
def find_star():
    # 首先检查 auto_captured 目录下的 network_data.csv 文件
    csv_file = os.path.join(os.path.dirname(__file__), '..', 'data', 'auto_captured', 'network_data.csv')
    if not os.path.exists(csv_file):
        csv_file = os.path.join(app.config['UPLOAD_FOLDER'], 'network_data.csv')
    
    cmd = [app.config['C_EXECUTABLE'], csv_file, "STAR"]
    return run_c_command(cmd)

@app.route('/api/rule', methods=['POST'])
def check_rule():
    data = request.json
    # RULE <addr1> <addr2> <addr3> <allow|deny>
    
    # 首先检查 auto_captured 目录下的 network_data.csv 文件
    csv_file = os.path.join(os.path.dirname(__file__), '..', 'data', 'auto_captured', 'network_data.csv')
    if not os.path.exists(csv_file):
        csv_file = os.path.join(app.config['UPLOAD_FOLDER'], 'network_data.csv')
    
    cmd = [app.config['C_EXECUTABLE'], csv_file, "RULE", 
           data['addr1'], data['addr2'], data['addr3'], data['mode']]
    return run_c_command(cmd)

# 辅助函数：统一执行 C 命令并返回
def run_c_command(cmd):
    try:
        process = subprocess.run(cmd, capture_output=True, text=True, encoding='utf-8')
        if process.returncode != 0:
            return jsonify({'error': process.stderr})
        # C程序输出的是 JSON 字符串，我们解析后再返回 JSON 对象
        try:
            result = json.loads(process.stdout)
            return jsonify(result)
        except json.JSONDecodeError:
             return jsonify({'error': 'Invalid JSON from C engine', 'raw': process.stdout})
    except Exception as e:
        return jsonify({'error': str(e)})

# ... (后面的代码保持不变) ...
if __name__ == '__main__':
    app.run(debug=True, port=5000)