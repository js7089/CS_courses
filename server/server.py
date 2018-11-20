from flask import Flask,request,redirect, render_template
from flask_socketio import SocketIO
import sqlite3,random
import os

app = Flask(__name__, template_folder='templetes')
socketio = SocketIO(app)

MAX_OF_CARTS=2 # 2x2 in demo


# SQL TABLE
#c.execute("CREATE TABLE cart (cid int not null primary key, src int, dest int, available int, time int)")
#c.execute("CREATE TABLE student (sid int not null primary key, cartid int, penalty int)")

taken_carts = []

DORM_LIST = ['사랑관','성실관']

distance_matrix = [[0,1],[1,0]]

def nearest_redir(src,dest,Ncartlist):
    global distance_matrix;
    idx = dest   # Index of destination
    q = []
    for i in range(len(distance_matrix[idx])):
        q.append( (distance_matrix[idx][i], i) )
    # Now q is list of tuples (distance,Dorm index)
    detour_sorted = sorted(q)
    detour_sorted.pop()
    # find the nearest available detour, in ascending order of distance
    for jdx in detour_sorted:
        if( Ncartlist[jdx[1]] < MAX_OF_CARTS ):
            return jdx[1]
    # if no vacancy available, there's no way but to put cart back to origin
    return src

@app.route('/init')
def init():
    global DORM_LIST, taken_carts

    conn = sqlite3.connect('test.db')
    c = conn.cursor()
    taken_carts=[]
    c.execute("DELETE FROM dorm")
    c.execute("DELETE FROM cart")
    c.execute("DELETE FROM student")
    
    for i in range(4):
        s = "INSERT INTO cart (cid, src, dest, available, time) VALUES ("
        s += ( str(i) + "," + "NULL" + "," + str(int(i/2)) + "," + "1" + "," + "0" + ")")
        c.execute(s)

    for j in range(len(DORM_LIST)):
        s2 = "INSERT INTO DORM (did,dname) VALUES ("
        s2 += ( str(j) + ",'" + str(DORM_LIST[j]) + "')")
        c.execute(s2)
        
    q = "<script> alert('Initializing : put 2 carts each on 2 dorms'); window.location.href='/';</script>"
    conn.commit()    
    conn.close()
    return q

@app.route('/')
def index():
    return render_template('main.html')

@app.route('/signup/', methods=['GET'])
def signup():
    global DORM_LIST
    res = ""
    sid = request.args.get('sid')
    conn = sqlite3.connect('test.db')
    c = conn.cursor()

    # check if SID is in table STUDENT
    argc = "SELECT * from student where sid=" + str(sid)
    p = c.execute(argc)
    q = ""
    for i in p:
        q += str(i)

    if(len(q)==0):  # NO SUCH STUDENT : MUST ADD STUINFO
        c.execute("INSERT INTO student (sid, cartid, penalty) VALUES (" + str(sid) + "," + "NULL,0)")
        print("NEW STUDENT %d added" %(int(sid)))
        conn.commit()
        conn.close()
    else:
        pass
    return redirect('/use/?sid='+sid)

@app.route('/return/', methods=['POST'])
def return_cart():
    global taken_carts
    conn=sqlite3.connect('test.db')
    c = conn.cursor()

    if(request.method == 'POST'):
        cid = request.form['cid']
        sid = c.execute("select sid from student where cartid="+cid).fetchone()[0]
        c.execute("update cart set src='NULL', available='1', time='NULL' where cid=" + str(cid))
        c.execute("update student set cartid='NULL' where sid="+str(sid))
        if(cid in taken_carts): taken_carts.pop(taken_carts.index(cid))
    conn.commit()
    conn.close()
    return redirect('/use/?sid='+str(sid))


# Connect with Arduino with <did>
@app.route('/queue/', methods=['GET'])
def get_queue():
    global taken_carts
    did = request.args.get('did')
    conn = sqlite3.connect('test.db')
    c = conn.cursor()

    cnt = c.execute('select count (*) from dorm where did='+str(did)).fetchone()[0]
    if(cnt==0):
        conn.close()
        return ""
    else:
        unlock = c.execute("select cid from cart where src="+str(did)).fetchall()
        lock = c.execute("select cid from cart where dest="+str(did) + " and available=0").fetchall()
        ac = ""
        
        for items in unlock:
            if(not items[0] in taken_carts):
                ac += ("U" + str(items[0]))
        for items in lock:
            if(items[0] in taken_carts):
                ac += ("L" + str(items)[1])
        conn.close()
        return ac
        
    
@app.route('/taken/',methods=['GET'])
def take_cart():
    global taken_carts
    cid = int(request.args.get('cid'))
    if(not cid in taken_carts):
        taken_carts.append(cid)
    return "1"


@app.route('/use/', methods=['GET','POST'])
def use_cart():
    conn = sqlite3.connect('test.db')
    c = conn.cursor()
    res="<a href='/'>HOME</a><br>"
    global DORM_LIST,form_use, form_use2,form_using, form_using2
    # show whole DB STUDENT
    sum_clist = []
    for dorms_n in range(len(DORM_LIST)):
        std_dorm_each = ""
        std_dorm_each += (str(DORM_LIST[dorms_n]) + "(" + str(dorms_n) + ") : ")

        avail_clist=[]
        na_clist=[]
        for rdx in c.execute("select cid from cart where dest=" + str(dorms_n) + " and available=1"):
            avail_clist.append(str(rdx[0]))
        for rdx2 in c.execute("select cid from cart where dest=" + str(dorms_n) + " and available=0"):
            na_clist.append(str(rdx2[0]))

        sum_clist.append( [dorms_n,avail_clist,na_clist] )


        std_dorm_each += ("<b>" + str(avail_clist) + "</b> /" + str(na_clist))
        res += (std_dorm_each + "<br>")

    #print(sum_clist)
    if(request.method == 'GET'):
        sid = request.args.get('sid')
        print("sid is " + str(sid))
        
        q = []
        
        # check if stu is using cart
        for stu in c.execute("select sid from student where cartid!='NULL'"):
            q.append(stu[0])
        using = int(sid) in q

        cptr = c.execute("select cartid from student where sid=" + str(sid)).fetchone()
        if(not cptr==None):
            cid=cptr[0]

        dic = {
            'sid' : str(sid),
            'dorm_carts' : sum_clist,
            'dormN_CONST' : DORM_LIST,
            'using' : using,
            'cid' : 0,
            'dest' : 0
            }
        
        if(using):
            dic['dest'] = c.execute("select dest from cart where cid="+str(cid)).fetchone()[0]
            dic['cid'] = c.execute("select cartid from student where sid=" + str(sid)).fetchone()[0]
            
        return render_template('use.html',d=dic)
    
    else:
        sid,cid,dest = request.form['sid'], request.form['cid'], request.form['dest']

        n_of_carts_db = int(c.execute("select count (*) from cart").fetchone()[0])
        n_of_dorms_db = int(c.execute("select count (*) from dorm").fetchone()[0])

        if((c.execute("select count (*) from cart where cid="+str(cid)).fetchone()[0]==0) or (c.execute("select count (*) from dorm where did="+str(dest)).fetchone()[0]==0)):
            return "<script> alert('Invalid input. Try again.'); window.location.href='/use/?sid="+ str(sid) + "';</script>"

        src = str(c.execute("select dest from cart where cid="+str(cid)).fetchone()[0])
        
        q = []
        # check if stu is using cart
        for stu in c.execute("select sid from student where cartid!='NULL'"):
            q.append(stu[0])
        using = int(sid) in q
        
        if( c.execute("select available from cart where cid="+str(cid)).fetchone()[0] == 0 ):
            return "<script> alert('The cart is already in use.'); window.location.href='/use/?sid="+ str(sid) + "';</script>" 
        elif(using):
            return "<script> alert('Illegal instruction');</script>" + res + form_using

        # 카트수 제한 리디렉션 코드
        redir=False
        carts_at_dest = c.execute("select count (*) from cart where dest="+str(dest)).fetchone()[0]
        if(carts_at_dest < MAX_OF_CARTS):
            pass
        else:
            count_all_carts = []
            for i in range(len(DORM_LIST)):
                cnt = c.execute("select count (*) from cart where dest="+str(i)).fetchone()[0]
                count_all_carts.append(cnt)
            dest = nearest_redir(int(src),int(dest),count_all_carts)
            redir=True
            
        c.execute("update cart set src=" + str(src) + ", dest=" + str(dest) +
                  ", available=0, time=30 where cid=" + cid)
        
        c.execute("update student set cartid="+str(cid) + " where sid="+str(sid))
        conn.commit()
        conn.close()
        return redirect('/use/?sid='+str(sid))
    
    conn.commit()
    conn.close()
    return res
    
if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0')
