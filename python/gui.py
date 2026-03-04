import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import subprocess
import json
import threading
import os

# Configuration
C_EXECUTABLE = os.path.join("..", "build", "program.exe")  # Adjust path as needed
DEFAULT_CSV = os.path.join("..", "data", "network_data.csv")

class NetworkApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Network Traffic Analysis System")
        self.root.geometry("900x600")
        
        self.csv_path = tk.StringVar(value=DEFAULT_CSV)
        
        self.create_widgets()

    def create_widgets(self):
        # --- File Selection Frame ---
        file_frame = ttk.LabelFrame(self.root, text="Data Source")
        file_frame.pack(fill="x", padx=10, pady=5)
        
        ttk.Entry(file_frame, textvariable=self.csv_path, width=80).pack(side="left", padx=5)
        ttk.Button(file_frame, text="Browse CSV", command=self.browse_csv).pack(side="left", padx=5)

        # --- Control Panel Frame ---
        control_frame = ttk.Frame(self.root)
        control_frame.pack(fill="x", padx=10, pady=5)
        
        # Tabs for different functions
        self.notebook = ttk.Notebook(control_frame)
        self.notebook.pack(fill="both", expand=True)
        
        # Tab 1: Traffic Sorting
        self.tab_sort = ttk.Frame(self.notebook)
        self.notebook.add(self.tab_sort, text="Traffic Analysis")
        ttk.Button(self.tab_sort, text="Sort All Traffic", command=lambda: self.run_c_command("SORT_ALL")).pack(side="left", padx=10, pady=10)
        ttk.Button(self.tab_sort, text="Sort HTTPS", command=lambda: self.run_c_command("SORT_HTTPS")).pack(side="left", padx=10, pady=10)
        ttk.Button(self.tab_sort, text="High Outbound Ratio", command=lambda: self.run_c_command("SORT_UNI")).pack(side="left", padx=10, pady=10)

        # Tab 2: Path Finding
        self.tab_path = ttk.Frame(self.notebook)
        self.notebook.add(self.tab_path, text="Path Finding")
        ttk.Label(self.tab_path, text="Source IP:").grid(row=0, column=0, padx=5, pady=5)
        self.src_ip = ttk.Entry(self.tab_path)
        self.src_ip.grid(row=0, column=1, padx=5, pady=5)
        
        ttk.Label(self.tab_path, text="Dest IP:").grid(row=0, column=2, padx=5, pady=5)
        self.dst_ip = ttk.Entry(self.tab_path)
        self.dst_ip.grid(row=0, column=3, padx=5, pady=5)
        
        ttk.Button(self.tab_path, text="Find Min Hops", command=self.find_hops).grid(row=0, column=4, padx=5)
        ttk.Button(self.tab_path, text="Find Min Congestion", command=self.find_congestion).grid(row=0, column=5, padx=5)

        # Tab 3: Advanced (Star & Rules)
        self.tab_adv = ttk.Frame(self.notebook)
        self.notebook.add(self.tab_adv, text="Advanced Detection")
        ttk.Button(self.tab_adv, text="Detect Star Topology", command=lambda: self.run_c_command("STAR")).pack(side="left", padx=10, pady=10)
        # (Rule inputs can be added here similarly)

        # --- Output Area ---
        self.tree = ttk.Treeview(self.root, columns=("col1", "col2"), show="headings")
        self.tree.heading("col1", text="Attribute")
        self.tree.heading("col2", text="Value")
        self.tree.pack(fill="both", expand=True, padx=10, pady=10)

    def browse_csv(self):
        filename = filedialog.askopenfilename(filetypes=[("CSV files", "*.csv")])
        if filename:
            self.csv_path.set(filename)

    def find_hops(self):
        s, d = self.src_ip.get(), self.dst_ip.get()
        if s and d: self.run_c_command("PATH_HOP", [s, d])

    def find_congestion(self):
        s, d = self.src_ip.get(), self.dst_ip.get()
        if s and d: self.run_c_command("PATH_CONGEST", [s, d])

    def run_c_command(self, cmd, args=[]):
        # Clear previous results
        for i in self.tree.get_children():
            self.tree.delete(i)
            
        csv = self.csv_path.get()
        if not os.path.exists(csv):
            messagebox.showerror("Error", "CSV file not found!")
            return

        command_line = [C_EXECUTABLE, csv, cmd] + args
        
        def run_thread():
            try:
                # Run C executable
                process = subprocess.run(command_line, capture_output=True, text=True, encoding='utf-8') # Added encoding
                
                if process.returncode != 0:
                    self.root.after(0, lambda: messagebox.showerror("C Error", f"Process failed:\n{process.stderr}"))
                    return

                output = process.stdout.strip()
                try:
                    data = json.loads(output)
                    self.root.after(0, lambda: self.display_results(data))
                except json.JSONDecodeError:
                    self.root.after(0, lambda: messagebox.showerror("JSON Error", f"Invalid JSON output:\n{output}"))
            except Exception as e:
                self.root.after(0, lambda: messagebox.showerror("System Error", str(e)))

        threading.Thread(target=run_thread, daemon=True).start()

    def display_results(self, data):
        # Dynamic Treeview columns based on data
        if isinstance(data, list) and len(data) > 0:
            keys = list(data[0].keys())
            self.tree["columns"] = keys
            for k in keys:
                self.tree.heading(k, text=k.upper())
            
            for item in data:
                self.tree.insert("", "end", values=[item.get(k, "") for k in keys])
        
        elif isinstance(data, dict):
            self.tree["columns"] = ("Key", "Value")
            self.tree.heading("Key", text="Key")
            self.tree.heading("Value", text="Value")
            for k, v in data.items():
                self.tree.insert("", "end", values=(k, str(v)))

if __name__ == "__main__":
    root = tk.Tk()
    app = NetworkApp(root)
    root.mainloop()