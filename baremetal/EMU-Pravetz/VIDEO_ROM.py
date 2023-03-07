import os

filename = "VIDEO_ROM_8C.bin"

rom_file = open(filename, "rb") 
rom_bytes = rom_file.read()  

def reverse(data):
    a = []
    for d in data:
        d = int('{:08b}'.format(d)[::-1], 2)
        d = ~d & 0xFF
        d >>= 1
        a.append('0x%02X' % d)
    return a

rom_array = reverse( rom_bytes )
#rom_array = [ ('0x%02X' % x) for x in rom_bytes ]

output_file = open("src/VIDEO_ROM.h", "w")  

output_file.write("#ifndef VIDEO_ROM_H\n")
output_file.write("#define VIDEO_ROM_H\n\n")

output_file.write("// %s\n\n" % filename)

output_file.write("// #define CHARSET_REVERSE\n")

output_file.write("#define CHARACTER_SET_ROM_SIZE %d\n\n" % os.stat(filename).st_size )

output_file.write("const unsigned char ROM_CHAR[] = {\n")

for i in range(0, len(rom_array) ):
    output_file.write( rom_array[i] +', ' )
    if i % 16 == 15:
        output_file.write("\n")

output_file.write("};\n\n")
output_file.write("#endif\n")

output_file.close()
rom_file.close()  