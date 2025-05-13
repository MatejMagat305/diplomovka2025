import os
import re
from collections import defaultdict

project_root = '.'
output_file = 'include_dependencies.txt'

include_pattern = re.compile(r'^\s*#\s*include\s*[<"](.+?)[">]')

dependencies = defaultdict(set)

source_files = []
for root, _, files in os.walk(project_root):
    for file in files:
        if file.endswith(('.cpp', '.c', '.h', '.hpp')):
            full_path = os.path.join(root, file)
            source_files.append(full_path)

for file_path in source_files:
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        for line in f:
            match = include_pattern.match(line)
            if match:
                include_file = match.group(1)
                dependencies[file_path].add(include_file)

with open(output_file, 'w', encoding='utf-8') as f:
    for file_path, includes in dependencies.items():
        f.write(f"{file_path} includes:\n")
        for include in sorted(includes):
            f.write(f"  └── {include}\n")
        f.write('\n')

print(f"{output_file}")

