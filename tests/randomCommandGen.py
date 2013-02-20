import sys
import random

if(len(sys.argv)>3):
    try:
        f = open(sys.argv[1], "r")
        try:
            wordslist= f.readlines()
            commands = ['a','d','q']
            for i in range(int(sys.argv[2])):
                o = open('random/r'+str(i),'w')
                print "r"+str(i)+" created\n"
                for j in range(int(sys.argv[3])):
                    random.seed()
                    com = commands[int(random.randint(0,32)%3)]                
                    random.seed()
                    val = wordslist[random.randint(0,len(wordslist)-1)].strip()
                    output = ""
                    if(com == 'a'):
                        output = "a "+val+" "+val
                    else:
                        output = com+" "+val
                    o.write(output+"\n")    
                o.close()
        finally:
            f.close()
    except IOError:
        pass
else:
    print "program needs three inputs: a file with words, the number of random generated files wanted and the number of commands\n"

