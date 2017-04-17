#!/usr/bin/env python3
import socket
import queue
import threading
from time import sleep
import signal
import errno
import subprocess

#listen_port = 12345
#'''
gameboxs = [
	"10.217.1.201",
	"10.217.2.201",
	"10.217.3.201",
	#"10.217.4.201",
	"10.217.5.201",
	"10.217.6.201",
	"10.217.7.201",
	"10.217.8.201",
	"10.217.9.201",
	"10.217.10.201",
	"10.217.11.201",
	"10.217.12.201",
	"10.217.13.201",
	"10.217.14.201",
	"10.217.15.201",
	"10.217.16.201"
]
'''
gameboxs = ['127.0.0.1']
#'''

def handlemsg(ip, msg):
	print(ip, msg[:-1])
	oput = subprocess.check_output("curl '10.217.0.100/team/submit_key?token=79c929f25c5fb1c189c1e599f99f1c24&key=%s'"%(msg[:-1]), shell=True).decode()
	print(oput)
	if oput.find("Congratulations") != -1:
		qdict[ip].done = True

def is_flag(msg):
	return len(msg) == 33

def validip(ip):
	return ip in gameboxs

def handler(signum, stack):
	print("Next Round!")
	for myq in qdict.values():
		if myq.done:
			while not myq.empty():
				myq.get()
			myq.done = False

def putmsg(addr, msg):
	if addr[0] not in qdict:
		qdict[addr[0]] = myqueue(10)
		sendthread(addr[0]).start()
	if not qdict[addr[0]].full():
		qdict[addr[0]].put(msg)

class recvthread(threading.Thread):
	def run(self):
		while not connq.empty():
			self.conn, self.addr = connq.get()
			if not validip(self.addr[0]) or self.addr[0] in qdict and qdict[self.addr[0]].done:
				self.conn.close()
				continue
			self.msg = ""
			while True:
				self.data = self.conn.recv(1024)
				if not self.data:
					break
				else:
					self.msg += self.data.decode()
			self.conn.close()
			if is_flag(self.msg):
				putmsg(self.addr, self.msg)

class sendthread(threading.Thread):
	def __init__(self, ip):
		threading.Thread.__init__(self)
		self.ip = ip

	def run(self):
		self.q = qdict[self.ip]
		while True:
			if not self.q.done:
				self.msg = self.q.get()
				handlemsg(self.ip, self.msg)
			sleep(2)


class myqueue(queue.Queue):
	def __init__(self, arg):
		super(myqueue, self).__init__(arg)
		self.done = False		

if "__main__" == __name__:
	signal.signal(signal.SIGALRM, handler)
	qdict = dict()

	connq = queue.Queue()

	sock = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
	sock.bind(('',listen_port))
	sock.listen(5)

	while True:
		try:
			conn, addr = sock.accept()
		except socket.error as e:
			if e.errno != errno.EINTR:
				raise
			continue
		connq.put((conn, addr))
		#sleep(0.1)
		if not connq.empty():
			recvthread().start()
