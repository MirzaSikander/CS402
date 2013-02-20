def words(fileobj,words):
    for line in fileobj:
        for word in line.split():
            if(words.count(word) == 0):
                words.append(word.lower().strip())

def sortIntoBalanceTreeVersion(words,outputList,start, end):
    if (end < start):
        return     
    mid = int((start + end) / 2)
    outputList.append(words[mid])
    sortIntoBalanceTreeVersion(words,outputList, start, mid - 1)
    sortIntoBalanceTreeVersion(words, outputList, mid + 1, end)
    return 

try:
    f = open("assignmentText", "r")


    try:
        wordslist= []
        balancedTreeVersion = []

        words(f,wordslist)
        wordslist.sort()
        f = open('assignmentText_SortedList','w')
        for i in range(len(wordslist)):
            print wordslist[i]
            f.write(wordslist[i]+'\n')

        f.close()
        sortIntoBalanceTreeVersion(wordslist,balancedTreeVersion,0,len(wordslist)-1)

        f = open('assignmentText_bTList', 'w')
        for w in balancedTreeVersion:
            f.write("a "+w+" "+w)
            f.write('\n')
    finally:
        f.close()
except IOError:
    pass


