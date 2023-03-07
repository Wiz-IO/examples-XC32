# http://mirrors.apple2.org.za/Apple%20II%20Documentation%20Project/Computers/Pravetz/

filename = "ROM_8M.bin"

rom_file = open(filename, "rb") 
rom_bytes = rom_file.read()  

rom_array = [ ('0x%02X' % x) for x in rom_bytes ]

output_file = open("src/ROM_DATA.h", "w")  

output_file.write("#ifndef ROM_DATA_H\n")
output_file.write("#define ROM_DATA_H\n\n")

output_file.write("// %s\n" % filename)
output_file.write("#define MCU_ROM_ADDRESS 0xD000\n\n")

output_file.write("const unsigned char MCU_ROM_DATA[] = {\n")

for i in range(0, len(rom_array) ):
    output_file.write( rom_array[i] + ', ' )
    if i % 32 == 31: output_file.write("\n")
    if i % 4096 == 4095: output_file.write("\n")

output_file.write("};\n\n")
output_file.write("#endif\n")

output_file.close()
rom_file.close()  