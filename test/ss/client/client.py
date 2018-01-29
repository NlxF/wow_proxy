#!_*_coding:utf-8_*_
#!/usr/bin/python

import socket
import json
import crc8
import time
import multiprocessing
import random
from contextlib import closing

HOST = '127.0.0.1'
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
		return None
	else:
		return {"values": vals, "keys": keys}


def test_performance(account):

	cmd = [[{'op': '1'}, {'name': account}, {'pwd': '123456'}]] #, [{'op': '2'}, {'name': 'username'}]]
	# cmd = [[{'op': '12'}, {'account': 'ads'}, {'name': '20'}]]
	# cmd = [[{'op': '13'}, {'role': 'CHRACE;UPLEVEL'}, {'label': u'变种族;升级'}, {'value': '10'}]]
        
	dic = assembly_request(cmd)
	news_str = json.dumps(dic)
	
	c = crc8.crc8()
	crc_str = c.crc8_data(news_str)

	with closing(socket.socket(socket.AF_INET, socket.SOCK_STREAM)) as s:
		s.connect((HOST, PORT))
		loop = 1
		while loop>0:
			s.sendall(crc_str)
			print('send data:%s to lserver' % (dic,))
			
			try:
				s.settimeout(4)
				buf=s.recv(1024)
				print "client recv data: %s" %(c.reverse_crc8_data(buf))
			except socket.timeout as e:
				print("sock time out")
				s.close()
			loop -= 1
			
	#time.sleep(0.1)           



if __name__ == "__main__":
	pool_size = multiprocessing.cpu_count() * 2
	pool = multiprocessing.Pool(processes=pool_size)
	for i in xrange(5):
		name = '{0}{1}'.format('usernameee', i)
		pool.apply_async(test_performance, (name, ))
	pool.close()
	pool.join()
	print "Sub-process(es) done."
	
