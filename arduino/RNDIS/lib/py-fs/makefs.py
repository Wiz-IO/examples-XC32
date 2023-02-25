import os, sys
from os.path import join, normpath, basename
from binascii import hexlify

############################################################################
PYTHON2 = sys.version_info[0] < 3  # True if on pre-Python 3
if PYTHON2:
    pass
else:
    def xrange(*args, **kwargs):
        return iter( range(*args, **kwargs) )
############################################################################

path = os.path.dirname(__file__)
OUTPUT = open( join(path, "fsdata.c"), "w") 
OUTPUT.write( '#include "fsdata.h"\n' )

cnt_files = 0
list = []

def hexs(s):
    if False == PYTHON2: 
        if str == type(s):
            s = bytearray(s, 'utf-8')
    return hexlify(s).decode("ascii").upper()

def list_files(startpath):
    global cnt_files
    for root, dirs, files in os.walk(startpath):
        
        for f in files:
            print('----', os.path.basename(root) )

            if os.path.basename(root) != "fs": 
                name = '/' + os.path.basename(root) +'/' + f
            else: 
                name = '/' + f

            OUTPUT.write( "static const unsigned char data_" + name.replace('.','_').replace('/','_') + "[] = {\n")
            F = open(join(root, f), "rb")
            OUTPUT.write( "\t" )

            for i in range( len(name) ):
                OUTPUT.write( '0x' + hexs( name[i] ) + ', ' )
            OUTPUT.write( "0x00,\n" )

            i = 0
            c = 0
            while True: 
                data = F.read(1)          
                if not data: 
                    break
                if i % 16 == 0: OUTPUT.write("\n\t")
                i += 1    
                c += 1
                OUTPUT.write( '0x' + hexs(data) + ',' )
            OUTPUT.write( '};\n\n' )
            F.close()
            cnt_files += 1
            
            list.append([name, c])
            print( len(name)+1)

def main():
    global cnt_files
    list_files( join(path, 'fs') )
    print(list)
    for i in range( len(list) ):
        cur = '_' + list[i][0].replace('.','_').replace('/','_')
        if i == 0:
            prev = "NULL";
        else:
            prev = "file_" + list[i-1][0].replace('.','_').replace('/','_')

        OUTPUT.write("const struct fsdata_file file" + cur + "[] = {{" + prev + ", data" + cur + ", ");
        OUTPUT.write("data" + cur + ' + ' + str( len( list[i][0] ) + 1 )  + ", ");
        OUTPUT.write("sizeof(data" + cur + ") - " + str( len( list[i][0] ) + 1) + ", 0}};\n\n");

    OUTPUT.write( '\n#define FS_ROOT file_img_favicon_png\n' )
    OUTPUT.write( '#define FS_NUMFILES {}\n'.format(cnt_files) )
    OUTPUT.close()


if __name__ == "__main__":
    main()   
