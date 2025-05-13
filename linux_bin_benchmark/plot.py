import pandas as pd
import matplotlib.pyplot as plt
import re

method_names = {
    4: "SYCL A* algoritmus",
    3: "CBS hybrid SYCL+CPU",
    2: "A* CPU (multi-thread)",
    1: "A* CPU (1 thread)",
    0: "CBS CPU"
}

def parse_filename(name):
    match = re.search(r"map_(\d+)x(\d+)_a(\d+)_o(\d+)", name)
    if not match:
        return pd.Series([None]*4)
    return pd.Series([int(match[1]), int(match[2]), int(match[3]), int(match[4])])

# Load only required columns
df = pd.read_csv("results.csv", usecols=["File", "Method", "TimeRun(ms)"])
df["MethodName"] = df["Method"].map(method_names)
df[["width", "height", "agents", "obstacles"]] = df["File"].apply(parse_filename)
df.dropna(inplace=True)

# Calculate obstacle percentage
df["obstacle_pct"] = (df["obstacles"] / (df["width"] * df["height"]) * 100).round(2)

# GRAF 1 – 1024x1024, 63/64 agents, o=256
target_files = ["map_1024x1024_a64_o256.data", "map_1024x1024_a63_o256.data"]
subset1 = df[df["File"].isin(target_files)]

plt.figure(figsize=(10, 6))
for fname in target_files:
    data = subset1[subset1["File"] == fname]
    plt.plot(data["MethodName"], data["TimeRun(ms)"], marker='o', label=fname)
plt.title("Výkon metód – 1024x1024 mapa, 63/64 agentov, 256 prekážok")
plt.ylabel("Čas výpočtu (ms)")
plt.xticks(rotation=30)
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.savefig("graf_1_1024_a63a64_o256.png")

# GRAF 2 – 16x16, agents = {3, 4, ..., 64}, o=0
agents_set = [3, 4, 7, 8, 15, 16, 31, 32, 63, 64]
subset2 = df[(df["width"] == 16) & (df["height"] == 16) & (df["obstacles"] == 0) & (df["agents"].isin(agents_set))]

plt.figure(figsize=(10, 6))
for method in subset2["MethodName"].unique():
    data = subset2[subset2["MethodName"] == method].sort_values("agents")
    plt.plot(data["agents"], data["TimeRun(ms)"], marker='o', label=method)
plt.title("Výkon metód – 16x16 mapa, rôzny počet agentov, 0 prekážok")
plt.xlabel("Počet agentov")
plt.ylabel("Čas výpočtu (ms)")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.savefig("graf_2_16x16_var_agents.png")

# GRAF 3 – 1024x1024, 63 agentov, rôzne prekážky
obstacles_set = [0, int(1024*1/16), int(1024*2/16), int(1024*3/16), 256]
subset3 = df[(df["width"] == 1024) & (df["agents"] == 63) & (df["obstacles"].isin(obstacles_set))]

plt.figure(figsize=(10, 6))
for method in subset3["MethodName"].unique():
    data = subset3[subset3["MethodName"] == method].sort_values("obstacles")
    plt.plot(data["obstacle_pct"], data["TimeRun(ms)"], marker='o', label=method)
plt.title("Výkon metód – 1024x1024, 63 agentov, rôzny počet prekážok")
plt.xlabel("Prekážky (% z plochy mapy)")
plt.ylabel("Čas výpočtu (ms)")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.savefig("graf_3_1024x1024_a63_obstacles.png")

plt.show()

