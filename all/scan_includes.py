import os
import re
from collections import defaultdict

# Nastav cestu do svojho projektu
PROJECT_DIR = "./"  # alebo napr. "./src"

include_pattern = re.compile(r'#include\s+"([^"]+)"')

# Mapovanie súbor -> závislosti
dependencies = defaultdict(list)

def scan_file(file_path):
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        for line in f:
            match = include_pattern.search(line)
            if match:
                included = match.group(1)
                dependencies[file_path].append(included)

def collect_files(root_dir):
    all_files = []
    for root, dirs, files in os.walk(root_dir):
        for file in files:
            if file.endswith(".cpp") or file.endswith(".h"):
                full_path = os.path.relpath(os.path.join(root, file), root_dir)
                all_files.append(full_path)
    return all_files

def build_dependency_tree():
    all_files = collect_files(PROJECT_DIR)
    for file in all_files:
        scan_file(os.path.join(PROJECT_DIR, file))
    return dependencies

def print_dependencies(deps):
    print("### Dependency Tree (for Makefile use) ###\n")
    for src, includes in deps.items():
        if src.endswith(".cpp"):
            obj = os.path.splitext(src)[0] + ".o"
            deps_line = " ".join(includes)
            print(f"{obj}: {src} {deps_line}")

if __name__ == "__main__":
    deps = build_dependency_tree()
    print_dependencies(deps)
