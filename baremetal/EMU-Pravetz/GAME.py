import os

filename = "SABOTAGE.BIN"
start = "1D00"

#filename = "MARIO.BIN"
#start = "0803"

rom_file = open(filename, "rb") 
rom_bytes = rom_file.read()  

rom_array = [ ('0x%02X' % x) for x in rom_bytes ]

output_file = open("src/GAME_ROM.h", "w")  

output_file.write("#ifndef GAME_ROM_H\n")

output_file.write("#define GAME_ROM_H\n\n")

output_file.write("// %s\n\n" % filename)

output_file.write("#define GAME_ADDRESS 0x%s\n\n" % start)

output_file.write("#define GAME_SIZE %d\n\n" % os.stat(filename).st_size )

output_file.write("const unsigned char GAME[GAME_SIZE] = {\n")

for i in range(0, len(rom_array) ):
    output_file.write( rom_array[i] +', ' )
    if i % 16 == 15:
        output_file.write("\n")

output_file.write("};\n\n")
output_file.write("#endif\n")

output_file.close()
rom_file.close()  