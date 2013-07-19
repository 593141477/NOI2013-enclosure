#!/usr/bin/python
# -*- coding: utf-8 -*-
import sys
import subprocess

NUM_PLAYER = 4
TOTAL_ROUND = 100
MAX_X = 10
MAX_Y = 10
TRAP_LIFETIME = 3
PAUSE_ROUND = 5

RED_START="\x1B[0;31m"
GREEN_START="\x1B[0;32m"
YELLOW_START="\x1B[0;33m"
COLOR_END="\x1B[0m"

GUI_LAND_SIZE = 20
GUI_LAND_COLOR= ['#ffffaa', '#aaffaa', '#aaaaff', '#ffaaaa'] 
GUI_BOT_COLOR = ['#d9b527', '#2e9e2e', '#4843d9', '#d14343']
GUI_FORM_PADDING = 5
#游戏地图,静态
class GameMap:
	NO_OWNER = -1
	class Trap_t:
		def __init__(self, x, y):
			self.x = x
			self.y = y
			self.left = TRAP_LIFETIME
		def __str__(self):
			return "(x=%d y=%d left=%d)"%(self.x,self.y,self.left)
	class Enclosure_tmp:
		def __init__(self, name, blocks):
			self.name = name
			self.blocks = blocks
			self.area = len(blocks)
	Traps = []
	nEnclosure = 0
	LandOwner = [[NO_OWNER]*MAX_X for i in xrange(MAX_Y)]
	LandID = [[-1]*MAX_X for i in xrange(MAX_Y)]
	lastEnclosure = [[False]*MAX_X for i in xrange(MAX_Y)]
	pending = []
	remaining = MAX_Y*MAX_X

	@staticmethod
	def reportTraps():
		ret = [len(GameMap.Traps)]
		for i in GameMap.Traps:
			ret += [i.x, i.y, i.left]
		return ret

	@staticmethod
	def invalid_pos(pos):
		(x, y) = pos
		return x<0 or x>MAX_X or y<0 or y>MAX_Y

	@staticmethod
	def onBorder(pos):
		(x, y) = pos
		return x==0 or x==MAX_X or y==0 or y==MAX_Y

	@staticmethod
	def updateTraps():
		for i in GameMap.Traps:
			i.left -= 1
		GameMap.Traps = filter((lambda x: x.left>0), GameMap.Traps)

	@staticmethod
	#判断指定点是否有陷阱,如果有,按照规则消除陷阱
	def checkTrap(x, y):
		new_list = filter((lambda i: not(i.x==x and i.y==y)), GameMap.Traps)
		ret = (len(new_list) != len(GameMap.Traps))
		GameMap.Traps = new_list
		return ret

	@staticmethod
	def __addOneTrap(x, y):
		if GameMap.invalid_pos((x,y)):
			return
		for i in xrange(len(GameMap.Traps)):
			if GameMap.Traps[i].x == x and GameMap.Traps[i].y == y:
				GameMap.Traps[i] = GameMap.Trap_t(x, y)
				return
		GameMap.Traps.append(GameMap.Trap_t(x, y))

	@staticmethod
	#使用移动轨迹的两点添加三个陷阱
	def addTrap(st, ed):
		if st[0] == ed[0]:
			for i in range(-1, 2):
				GameMap.__addOneTrap(st[0]+i, st[1])
		else:
			for i in range(-1, 2):
				GameMap.__addOneTrap(st[0], st[1]+i)

	@staticmethod
	#使用两点确定移动轨迹穿越了谁的区域
	def getEdgeOwner(st, ed):
		if st[0]==ed[0]: #vertical
			rx = st[0]
			lx = rx-1
			ry = ly = min(ed[1], st[1])
			if lx<0 or rx>=MAX_X:
				return GameMap.NO_OWNER
			lid = GameMap.LandID
			if lid[lx][ly] != lid[rx][ry]:
				return GameMap.NO_OWNER
			return GameMap.LandOwner[lx][ly]
		elif st[1]==ed[1]: #horizontal
			dy = st[1]
			uy = dy-1
			dx = ux = min(ed[0], st[0])
			if uy<0 or dy>=MAX_Y:
				return GameMap.NO_OWNER
			lid = GameMap.LandID
			if lid[dx][dy] != lid[ux][uy]:
				return GameMap.NO_OWNER
			return GameMap.LandOwner[dx][dy]
		else:
			raise Exception("Not an edge")

	@staticmethod
	#确定一个点处于谁的区域
	def getPointOwner(pt):
		if GameMap.onBorder(pt):
			return GameMap.NO_OWNER
		(x,y) = pt
		lid = GameMap.LandID
		#周围不是同一时期被圈的地,说明是边界上的点
		if not(lid[x-1][y-1]==lid[x-1][y]==lid[x][y-1]==lid[x][y]):
			return GameMap.NO_OWNER
		return GameMap.LandOwner[x][y]

	@staticmethod
	#添加一个包围
	def addEnclosure(name, track):
		assert len(track)>=5
		assert track[0]==track[-1]
		vt_lines = []
		#得到轨迹中所有竖线
		for i in xrange(1, len(track)):
			st = track[i-1]
			ed = track[i]
			if st[0]!=ed[0]:
				continue
			#存储竖线右边的一个方格,先行后列(先y后x),便于排序
			vt_lines.append((min(ed[1], st[1]), st[0]))

		assert len(vt_lines)%2==0
		vt_lines.sort()
		blocks = []
		#把竖线两两配对,中间的部分就是圈到的地
		for i in xrange(0, len(vt_lines), 2):
			for j in xrange(vt_lines[i][1], vt_lines[i+1][1]):
				blocks.append((j, vt_lines[i][0]))
		GameMap.pending.append(GameMap.Enclosure_tmp(name, blocks))

		# print GREEN_START,"addEnclosure:\n",track,"\n",vt_lines,COLOR_END

	@staticmethod
	def __floodfill_mark(x, y, ID):
		if x<0 or y<0 or x>=MAX_X or y>=MAX_Y or GameMap.LandID[x][y]!=-2:
			return 0
		ret = 1
		GameMap.LandID[x][y] = ID
		for i in [(-1, 0), (1, 0), (0, 1), (0, -1)]:
			ret += GameMap.__floodfill_mark(x+i[0], y+i[1], ID)
		return ret

	@staticmethod
	#处理之前添加的包围
	def doEnclose():
		result = []
		GameMap.lastEnclosure = [[False]*MAX_X for i in xrange(MAX_Y)]
		#面积从小到大处理,以满足题目描述的由内向外结算
		GameMap.pending.sort(cmp=(lambda x,y: x.area-y.area))
		for e in GameMap.pending:
			# print GREEN_START,e.blocks,COLOR_END
			for i in e.blocks:
				(x, y) = i;
				GameMap.lastEnclosure[x][y] = True
				if GameMap.LandOwner[x][y] == GameMap.NO_OWNER:
					GameMap.LandOwner[x][y] = e.name
					# GameMap.LandID[x][y] = GameMap.nEnclosure
					GameMap.LandID[x][y] = -2
			#使用floodfill的方式圈地,以满足题目要求"不是连续区域的话会算作多个区域"
			for i in e.blocks:
				(x, y) = i
				if GameMap.LandID[x][y] == -2:
					cnt = GameMap.__floodfill_mark(x, y, GameMap.nEnclosure)
					GameMap.nEnclosure+=1
					GameMap.remaining -=cnt
					result.append((e.name,cnt))
					print YELLOW_START,"Bot %d got %d blocks"%(e.name, cnt),COLOR_END
		del GameMap.pending[:]

		# print GREEN_START,"lastEnclosure:"
		# for i in GameMap.lastEnclosure:
		# 	print i
		# print COLOR_END
		return result

	@staticmethod
	#判断点是否处于上一次圈地范围中(被围杀)
	def inLastEnclosure(pt):
		if GameMap.onBorder(pt):
			return False
		(x,y) = pt
		last = GameMap.lastEnclosure
		return last[x-1][y-1] and last[x-1][y] and last[x][y-1] and last[x][y]

	@staticmethod
	#统计剩余的无主区域
	def remainingArea(bot_name):
		result = []
		for i in xrange(MAX_X):
			for j in xrange(MAX_Y):
				if GameMap.LandOwner[i][j] == GameMap.NO_OWNER:
					GameMap.LandOwner[i][j] = bot_name
					assert GameMap.LandID[i][j]==-1
					GameMap.LandID[i][j] = -2
		for i in xrange(MAX_X):
			for j in xrange(MAX_Y):
				if GameMap.LandID[i][j] == -2:
					cnt = GameMap.__floodfill_mark(i, j, GameMap.nEnclosure)
					GameMap.nEnclosure+=1
					GameMap.remaining -=cnt
					result.append((bot_name,cnt))
					print YELLOW_START,"Bot %d got %d blocks"%(bot_name, cnt),COLOR_END
		return result

#机器人
class Bot:
	DIR = {'l': (-1,0), 'r': (1,0), 'u': (0,-1), 'd': (0,1), 's': (0,0)}
	OP = [-1, 0, 1]
	class STATUS:
		DEAD = -1  #死亡
		UP   = 0   #抬笔
		DOWN = 1   #落笔

	def __init__(self, name, program):
		self.name = name
		self.x = 0
		self.y = 0
		self.status = Bot.STATUS.UP
		self.trappedleft = 0
		self.score = 4-name
		self.score_used = 0
		self.track = []
		try:
			self.prog = subprocess.Popen(args=program, stdin=subprocess.PIPE, stdout=subprocess.PIPE, universal_newlines=True, bufsize=1)
		except:
			print "Cannot run bot %d"%name

	def die(self, reason):
		if self.status != Bot.STATUS.DEAD:
			print RED_START,"Bot %d died"%self.name,COLOR_END
			if reason:
				print RED_START,"Reason:",reason,COLOR_END
		self.status = Bot.STATUS.DEAD

	def __read_bot(self):
		line = self.prog.stdout.readline().rstrip()
		print ("%d >>> judge:"%self.name), line
		return line.split(' ')

	def __write_bot(self, data):
		try:
			print ("judge >>> %d:"%self.name), data
			self.prog.stdin.write(data)
			self.prog.stdin.write('\n')
		except Exception:
			self.die("Failed to write to bot")

	def __read_pos(self):
		try:
			s = self.__read_bot();
			assert s[0]=="[POS]"
			self.x = int(s[1])
			self.y = int(s[2])
			if self.x<0 or self.x>MAX_X or self.y<0 or self.y>MAX_Y:
				self.die("Invalid initial position")
		except:
			self.die("Invalid [POS] statement")

	#向机器人发送信息
	def send(self, data):
		if self.status == Bot.STATUS.DEAD:
			return
		self.__write_bot(data)
	#从机器人接收指令
	def recv(self):
		if self.status == Bot.STATUS.DEAD:
			return
		try:
			s = self.__read_bot()

			#判断输入格式
			assert (len(s)>1 and s[0]=="[ACTION]"), "empty response, program may crash"
			assert (s[1] in Bot.DIR),"direction not in %s" % Bot.DIR
			delta = Bot.DIR.get(s[1])
			op = int(s[2])
			assert (op in Bot.OP),"operation not in %s" % Bot.OP
			assert (not (self.status==Bot.STATUS.DOWN and op!=0)),"operation!=0 while drawing"

			if self.trappedleft > 0:
				return

			old_pos = (self.x, self.y)
			new_pos = (self.x+delta[0], self.y+delta[1])

			#出界
			if GameMap.invalid_pos(new_pos):
				raise Exception("out of bounds")
			#使用泥缚但不移动
			if op == -1 and delta == (0,0):
				raise Exception("didn't move while setting a trap")
			
			#非法闯入检测
			if old_pos!=new_pos:
				owner = GameMap.getEdgeOwner(old_pos, new_pos)
				if owner!= GameMap.NO_OWNER:
					#进入他人领地
					if owner!=self.name:
						raise Exception("Enter others' area")
					#落笔进入自己领地
					elif op==1 or self.status==Bot.STATUS.DOWN:
						raise Exception("Enter your own area while drawing")
			
			if self.status == Bot.STATUS.DOWN:
				#落笔时后退
				if len(self.track)>1 and self.track[-2]==new_pos:
					raise Exception("Try to go back while drawing")
				#记录轨迹(不记录重复点)
				if self.track[-1]!=new_pos:
					self.track.append(new_pos)
			
			#角色移动
			(self.last_x, self.last_y) = old_pos
			(self.x, self.y) = new_pos

			#落笔,开始记录轨迹
			if op == 1:
				self.status = Bot.STATUS.DOWN
				self.track = [old_pos]
				if new_pos != old_pos:
					self.track.append(new_pos)
			#使用技能
			elif op == -1:
				GameMap.addTrap(old_pos, new_pos)
				self.score -= 10
				self.score_used += 10
		except IOError:
			self.die("IOError: Failed to read from bot")
		except AssertionError as e:
			self.die("AssertionFailed: %s" % e.message)
		except Exception as e:
			msg = str(e)
			if len(msg) == 0:
				msg = "Invalid [ACTION] statement";
			self.die(msg)
	#检测是否进入陷阱
	def checkTrap(self):
		if self.status == Bot.STATUS.DEAD:
			return
		if GameMap.checkTrap(self.x, self.y):
			#因为本轮结束时会更新减一,所以现在增加一轮
			self.trappedleft = PAUSE_ROUND+1
	#处理圈地事件
	def enclose(self):
		if self.status == Bot.STATUS.DEAD:
			return
		n = len(self.track)
		if n<4:
			return
		last = self.track[-1]
		for i in xrange(0, n-1):
			#在轨迹前n-1个点中找到与第n个相同的点,为闭合轨迹
			if self.track[i]==last:
				GameMap.addEnclosure(self.name, self.track[i:])
				self.status=Bot.STATUS.UP
				self.track=[]
				return
	#检测被围杀事件
	def checkEnclosure(self):
		if self.status == Bot.STATUS.DEAD:
			return
		if GameMap.inLastEnclosure((self.x,self.y)):
			self.die("Be enclosed")
	#报告状态
	def report(self):
		return (self.x, self.y, self.status, self.trappedleft, self.score_used)
	#更新机器人状态
	def update(self):
		if self.trappedleft > 0:
			self.trappedleft -= 1;
	#第一回合开始前,启动机器人
	def poweron(self):
		self.__write_bot("[START] %d"%self.name)
		self.__read_pos()
	#游戏结束,关闭机器人
	def poweroff(self):
		try:
			self.prog.kill()
			self.prog.wait()
		except:
			self.die("Failed to kill bot")

def list2str(l):
	return ' '.join(str(x) for x in l)

#存储所有机器人实例
bots = []
#已进行回合数
nRound = 0

#游戏开始
def startGame():
	#启动所有机器人,获取初始位置
	for x in xrange(0, NUM_PLAYER):
		bot = Bot(x, sys.argv[x+1])
		bot.poweron()
		bots.append(bot)

#运行一回合游戏,返回True说明达到游戏结束条件
def runRound():
	global nRound
	output_list = ["[STATUS]"]

	#查询所有机器人状态
	for i in xrange(0, NUM_PLAYER):
		info = bots[i].report()
		output_list += list(info)
	#查询所有陷阱状态
	output_list += GameMap.reportTraps()
	#发送状态信息
	output_str = list2str(output_list)
	for i in xrange(0, NUM_PLAYER):
		bots[i].send(output_str)

	#接收机器人指令,处理移动等
	for i in xrange(0, NUM_PLAYER):
		bots[i].recv()
	#检测是否进入泥淖
	for i in xrange(0, NUM_PLAYER):
		bots[i].checkTrap()
	#处理圈地事件
	for i in xrange(0, NUM_PLAYER):
		bots[i].enclose()
	#等到所有机器人都确定圈地范围后再一同处理
	result = GameMap.doEnclose()
	#根据圈地情况加分
	for i in result:
		bots[i[0]].score += 10*(i[1]**2)
	#检测被围杀
	for i in xrange(0, NUM_PLAYER):
		bots[i].checkEnclosure()
	#检测被打断事件
	killed = set()
	for i in xrange(0, NUM_PLAYER):
		if bots[i].status==Bot.STATUS.DEAD:
			continue
		pt = (bots[i].x,bots[i].y)
		for j in xrange(0, NUM_PLAYER):
			if i==j or bots[j].status!=Bot.STATUS.DOWN:
				continue
			if pt in bots[j].track:
				killed.add(j)
	for i in killed:
		bots[i].die("Interrupted")

	#更新泥淖剩余周期
	GameMap.updateTraps()
	for i in xrange(0, NUM_PLAYER):
		bots[i].update()

	nRound += 1

	live = 0
	for i in xrange(0, NUM_PLAYER):
		if bots[i].status != Bot.STATUS.DEAD:
			live += 1
			live_bot = bots[i]
	#检测游戏结束条件
	if live<2 or nRound==TOTAL_ROUND or GameMap.remaining==0:
		if live == 1:
			getRemaining(live_bot)
		return False
	return True

#剩余1个玩家获取剩余土地
def getRemaining(bot):
	result = GameMap.remainingArea(bot.name)
	for i in result:
		bot.score += 10*(i[1]**2)

def printScore(write_to_file = False):
	print YELLOW_START+"Score:"
	if write_to_file:
		score_file = open("score.log", 'a')
	for i in xrange(0, NUM_PLAYER):
		print "Bot %d: %d"%(bots[i].name, bots[i].score)
		if write_to_file:
			score_file.write("%d " % bots[i].score)
	if write_to_file:
		score_file.write("\n")
	print COLOR_END

#游戏结束
def endGame():
	#关闭所有机器人
	for x in xrange(0, NUM_PLAYER):
		bots[x].poweroff()

def initGUI():
	global tk_root, canvas, Tkinter
	import Tkinter
	tk_root = Tkinter.Tk()
	tk_root.geometry('+200+200')
	canvas = Tkinter.Canvas(tk_root, bd=0, height=GUI_LAND_SIZE*MAX_Y+GUI_FORM_PADDING*2, width=GUI_LAND_SIZE*MAX_X+GUI_FORM_PADDING*2)
	canvas.pack()

def updateGUI():
	canvas.delete(Tkinter.ALL)
	for x in xrange(MAX_X):
		for y in xrange(MAX_Y):
			owner = GameMap.LandOwner[x][y]
			if owner==GameMap.NO_OWNER:
				color = 'white'
			else:
				color = GUI_LAND_COLOR[owner]
			sx=x*GUI_LAND_SIZE
			sy=y*GUI_LAND_SIZE
			canvas.create_rectangle(sx+1+GUI_FORM_PADDING, sy+1+GUI_FORM_PADDING, sx+GUI_LAND_SIZE+GUI_FORM_PADDING, sy+GUI_LAND_SIZE+GUI_FORM_PADDING, fill=color, outline='')
			# canvas.create_text(sx+9+GUI_FORM_PADDING, sy+9+GUI_FORM_PADDING, font=('Courier','14','bold'),text=GameMap.LandID[x][y])
	for i in GameMap.Traps:
		x, y = i.x*GUI_LAND_SIZE+GUI_FORM_PADDING, i.y*GUI_LAND_SIZE+GUI_FORM_PADDING
		canvas.create_oval(x-6, y-6, x+7, y+7, fill='black', outline='')
		canvas.create_text(x, y+1, font=('Courier','12','bold'),text=str(i.left),fill="white")
	for i in bots:
		if i.status == Bot.STATUS.DEAD:
			continue
		pts = []
		for j in i.track:
			pts.append(j[0]*GUI_LAND_SIZE+GUI_FORM_PADDING)
			pts.append(j[1]*GUI_LAND_SIZE+GUI_FORM_PADDING)
		if len(pts)>2:
			canvas.create_line(*pts)
	for i in bots:
		if i.status == Bot.STATUS.DEAD:
			continue
		x, y = i.x*GUI_LAND_SIZE+GUI_FORM_PADDING, i.y*GUI_LAND_SIZE+GUI_FORM_PADDING
		canvas.create_oval(x-5, y-5, x+6, y+6, fill=GUI_BOT_COLOR[i.name], outline='')
		if i.trappedleft > 0:
			canvas.create_text(x, y+1, font=('Courier','12','bold'),text=str(i.trappedleft),fill="white")

def GUItask():
	ret = runRound()
	updateGUI()
	if ret:
		tk_root.after(100, GUItask)
	else:
		tk_root.after(1000, tk_root.destroy)
		
#======= main =======
if len(sys.argv) < 5:
	print "usage:",sys.argv[0],"bot1 bot2 bot3 bot4 [-gui]"
	exit(1)

startGame()
if len(sys.argv)>5 and sys.argv[5]=='-gui':
	initGUI()
	updateGUI()
	tk_root.update()
	tk_root.after(10, GUItask)
	tk_root.mainloop()
else:
	while runRound():
		pass
if len(sys.argv)>5 and sys.argv[5]=='-w':
	printScore(True)
else:
	printScore()
endGame()
