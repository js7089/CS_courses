from flask import Flask,request,redirect, render_template
from flask_socketio import SocketIO
import sqlite3,random
import os

app = Flask(__name__, template_folder='templetes')
socketio = SocketIO(app)

MAX_OF_CARTS=6

# to use cart, fill the form (which will be replaced by UI next month)

# mainpage redirection

# SQL TABLE
#c.execute("CREATE TABLE cart (cid int not null primary key, src int, dest int, available int, time int)")
#c.execute("CREATE TABLE student (sid int not null primary key, cartid int, penalty int)")


CNT=0 # CHAT COUNT
DORM_LIST = ['사랑관','성실관','소망관','지혜관','진리관','신뢰관','희망관','미르관','아름관','다솜관']

def n_of_usedcart():
    conn = sqlite3.connect('test.db')
    c = conn.cursor()

    cnt = c.execute("select count (*) from cart where available=0").fetchone()[0]
    return cnt

def decr_MAXCART(n):
    global MAX_OF_CARTS
    if(n_of_usedcart() > n):
        MAX_OF_CARTS=5
    else:
        MAX_OF_CARTS=6

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

distance_matrix = generate_matrix(len(DORM_LIST))

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
    global DORM_LIST

    
    conn = sqlite3.connect('test.db')
    c = conn.cursor()

    c.execute("DELETE FROM dorm")
    c.execute("DELETE FROM cart")
    c.execute("DELETE FROM student")
    
    for i in range(50):
        s = "INSERT INTO cart (cid, src, dest, available, time) VALUES ("
        s += ( str(i) + "," + "NULL" + "," + str(int(i/5)) + "," + "1" + "," + "0" + ")")
        c.execute(s)

    for j in range(len(DORM_LIST)):
        s2 = "INSERT INTO DORM (did,dname) VALUES ("
        s2 += ( str(j) + ",'" + str(DORM_LIST[j]) + "')")
        c.execute(s2)

    sp = "&nbsp"*5
    q = "<script> alert('Initializing : put 5 carts each on 10 dorms'); window.location.href='/';</script>"

    conn.commit()    
    conn.close()

    return q

@app.route('/list')
def list():
    q = ""
    conn = sqlite3.connect('test.db')
    c=conn.cursor()
    
    clist = c.execute("SELECT * FROM CART")
    for rows in clist:
        for j in range(len(rows)):
            if(False):
                tmp = str(rows[j])
                if(rows[j] != None):
                    stb = "SELECT dname from dorm where did=" + tmp
                    ex_dname = ""
                    for k in c.execute(stb):
                        ex_dname += str(k[0])
                    
                    q+=ex_dname
                else:
                    q+="None"
            else:
                q += str(rows[j])
                
            q += "&nbsp"*10
        q += "<br>"

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
        res = '''
        <script> alert("신규 등록에 성공하였습니다! 사용 페이지로 이동"); </script>
        '''
        conn.commit()
    else:
        res = '''
        <script> alert("이미 등록된 학생입니다. 사용 페이지로 이동"); </script>
        '''
    return redirect('/use/?sid='+sid)

@app.route('/return/', methods=['POST'])
def return_cart():
    conn=sqlite3.connect('test.db')
    c = conn.cursor()

    if(request.method == 'POST'):
        sid,cid = request.form['sid'], request.form['cid']
        c.execute("update cart set src='NULL', available='1', time='NULL' where cid=" + str(cid))
        c.execute("update student set cartid='NULL' where sid="+str(sid))
    conn.commit()
    conn.close()
    return redirect('/use/?sid='+str(sid))


@app.route('/use/', methods=['GET','POST'])
def use_cart():
    conn = sqlite3.connect('test.db')
    c = conn.cursor()
    decr_MAXCART(5)
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
            retfrm = '''
            <form action='/return/' method='POST'>
            <input type='hidden' name='sid' value=
            '''
            retfrm += (str(sid) + ">")
            retfrm += '''
            <input type='hidden' name='cid' value=
            '''
            retfrm += str(cid)
            retfrm += '''
            > <input type='submit' value='반납'> </form>
            '''

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
            
            #return res+form_using+"<br>반납 장소 : " + DORM_LIST[new_dest] +"(" + str(new_dest) + ")" +retfrm+form_using2
        #else:
            #return res+form_use + str(sid) + form_use2
        return render_template('use.html',d=dic)
    else:
        sid,cid,dest = request.form['sid'], request.form['cid'], request.form['dest']

        n_of_carts_db = int(c.execute("select count (*) from cart").fetchone()[0])
        n_of_dorms_db = int(c.execute("select count (*) from dorm").fetchone()[0])

        if( (int(dest) >= n_of_dorms_db) or (int(cid) >= n_of_carts_db) ):
            print(str(dest) + str(cid))
            return "<script> alert('유효하지 않은 입력입니다.'); window.location.href='/use/?sid="+ str(sid) + "';</script>"

        src = str(c.execute("select dest from cart where cid="+str(cid)).fetchone()[0])
        
        q = []
        # check if stu is using cart
        for stu in c.execute("select sid from student where cartid!='NULL'"):
            q.append(stu[0])
        using = int(sid) in q
        
        if( c.execute("select available from cart where cid="+str(cid)).fetchone()[0] == 0 ):
            return "<script> alert('해당 카트는 이미 사용중입니다.'); window.location.href='/use/?sid="+ str(sid) + "';</script>" 
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
            
        ###

            
        c.execute("update cart set src=" + str(src) + ", dest=" + str(dest) +
                  ", available=0, time=30 where cid=" + cid)


        c.execute("update student set cartid="+str(cid) + " where sid="+str(sid))
        conn.commit()

        return redirect('/use/?sid='+str(sid))
    
    conn.commit()
    conn.close()
    return res
    
@app.route('/todo/<id>',methods=['GET','POST'])
def todo(id):
    if(not (id == ADMIN_PW)):
        return '''<meta name="viewport" content="width=device-width, initial-scale=1.0"><h2>Access Denied</h2>        '''
        
    else:
        conn = sqlite3.connect('test.db')
        c = conn.cursor()
        #c.execute("drop table todolist")
        #c.execute("CREATE TABLE todolist (pid int not null primary key, datetime text(5), todo text(20))")
        #c.execute("delete from todolist")
       # conn.commit()
        q = c.execute("select max(pid) from todolist").fetchone()[0]
        cnt=0
        if (q==None):
            cnt=0
        else:
            cnt = int(c.execute("select max(pid) from todolist").fetchone()[0])+1

        if(request.method == 'POST'):
            time, todo = request.form['datetime'], request.form['todo']
            arg = "insert into todolist (pid, datetime, todo) values('" + str(cnt) + "','" + str(time) + "','" + str(todo) + "')"
            c.execute(arg)

        conn.commit()
        
        p = c.execute('select * from todolist order by datetime').fetchall()
        conn.commit()
        conn.close()
        return render_template('todo2.html', data=p )
        


if __name__ == '__main__':
    app.run(debug=False, host='0.0.0.0')
