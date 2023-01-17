NUM_TESTCASES = 1500
NUM_DORMS = 10
MIN_TIME_LIM = 10
MAX_TIME_LIM = 30

import random,os;

dorm = []

def choose_two(ls):
    random.shuffle(ls)
    return (ls[0],ls[1])

if __name__ == '__main__':
    s = ""
    for d in range(NUM_DORMS):
        dorm.append(d)
    
    for i in range(NUM_TESTCASES):
        tup = choose_two(dorm)
        ts = 8 + int(i/2) + random.randint(-8,8)
        line = "use " + str(i) + " " + str(tup[0]) + " " + str(tup[1]) + " " + str(random.randint(MIN_TIME_LIM,MAX_TIME_LIM)) + " " + str(ts)
        s += line
        s +='\n'
    s = s[:-1]

    if(os.path.exists("dataset.txt")):
        os.remove("dataset.txt")
    
    f = open("dataset.txt","x")
    f.write(s)
    f.close()
    

