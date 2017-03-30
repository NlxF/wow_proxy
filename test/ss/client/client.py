#!_*_coding:utf-8_*_
#!/usr/bin/python

import socket
import json
import crc8
import time
import multiprocessing
import random
from contextlib import closing

HOST = '192.168.1.116'
PORT = 8083
MESSAGE = "Hello world!"

def assembly_request(cmds):
	vals = []
	keys = []
	for cmd in cmds:
		t1 = {}
		t2 = []
		[(t1.update(k), t2.extend(k.keys())) for k in cmd]
		keys.append(t2)
		vals.append(t1)

	if len(vals)==0 or len(keys)==0: 
		return None 
	else:
		return {"values": vals, "keys": keys}


def test_performance(msg):

	# cmd = [[{'op': '1'}, {'name': 'username'}, {'pwd': 'password'}], [{'op': '2'}, {'name': 'username'}]]
    # cmd = [[{'op': '12'}, {'account': 'ads'}, {'name': '20'}]]
	cmd = [[{'op': '13'}, {'role': 'CHRACE;UPLEVEL'}, {'label': u'变种族;升级'}, {'value': '10'}]]
        
	dic = assembly_request(cmd)
	news_str = json.dumps(dic)
	
	c = crc8.crc8()
	crc_str = c.crc8_data(news_str)

	with closing(socket.socket(socket.AF_INET, socket.SOCK_STREAM)) as s:
		s.connect((HOST, PORT))
		s.sendall(crc_str)
		print('send data:%s to lserver' % (dic,))
		
		try:
			s.settimeout(2)
			buf=s.recv(1024)
			print "client recv data: %s" %(c.reverse_crc8_data(buf))
		except socket.timeout as e:
			print("sock time out")
			s.close()
			
	#time.sleep(0.1)           



if __name__ == "__main__":
	#pool = multiprocessing.Pool(processes=1)
	for i in xrange(200):
		msg = "From client %d, hello world!" %(i)
		test_performance(msg)

	print "Sub-process(es) done."
	#msg = 'From client, hello world!'
	#test_performance(msg)
