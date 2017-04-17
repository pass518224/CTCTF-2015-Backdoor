#!/usr/bin/env python3
import socket
import queue
import threading
from sys import argv

class mythread(threading.Thread):
	def run(self):
		while not q.empty():
			self.conn, self.addr = q.get()
			print("Connection from %s: %s"%self.addr)
			self.conn.send(content)
			self.conn.close()

if "__main__" == __name__:
	f = open("malware", "rb")
	content = f.read()
	f.close()

	q = queue.Queue()

	sock = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
	sock.bind(('',listen_port))
	sock.listen(5)

	while True:
		conn, addr = sock.accept()
		q.put((conn, addr))
		if not q.empty():
			mythread().start()
