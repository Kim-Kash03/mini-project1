import pexpect
import sys
import matplotlib.pyplot as plt
import os

def run_scheduler(scheduler_type):
    print(f"Running QEMU with {scheduler_type}...")
    # Clean and build with specific scheduler
    os.system('make clean')
    os.system(f'make SCHEDULER={scheduler_type}')
    
    # Spawn qemu
    child = pexpect.spawn(f'make qemu SCHEDULER={scheduler_type}', encoding='utf-8')
    child.expect(r'\$ ', timeout=30)
    
    print("Running schedulertest...")
    child.sendline('schedulertest')
    
    # Collect output until test finishes
    output = []
    
    while True:
        try:
            line = child.readline()
            if not line: break
            line = line.strip()
            if line:
                output.append(line)
            if "Scheduler test done!" in line:
                break
        except pexpect.TIMEOUT:
            print("Timeout reading from QEMU")
            break
            
    # Exit QEMU
    child.sendline('\x01x')
    child.close()
    
    return output

def parse_traces(output):
    traces = []
    stats = []
    for line in output:
        if line.startswith("TRC "):
            parts = line.split()
            # TRC ticks pid q
            if len(parts) >= 4:
                try:
                    traces.append({
                        'tick': int(parts[1]),
                        'pid': int(parts[2]),
                        'queue': int(parts[3])
                    })
                except Exception as e:
                    pass
        elif line.startswith("STAT "):
            # STAT pid X name Y TT Z RT Z WT Z
            parts = line.split()
            if len(parts) >= 11:
                try:
                    stats.append({
                        'pid': int(parts[2]),
                        'name': parts[4],
                        'TT': int(parts[6]),
                        'RT': int(parts[8]),
                        'WT': int(parts[10]),
                    })
                except:
                    pass
    return traces, stats

def generate_graph(traces):
    if not traces:
        print("No trace data to plot!")
        return
        
    plt.figure(figsize=(10, 6))
    
    # Group by PID
    pids = list(set([t['pid'] for t in traces]))
    pids.sort()
    
    colors = ['r', 'g', 'b', 'c', 'm', 'y', 'k']
    for i, pid in enumerate(pids):
        pid_traces = [t for t in traces if t['pid'] == pid]
        ticks = [t['tick'] for t in pid_traces]
        queues = [t['queue'] for t in pid_traces]
        plt.scatter(ticks, queues, label=f'PID {pid}', color=colors[i % len(colors)], s=40)
    
    plt.yticks([0, 1, 2, 3])
    plt.xlabel('Time (ticks)')
    plt.ylabel('Queue ID')
    plt.title('MLFQ Process Queues over Time')
    plt.legend()
    plt.grid(True, axis='y')
    
    # Draw Boost lines based on 48 tick cyclic intervals
    all_ticks = [t['tick'] for t in traces]
    if len(all_ticks) > 0:
        max_tick = max(all_ticks) if all_ticks else 200
        for b in range(48, max_tick + 1, 48):
            plt.axvline(x=b, color='red', linestyle='--', alpha=0.5)
            plt.text(b, 3.1, 'BOOST', color='red', ha='center', va='bottom', fontsize=10, weight='bold')
            
    # Add watermark
    plt.text(0.5, 0.5, 'kimaya.kashyap', fontsize=40, color='gray', ha='center',
             va='center', alpha=0.2, transform=plt.gca().transAxes)
    
    plt.savefig('mlfq_plot.png', bbox_inches='tight')
    print("Saved plot to mlfq_plot.png")

if __name__ == "__main__":
    mlfq_out = run_scheduler('MLFQ')
    traces, mlfq_stats = parse_traces(mlfq_out)
    
    generate_graph(traces)
    
    rr_out = run_scheduler('RR')
    _, rr_stats = parse_traces(rr_out)
    
    fifo_out = run_scheduler('FIFO')
    _, fifo_stats = parse_traces(fifo_out)
    
    # Print comparison
    print("\n--- SCHEDULER STATISTICS ---")
    
    def print_stat(name, stats):
        print(f"\n{name} Stats:")
        if not stats: 
            print("  No stats collected")
            return
        
        avg_tt = sum([s['TT'] for s in stats]) / len(stats)
        avg_rt = sum([s['RT'] for s in stats]) / len(stats)
        avg_wt = sum([s['WT'] for s in stats]) / len(stats)
        print(f"  Average TT: {avg_tt:.2f} ticks")
        print(f"  Average RT: {avg_rt:.2f} ticks")
        print(f"  Average WT: {avg_wt:.2f} ticks")
        
    print_stat('FIFO', fifo_stats)
    print_stat('RR', rr_stats)
    print_stat('MLFQ', mlfq_stats)

    with open('stats.txt', 'w') as f:
        f.write("FIFO\n")
        for s in fifo_stats: f.write(str(s) + "\n")
        f.write("\nRR\n")
        for s in rr_stats: f.write(str(s) + "\n")
        f.write("\nMLFQ\n")
        for s in mlfq_stats: f.write(str(s) + "\n")

    print("\nAll done!")
