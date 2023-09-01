import sys
import os
import re

if len(sys.argv) != 3:
    print('error: expected two command line arguments: <root_dir> <binary_dir>')
    exit()

root_dir = sys.argv[1]
binary_dir = sys.argv[2]

output_dir = os.path.join(binary_dir, 'snitch')
output_filename = 'snitch_all.hpp'

# Make sure the output directory exists; should always be true since we read
# 'snitch_config.hpp' from there, but in case this changes in the future:
os.makedirs(output_dir, exist_ok=True)

# Gather header inclusion structure.
header_map = {}
def add_headers(filename):
    headers = []
    with open(filename) as header:
        for line in header.readlines():
            include = re.search(r'#\s*include "(snitch/snitch_[^"]+hpp)"', line)
            if not include:
                continue

            include = include.group(1)
            if 'snitch_config.hpp' in include:
                include_dir = binary_dir
            else:
                include_dir = os.path.join(root_dir, 'include')

            child_header = os.path.join(include_dir, include)
            headers.append(child_header)

            if include not in header_map:
                header_map[child_header] = add_headers(child_header)

    return headers

add_headers(os.path.join(root_dir, 'include', 'snitch', 'snitch.hpp'))

# Add leaf headers that don't include any other.
input_filenames = list(include for include, children in header_map.items() if len(children) == 0)

# Add other headers iteratively.
while len(input_filenames) < len(header_map):
    last_length = len(input_filenames)

    for include, children in header_map.items():
        if not include in input_filenames and all(child in input_filenames for child in children):
            input_filenames.append(include)

    if len(input_filenames) == last_length:
        raise Exception('Circular include pattern detected')

# Add implementation.
main_source = os.path.join(root_dir, 'src', 'snitch.cpp')
with open(main_source) as src:
    for line in src.readlines():
        file = re.search(r'#\s*include "(snitch_[^"]+cpp)"', line)
        if file:
            input_filenames.append(os.path.join(root_dir, 'src', file.group(1)))

with open(os.path.join(output_dir, output_filename), 'w') as output_file:
    # Add preamble
    output_file.write('#ifndef SNITCH_ALL_HPP\n')
    output_file.write('#define SNITCH_ALL_HPP\n')

    file_count = 0
    for input_filename in input_filenames:
        # Add guard for implementation files
        if '.cpp' in input_filename:
            output_file.write('#if defined(SNITCH_IMPLEMENTATION)\n')

        # Write whole file
        with open(input_filename, 'r') as input_file:
            for line in input_file.readlines():
                # Remove includes to snitch/*.hpp; it's all one header now
                if re.match(r'#\s*include "snitch', line):
                    continue
                output_file.write(line)

        # Close guard for implementation files
        if '.cpp' in input_filename:
            output_file.write('\n#endif\n')

        file_count += 1
        if file_count != len(input_filenames):
            output_file.write('\n')

    output_file.write('\n#endif\n')
