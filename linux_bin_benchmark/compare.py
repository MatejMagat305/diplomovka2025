import os
import re
def replace_nanoseconds_with_microseconds(file_path):
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as file:
            content = file.read()
    except Exception as e:
        print(f'Chyba pri čítaní {file_path}: {e}')
        return

    # Vzor pre zámenu
    new_content = re.sub(
        r'std::chrono::duration_cast\s*<\s*std::chrono::nanoseconds\s*>',
        'std::chrono::duration_cast<std::chrono::microseconds>',
        content
    )

    if content != new_content:
        try:
            with open(file_path, 'w', encoding='utf-8') as file:
                file.write(new_content)
            print(f'Zmenené: {file_path}')
        except Exception as e:
            print(f'Chyba pri zápise do {file_path}: {e}')
    else:
        print(f'Bez zmien: {file_path}')


def process_directory(directory):
    for root, _, files in os.walk(directory):
        for name in files:
            if name.endswith(('.cpp', '.hpp', '.cc', '.h')):
                replace_nanoseconds_with_microseconds(os.path.join(root, name))

# Spustiť na aktuálnom adresári
if __name__ == '__main__':
    target_dir = '.'  # môžeš zmeniť na konkrétny priečinok
    process_directory(target_dir)

