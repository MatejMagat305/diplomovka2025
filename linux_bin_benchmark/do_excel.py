import pandas as pd
import re

# Mapovanie metód
method_names = {
    4: "SYCL A* algoritmus",
    3: "CBS kombinované GPU+CPU",
    2: "SYCL A* vlákna (CPU)",
    1: "SYCL A* CPU 1 thread",
    0: "CBS CPU only"
}

# Pomocná funkcia na parsovanie názvu
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

# Načítanie dát
df = pd.read_csv("results.csv")
df["Method"] = df["Method"].map(method_names)
df[["width", "height", "agents", "obstacles"]] = df["File"].apply(lambda x: pd.Series(parse_filename(x)))

# Výbery pre grafy
graf1 = df[df["File"].isin(["map_1024x1024_a64_o256.data", "map_1024x1024_a63_o256.data"])]
relevant_agents = [3, 4, 7, 8, 15, 16, 31, 32, 63, 64]
graf2 = df[(df["width"] == 16) & (df["obstacles"] == 0) & (df["agents"].isin(relevant_agents))]
obstacle_values = [0, 64, 128, 192, 256]
graf3 = df[(df["width"] == 1024) & (df["agents"] == 63) & (df["obstacles"].isin(obstacle_values))]
special = df[df["File"] == "map_16x16_a64_o4.data"]

# Odstrániť nepotrebné stĺpce
columns_to_drop = ["File", "width", "height", "TotalGoalAchieve", "Error"]
graf1 = graf1.sort_values("Method")
graf2 = graf2.sort_values("agents")

dfs = [graf1, graf2, graf3, special]
dfs_cleaned = [d.drop(columns=columns_to_drop, errors="ignore") for d in dfs]

# Rozbalenie do jednotlivých premenných
graf1_clean, graf2_clean, graf3_clean, special_clean = dfs_cleaned

# Export do Excelu
with pd.ExcelWriter("grafy_export.xlsx", engine="xlsxwriter") as writer:
    graf1_clean.to_excel(writer, sheet_name="1024x1024_a64_a63_o256", index=False)
    graf2_clean.to_excel(writer, sheet_name="16x16_aX_o0", index=False)
    graf3_clean.to_excel(writer, sheet_name="1024x1024_a63_oX", index=False)
    special_clean.to_excel(writer, sheet_name="16x16_a64_o4", index=False)

print("Export úspešný – súbor grafy_export_windows.xlsx bol vytvorený.")

