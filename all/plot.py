import pandas as pd
import matplotlib.pyplot as plt
import re

# Mapovanie enum ��sel na popisy z tvojej aplik�cie
method_names = {
    4: "gpu A* algoritmus",
    3: "CBS algoritmus on combination gpu and cpu",
    2: "A* gpu algoritmus on cpu (thread per agent)",
    1: "repeat A* gpu algoritmus on cpu 1 thread",
    0: "CBS algoritmus on only cpu"
}

# Pomocn� funkcia na extrakciu parametrov z n�zvu s�boru
def parse_filename(name):
    match = re.search(r"map_(\d+)x(\d+)_a(\d+)_o(\d+)", name)
    if not match:
        return None
    return {
        "width": int(match.group(1)),
        "height": int(match.group(2)),
        "agents": int(match.group(3)),
        "obstacles": int(match.group(4))
    }

# Na��tanie CSV
df = pd.read_csv("results.csv")
df["MethodName"] = df["Method"].map(method_names)
df[["width", "height", "agents", "obstacles"]] = df["File"].apply(
    lambda x: pd.Series(parse_filename(x))
)

# V�po�et percenta prek�ok
df["map_area"] = df["width"] * df["height"]
df["obstacle_pct"] = (df["obstacles"] / df["map_area"] * 100).round(1)

### GRAF 1: 1024x1024, 64 agentov, 25% prek�ok
subset1 = df[(df["width"] == 1024) & (df["agents"] == 64) & (df["obstacle_pct"] >= 25)]

plt.figure(figsize=(10, 5))
plt.title("V�kon met�d � mapa 1024x1024, 64 agentov, 25% prek�ok")
plt.bar(subset1["MethodName"], subset1["TimeRun(ms)"], color='lightblue')
plt.ylabel("�as v�po�tu (ms)")
plt.xticks(rotation=30, ha='right')
plt.grid(axis='y')
plt.tight_layout()
plt.savefig("graf_1_1024x1024_64agents_25pct.png")

### GRAF 2: 25% prek�ok, 8 agentov, ve�kos� od 16x16 po 1024x1024
subset2 = df[(df["agents"] == 8) & (df["obstacle_pct"] >= 25)]
plt.figure(figsize=(10, 6))
for method, group in subset2.groupby("MethodName"):
    sorted_group = group.sort_values("width")
    plt.plot(sorted_group["width"], sorted_group["TimeRun(ms)"], marker='o', label=method)

plt.title("V�kon met�d � 8 agentov, 25% prek�ok, r�zne ve�kosti m�p")
plt.xlabel("Ve�kos� mapy (��rka = v��ka)")
plt.ylabel("�as v�po�tu (ms)")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("graf_2_8agents_25pct_varsize.png")

### GRAF 3: 1024x1024, 64 agentov, prek�ky 0% a� 25%
subset3 = df[(df["width"] == 1024) & (df["agents"] == 64)]
plt.figure(figsize=(10, 6))
for method, group in subset3.groupby("MethodName"):
    sorted_group = group.sort_values("obstacle_pct")
    plt.plot(sorted_group["obstacle_pct"], sorted_group["TimeRun(ms)"], marker='o', label=method)

plt.title("V�kon met�d � 1024x1024, 64 agentov, r�zny po�et prek�ok")
plt.xlabel("Po�et prek�ok (% z plochy mapy)")
plt.ylabel("�as v�po�tu (ms)")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("graf_3_1024x1024_64agents_obstacles.png")

plt.show()
