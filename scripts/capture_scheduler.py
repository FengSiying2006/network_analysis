import time
import subprocess
import datetime
import os
from scapy.all import sniff, wrpcap

# 配置
INTERFACE = "WLAN" # 根据你的网卡名称修改 (Windows上可能是 'Wi-Fi', 'Ethernet' 或 'WLAN')
DURATION = 5        # 每次抓包持续时间 (秒)，为了测试设为5秒
INTERVAL = 10       # 间隔 10 秒，为了测试设为10秒
DATA_DIR = "../data/auto_captured"

if not os.path.exists(DATA_DIR):
    os.makedirs(DATA_DIR)

def capture_traffic():
    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    pcap_file = os.path.join(DATA_DIR, f"traffic_{timestamp}.pcap")
    csv_file = os.path.join(DATA_DIR, "network_data.csv") # 覆盖主文件以便 Web 实时读取

    print(f"[{timestamp}] Starting capture on {INTERFACE} for {DURATION}s...")
    
    try:
        # 使用 scapy 抓包
        packets = sniff(iface=INTERFACE, count=0, timeout=DURATION)
        wrpcap(pcap_file, packets)
        print(f"  -> Saved pcap to {pcap_file}")
        
        # 调用转换脚本 (假设你之前的 python/pcap_parser.py 存在)
        # 注意路径调整
        parser_script = "../python/pcap_parser.py"
        subprocess.run(["python", parser_script, pcap_file, csv_file], check=True)
        print(f"  -> Converted to CSV and updated system data.")
        
    except Exception as e:
        print(f"Error during capture: {e}")

if __name__ == "__main__":
    print("🚀 Auto-Capture Scheduler Started (Ctrl+C to stop)")
    while True:
        capture_traffic()
        print(f"Sleeping for {INTERVAL} seconds...")
        time.sleep(INTERVAL)