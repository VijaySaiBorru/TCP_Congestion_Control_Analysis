
# TCP Congestion Control Analysis in Wireless Networks

## Project Overview
This project compares performance of TCP NewReno, Cubic, and Vegas in high packet loss wireless environments using NS-3 simulations. It includes:

- `tcp_comparison_algorithms.cc` - NS-3 simulation code
- `plot_analysis.py` - Python visualization script
- `Computer_Networks.pdf` - Project report with analysis

## Prerequisites

### Linux OS (Ubuntu 20.04+ recommended)

### NS-3 Dependencies:
```bash
sudo apt install git build-essential libsqlite3-dev libboost-dev libgsl-dev \
libxml2 libxml2-dev libgtk-3-dev cmake python3 python3-pip
```

### Python Packages:
```bash
pip install matplotlib numpy
```

## Installation & Setup

### 1. Install NS-3
```bash
git clone https://gitlab.com/nsnam/ns-3-dev.git
cd ns-3-dev
./ns3 configure --build-profile=debug --enable-examples --enable-tests
./ns3 build
```

### 2. Add Simulation Files
```bash
cp tcp_comparison_algorithms.cc scratch/
cp tcp_comparison_algorithms.cc src/example/scratch/
```

## Running the Simulation

### 1. Build and Execute
```bash
./ns3 build
./ns3 run scratch/tcp_comparison_algorithms
```

### 2. Generated Files
The simulation will create:

- `cwnd-node-0.txt` (NewReno)
- `cwnd-node-1.txt` (Cubic)
- `cwnd-node-2.txt` (Vegas)
- `tcp-flows.xml` (Flow statistics)
- Console output showing PHY layer packet drops

## Visualizing Results

### 1. Generate Plots
```bash
python3 plot_analysis.py
```

### 2. Output
- `cwnd_plot.png` - Congestion window evolution graph
- Terminal output showing throughput/delay statistics

## Expected Output

### Flow Statistics:
```
Flow 1 (10.1.1.1 -> 10.1.1.4)
  Throughput: 6.46 Mbps | Avg Delay: 0.081s
Flow 3 (10.1.1.2 -> 10.1.1.4)  
  Throughput: 6.07 Mbps | Avg Delay: 0.111s
Flow 5 (10.1.1.3 -> 10.1.1.4)
  Throughput: 0.81 Mbps | Avg Delay: 0.002s
PHY layer drops: 1429 packets
```

## Key Implementation Details

### Simulation Configuration

Wireless topology with 3 mobile nodes & 1 AP

High packet loss induced via:
```cpp
channel.AddPropagationLoss("ns3::LogDistancePropagationLossModel",
                          "Exponent", DoubleValue(2.7));
channel.AddPropagationLoss("ns3::NakagamiPropagationLossModel");
```

TCP parameter tuning:
```cpp
Config::Set("/NodeList/2/$ns3::TcpVegas/Alpha", UintegerValue(2));
Config::Set("/NodeList/2/$ns3::TcpVegas/Beta", UintegerValue(4));
```

### Visualization Script

Reads `cwnd-node-*.txt` files

Uses matplotlib for plotting:
```python
plt.plot(time, cwnd, label=labels[i], color=colors[i])
plt.title("Congestion Window Evolution")
plt.savefig("cwnd_plot.png")
```

## Troubleshooting

**Build Errors:**
- Ensure all NS-3 dependencies are installed
- Verify file paths in scratch directory

**Missing Data Files:**
- Check simulation completed successfully
- Confirm write permissions in execution directory

**Plotting Issues:**
```bash
pip install --upgrade matplotlib
```
- Verify Python 3.6+ is used

## Team
- Borru Vijay Sai
- C V Harshith Reddy
- N Sai Charan Reddy
