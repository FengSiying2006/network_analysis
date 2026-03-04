import sys
import csv
from scapy.all import rdpcap, IP, TCP, UDP
from collections import defaultdict

def pcap_to_csv(pcap_file, output_csv):
    """
    Parses a PCAP file and aggregates flows into a CSV.
    Flow Key: (Source IP, Dest IP, Protocol, Src Port, Dst Port)
    """
    try:
        packets = rdpcap(pcap_file)
    except FileNotFoundError:
        print(f"Error: File {pcap_file} not found.")
        return False
    except Exception as e:
        print(f"Error reading pcap: {e}")
        return False

    flows = defaultdict(lambda: {'size': 0, 'start': float('inf'), 'end': 0})

    print(f"Parsing {len(packets)} packets...")

    for pkt in packets:
        if IP in pkt:
            src = pkt[IP].src
            dst = pkt[IP].dst
            proto = pkt[IP].proto
            size = len(pkt)
            time = float(pkt.time)

            sport = 0
            dport = 0

            if TCP in pkt:
                sport = pkt[TCP].sport
                dport = pkt[TCP].dport
            elif UDP in pkt:
                sport = pkt[UDP].sport
                dport = pkt[UDP].dport
            
            # Key for aggregation
            key = (src, dst, proto, sport, dport)
            
            flow = flows[key]
            flow['size'] += size
            if time < flow['start']: flow['start'] = time
            if time > flow['end']: flow['end'] = time

    print(f"Aggregated into {len(flows)} unique flows.")

    with open(output_csv, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['Source', 'Destination', 'Protocol', 'SrcPort', 'DstPort', 'DataSize', 'Duration'])
        
        for (src, dst, proto, sport, dport), data in flows.items():
            duration = max(0.001, data['end'] - data['start']) # Avoid 0 duration
            writer.writerow([src, dst, proto, sport, dport, data['size'], f"{duration:.6f}"])

    print(f"Successfully saved to {output_csv}")
    return True

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python pcap_parser.py <input.pcap> <output.csv>")
    else:
        pcap_to_csv(sys.argv[1], sys.argv[2])