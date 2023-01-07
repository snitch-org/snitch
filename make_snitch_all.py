import sys
import os

if len(sys.argv) != 3:
    print('error: expected two command line arguments: <root_dir> <binary_dir>')
    exit()

root_dir = sys.argv[1]
binary_dir = sys.argv[2]

output_dir = os.path.join(binary_dir, 'snitch')
output_filename = 'snitch_all.hpp'

# Files to concatenate, in order
input_filenames = [os.path.join(binary_dir, 'snitch/snitch_config.hpp'),
               os.path.join(root_dir, 'include/snitch/snitch.hpp'),
               os.path.join(root_dir, 'src/snitch.cpp')]

# Make sure the output directory exists; should always be true since we read
# 'snitch_config.hpp' from there, but in case this changes in the future:
os.makedirs(output_dir, exist_ok=True)

with open(os.path.join(output_dir, output_filename), 'w') as output_file:
    file_count = 0
    for input_filename in input_filenames:
        # Add guard for implementation files
        if '.cpp' in input_filename:
            output_file.write('#if defined(SNITCH_IMPLEMENTATION)\n')

        # Write whole file
        with open(input_filename, 'r') as input_file:
            for line in input_file.readlines():
                # Remove includes to snitch/*.hpp; it's all one header now
                if '#include "snitch' in line:
                    continue
                output_file.write(line)

        # Close guard for implementation files
        if '.cpp' in input_filename:
            output_file.write('#endif\n')

        file_count += 1
        if file_count != len(input_filenames):
            output_file.write('\n')
