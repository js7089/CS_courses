import time,random
from subutils import *

MAX_TIME = 3600
MAX_OF_CARTS = 6
EACH_DORM = 5
TIME_LIMIT = 20
LEN_TESTCASE = 1500

# actual distance matrix (tentative)
DIST_MATRIX = [[0, 1, 2, 3, 4, 5, 6, 10, 15, 20], [1, 0, 1, 2, 3, 4, 5, 11, 16, 19], [2, 1, 0, 1, 2, 3, 4, 12, 17, 18], [3, 2, 1, 0, 1, 2, 3, 13, 18, 19], [4, 3, 2, 1, 0, 1, 2, 14, 19, 20], [5, 4, 3, 2, 1, 0, 1, 15, 20, 21], [6, 5, 4, 3, 2, 1, 0, 16, 21, 22], [10, 11, 12, 13, 14, 15, 16, 0, 5, 30], [15, 16, 17, 18, 19, 20, 21, 5, 0, 35], [20, 19, 18, 19, 20, 21, 22, 30, 35, 0]]

verbose,original = False, False
#new_usr = False

class cart():
    global userlist
    cartNo, user = 0, None
    src, dest = None, None
    arrival = True
    starttime, occ_time = None, None
    
    def __init__(self,n):
        self.cartNo = n

    def use(self,userNo,src,dest,starttime,duration):
        userlist.append(userNo)
        userlist_cart.append((userNo,self.cartNo))
        self.arrival = False
        self.user = userNo
        self.src, self.dest = src, dest
        self.starttime, self.occ_time = starttime, duration

    def ret(self,userNo):
        idx = userlist.index(userNo)
        userlist.pop(idx)
        userlist_cart.pop(idx)
        self.user = None
        self.src, self.dest = None, None
        self.arrival = True
        
    
class user():
    global timer;
    uid = 0
    latency = []
    def __init__(self,uid):
        self.uid = uid
    def return_cart(self,delay):
        self.latency.append(delay)

class dorm():
    global userlist,timer,loc,wait
    
    name = ""
    total_carts = None    # available + about to arrive
    queue = None

    def __init__(self,name):
        self.name = name
        self.total_carts = []
        self.queue = []

    def count_cart(self):
        n = 0
        for carts in self.total_carts:
            #print(self.name + "hall : " + str(carts.cartNo))
            if(carts.arrival):
                n += 1
        return n

    
    """
    available_cart()
    return first available cart in total_carts
    if no available carts, return None instead.
    """
    def available_cart(self):
        for i in range(len(self.total_carts)):
            if(self.total_carts[i].arrival):
                return self.total_carts[i]
        return None
    
    def use_cart(self,userNo,src,dest,starttime,duration,rec):
        global dorms

        if(rec):
            # evaluate std_deviation of carts (for each dorm)
            n = []
            for d in dorms:
                n.append(d.count_cart())
            stdev_cart.append(stdev(n))

        
        cart_p = self.available_cart()
        if(cart_p == None):
            if(verbose):
                print("No cart available. Please wait : ",end="")
                print("user#%d [%s → %s] using %d min" %(userNo,src.name,dest.name,duration))
            self.queue.append((userNo,src,dest,duration,starttime))
            return -1;
        else:
            if(rec):    wait.append(0)
            
            cart_p.use(userNo,src,dest,starttime,duration)
            idx = self.total_carts.index(cart_p)
            self.total_carts.pop(idx)
            d = nearest_redir(dorms.index(src),dorms.index(dest))

            
            # distance
            distance = distance_matrix[dorms.index(src)][dorms.index(dest)]
            distance += distance_matrix[dorms.index(dest)][dorms.index(d)]
            dist_stu.append(distance)
            
            cart_p.dest = d
            tmp = d.put(cart_p)
            return tmp
            
    def put(self,cart):
        #print("Put cart #" + str(cart.cartNo) + " to " + self.name + " Hall")
        self.total_carts.append(cart)
        return cart
        
    def return_cart(self,cart):
        idx=userlist.index(cart.user)
        if(verbose):
            print("User#" + str(cart.user) + " is returning cart #" + str(cart.cartNo),end=" ")
        ts = cart.occ_time + cart.starttime
        
        if(verbose):
            print("at time=" + str( cart.occ_time + cart.starttime ) )
        userlist.pop(idx)
        userlist_cart.pop(idx)
        cart.arrival = True
        
        if(len(self.queue)>0):
            wusr = self.queue.pop(0)
            if(verbose):
                print("[Queued] User "+str(wusr[0])+" is using cart #" + str(cart.cartNo) + " t=" + str(ts))
            loc[ts+wusr[3]][1].append("ret " + str(wusr[0]) + " " + str(ts+wusr[3]))

            self.use_cart(wusr[0],wusr[1],wusr[2],ts,wusr[3],False) # call to use_cart() twice when queued!
            
            #if(wait[-1]==0): wait.pop()
            
            wait.append( ts - wusr[-1] )
            #print("Wait time = %d" %(ts - wusr[-1]))
            return ts+int(wusr[3])
        
          
dorms = []                      # list of dorm() object
distance_matrix = DIST_MATRIX   # actual distance matrix
userlist = []                   # who is/are using the carts
userlist_cart = []              # who is using cart @student
glb_carts = []                  # list of carts in use
timer = 0                       # timer variable

# SIMULATION DATA
stdev_cart = []                 # stdev of all carts @every request (simul-2)
dist_stu = []                   # actual distance user might travel (simul-3)

def generate_matrix(n):
    res = []
    for i in range(n):
        elem=[]
        for j in range(n):
            elem.append(random.randrange(1,10))
        res.append(elem)

    for x1 in range(n):
        for y1 in range(n):
            res[x1][y1] = res[y1][x1]
            if(x1==y1):
                res[x1][x1]=0
    
    return res

def zeros(n):
    return [[0]*n]*n

redir_cnt = 0

def nearest_redir(src,dest):
    global distance_matrix, redir_cnt;
    idx = dest   # Index of destination
    q = []
    redir_cnt +=1
    if(original):
        return dorms[src]

    for i in range(len(distance_matrix[idx])):
        q.append( (distance_matrix[idx][i], i) )
    # Now q is list of tuples (distance,Dorm index)
    detour_sorted = sorted(q)
    detour_sorted.pop()
    # find the nearest available detour, in ascending order of distance
    for jdx in detour_sorted:
        if( len(dorms[jdx[1]].total_carts) < MAX_OF_CARTS ):
            return dorms[jdx[1]]
    # if no vacancy available, there's no way but to put cart back to origin
    return dorms[src]

def nocart():
    global dorms
    q=[]
    for halls in dorms:
        q.append(len(halls.total_carts))
    return dorms[q.index(min(q))]
    

def highdemand():
    global dorms
    q=[]
    for halls in dorms:
        q.append(len(halls.queue))
    if( max(q) == 0 ):
        return None
    else:
        return dorms[q.index(max(q))]

def main():
    # main
    nametags = ['아름','소망','사랑','성실','진리','신뢰','지혜','미르','희망','세종']
    
    for names in nametags:
        dorms.append(dorm(names))
    
    global glb_carts,distance_matrix
    
    for i in range(len(nametags)*EACH_DORM):
         glb_carts.append(cart(i))
    for i in range(EACH_DORM):
        for j in range(len(dorms)):
            dorms[j].put(glb_carts[EACH_DORM*j+i])
    
    distance_matrix = DIST_MATRIX # generate_matrix(len(nametags))
    


def print_status(dorm):
    print(dorm.name,end="\t\t")
    for items in dorm.total_carts:
        if(items.arrival):
            print(items.cartNo,end=" ")
    print("\t\t",end="")
    for items in dorm.total_carts:
        if(not items.arrival):
            print(items.cartNo,end=" ")
            
def status():
    print("Dormitory\tAvailable\t\tPending")
    for dorm in dorms:
        print_status(dorm)
        print("")


def use(dorm,dest,user,starttime,duration=TIME_LIMIT):
    global userlist
    if(not (user in userlist)):
        q = dorm.use_cart(user,dorm,dest,starttime,duration,True)
        if(q != -1):
            if(verbose):
                print("User#" + str(user) + " is using cart#" + str(q.cartNo) + " at time=" + str(starttime))
        else:
            if(verbose):
                print("User#" + str(user) + " is requesting cart" + " t=" + str(starttime))
        return
    print(str(user) + " is already using a cart!")
    return

def ret_(stuNo):
    global glb_carts, userlist, userlist_cart
    if(not (stuNo in userlist)):
        #print(str(stuNo) + " is not using a cart!")
        return
    idx=userlist.index(stuNo)
    ret(userlist_cart[idx][1])
    

def ret(cartN):
    global glb_carts
    cart_t = glb_carts[cartN]
    if(cart_t.arrival):     # cart already returns
        return
    dorm_t = cart_t.dest
    dorm_t.return_cart(cart_t)




# prompt
# use <id> <src> <dest> <dur> <timestamp>

loc = []    # [ [timestamp, [<use> <ret>]]* ]
wait=[]
def pf(f="dataset.txt"):
    global loc,wait
    del loc[:]
    del wait[:]
    for time in range(MAX_TIME):
        loc.append([time,[]])
    p = open(f)
    for lines in p:
        l = lines.strip()
        time = int(l.split(" ")[-1])
        if(l.split(" ")[0] == "use"):
            loc[time][1].append(l)

def cnt_wait(lst):
    cnt = 0
    for items in lst:
        if(items>0):
            cnt+=1
    return cnt

def run():
    global loc,wait
    for i in range(len(loc)):
        if(len(loc[i][1])==0):
            continue

        for cmds in loc[i][1]:
            cmd = cmds.split(" ")
            if( cmd[0] == "use" ):
                q=use(dorms[int(cmd[2])],dorms[int(cmd[3])],int(cmd[1]),int(cmd[5]),int(cmd[4]))
                if(q != -1):
                    loc[i+int(cmd[4])][1].append("ret " + str(cmd[1]) + " " + str(i+int(cmd[4])))
                #print("fetching "+"ret " + str(cmd[1]) + " " + str(q))
            elif( cmd[0] == "ret" ):
                ret_(int(cmd[1]))
    vb = "기존의 방식" if(original) else "새로운 방식"
    
    print("■ 테스트 조건")
    print(" □ 알고리즘 :: (%s)" %(vb))
    print(" □ MAX_OF_CARTS = %d\n □ 요청 수 = %d (1분당 %.1f회)" %(MAX_OF_CARTS,LEN_TESTCASE,0.5))
    print("\n■ 결과 (%d개의 요청(%d분))" %(LEN_TESTCASE,find_last()),end="\n")
    print(" □ 대기 시간\n   평균 : %.3f / 최대 : %d, 최소 : %d" %((sum(wait)/LEN_TESTCASE),max(wait),min(wait)))
    print("   대기자 수 : %d (%.3f%%)" %(cnt_wait(wait), (cnt_wait(wait)/LEN_TESTCASE)*100 ))
    print("   100분 이상 대기자: %.3f%%" %(count_100()/LEN_TESTCASE*100))
    print(" □ 이동 거리\n   평균 : %.3f / 최대 : %d, 최소 : %d / 표준편차 : %.3f" %((sum(dist_stu)/len(dist_stu)),max(dist_stu),min(dist_stu), (stdev(dist_stu))))
    print(" □ 카트의 균등 분배\n   (각 요청에 대한) 카트 수의 표준편차\n   평균 : %.3f" %(sum(stdev_cart)/len(stdev_cart)))

def count_100():
    global wait
    cnt=0
    for j in wait:
        if(j>100): cnt+=1
    return cnt
    

    
def find_last():
    global loc
    cnt=0
    for i in range(len(loc)):
        if(len(loc[i][1])>0):
         cnt=i
    return cnt



def test():
    pf()
    run()
    #print("\n ## DISTRIBUTION ## ")
    #distabs(60)
    
    
if __name__ == '__main__':
    main()
    test()
