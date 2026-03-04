import csv
import random

def generate_csv(filename, rows=100):
    ips = [f"192.168.1.{i}" for i in range(1, 21)] # 20 IPs
    protocols = [6, 17, 1]
    
    with open(filename, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['Source', 'Destination', 'Protocol', 'SrcPort', 'DstPort', 'DataSize', 'Duration'])
        for _ in range(rows):
            src = random.choice(ips)
            dst = random.choice(ips)
            while src == dst: dst = random.choice(ips)
            writer.writerow([src, dst, random.choice(protocols), random.randint(1024, 65535), random.randint(1, 1024), random.randint(64, 5000), round(random.uniform(0.1, 5.0), 2)])

if __name__ == "__main__":
    generate_csv("../data/network_data_small.csv", 50)
    print("Generated data/network_data_small.csv")