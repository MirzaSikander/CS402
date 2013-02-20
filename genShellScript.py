import sys

f = open("automate_equal", "w") 
f.write("echo \"");
for i in range( sys.argv[1] if sys.argv[1] is None else 100):
    o = open("output/output"+str(i),"w")
    o.close()
    f.write("E tests/query_agent_t output/output"+str(i)+"\n")

for i in range( sys.argv[1] if sys.argv[1] is None else 100):
    o = open("output/output"+str(i+100),"w")
    o.close()
    f.write("E tests/edit_agent_t output/output"+str(100+i)+"\n")

for i in range( sys.argv[1] if sys.argv[1] is None else 100):
    o = open("output/output"+str(i+200),"w")
    o.close()
    f.write("E tests/random/r"+str(i)+" output/output"+str(200+i)+"\n")
    
f.write("w\n\"")
    
