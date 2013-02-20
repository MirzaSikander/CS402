import sys

try:
    if(len(sys.argv) > 2):
        f = open(sys.argv[1], "r")


        try:
            wordslist=f.readlines()
            o = open(sys.argv[1]+"_t",'w')
            for i in range(int(sys.argv[2])):
                o.writelines(wordslist)
            o.close()
        finally:
            f.close()

    else:
        print "Program needs the file to copy and the number of times to copy"
except IOError:
    pass
    


