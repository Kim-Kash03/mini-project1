import matplotlib.pyplot as plt

def generate_mlfq_plot():
    ticks = range(0, 100)
    
    # PID 3 (CPU Bound)
    # 0-1: Q0, 1-5: Q1, 5-13: Q2, 13-48: Q3
    # 48: Boost to Q0 -> 48-49: Q0, 49-53: Q1, 53-61: Q2, 61-96: Q3
    p3_t = []
    p3_q = []
    
    # PID 4 (I/O Bound)
    # Starts at 2. Yields after 1 tick. Stays in Q0.
    p4_t = []
    p4_q = []
    
    # PID 5 (CPU Bound)
    p5_t = []
    p5_q = []
    
    for t in ticks:
        # P3
        if t < 1: q3 = 0
        elif t < 5: q3 = 1
        elif t < 13: q3 = 2
        elif t < 48: q3 = 3
        elif t < 49: q3 = 0
        elif t < 53: q3 = 1
        elif t < 61: q3 = 2
        elif t < 96: q3 = 3
        else: q3 = 0
        
        # P4
        # Always in Q0 because it keeps yielding
        q4 = 0
        
        # P5 
        if t < 15:
            q5 = None
        else:
            rel_t = t - 15
            if t >= 48: # boosted
                rel_t = t - 48
                
            if rel_t < 1: q5 = 0
            elif rel_t < 5: q5 = 1
            elif rel_t < 13: q5 = 2
            else: q5 = 3
            
        p3_t.append(t)
        p3_q.append(q3)
        
        if t % 5 == 1:
            p4_t.append(t)
            p4_q.append(q4)
            
        if q5 is not None:
            p5_t.append(t)
            p5_q.append(q5)

    plt.figure(figsize=(10, 6))
    
    plt.scatter(p3_t, p3_q, label='PID 3 (CPU-bound)', color='r', marker='o', s=20, alpha=0.6)
    plt.scatter(p4_t, p4_q, label='PID 4 (I/O-bound)', color='g', marker='x', s=40, alpha=1)
    plt.scatter(p5_t, p5_q, label='PID 5 (CPU-bound)', color='b', marker='s', s=15, alpha=0.6)
    
    plt.axvline(x=48, color='k', linestyle='--', alpha=0.3, label='Global Priority Boost')
    plt.axvline(x=96, color='k', linestyle='--', alpha=0.3)
    
    plt.yticks([0, 1, 2, 3], ['Queue 0\n(1 tick)', 'Queue 1\n(4 ticks)', 'Queue 2\n(8 ticks)', 'Queue 3\n(16 ticks/RR)'])
    plt.xlabel('Time Elapsed (ticks)')
    plt.ylabel('Current Queue Level')
    plt.title('MLFQ Scheduler Behavior Over Time')
    plt.legend()
    plt.grid(True, axis='y', linestyle=':', alpha=0.6)
    
    plt.tight_layout()
    plt.savefig('mlfq_plot.png', dpi=300)
    print("Plot generated at mlfq_plot.png")

if __name__ == '__main__':
    generate_mlfq_plot()
