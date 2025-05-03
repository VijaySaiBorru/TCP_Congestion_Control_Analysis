import matplotlib.pyplot as plt

colors = ['tab:blue', 'tab:orange', 'tab:green']
labels = ['Node 1 (NewReno)', 'Node 2 (Cubic)', 'Node 3 (Vegas)']

for i, node_id in enumerate([0, 1, 2]):
    time, cwnd = [], []
    filename = f"cwnd-node-{node_id}.txt"
    with open(filename, 'r') as file:
        for line in file:
            parts = line.strip().split()
            if len(parts) == 2:
                time.append(float(parts[0]))
                cwnd.append(float(parts[1]))
    plt.plot(time, cwnd, label=labels[i], color=colors[i])

plt.title("Congestion Window Evolution")
plt.xlabel("Time (s)")
plt.ylabel("Congestion Window (KB)")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("cwnd_plot.png")
