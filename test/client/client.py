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


def test_performance(msg):
	
	'''news = wowmessage_pb2.news()
	news.needResponse = True #bool(random.randint(0,1))
	news.message = msg
	news.len = len(msg)
	news_str = news.SerializeToString()'''

	#use json
	dic = {"needResponse":True, "message": msg, "len": len(msg)}
	news_str = json.dumps(dic)	
	
	c=crc8.crc8()
	crc_str = c.crc8_data(news_str)

	with closing(socket.socket(socket.AF_INET, socket.SOCK_STREAM)) as s:
		s.connect((HOST, PORT))
		s.sendall(crc_str)
		print('send data:%s to lserver' % (msg,))
		if dic['needResponse']:
			s.settimeout(2)
			buf=s.recv(1024)
			print "client recv data: %s" %(c.reverse_crc8_data(buf))
#time.sleep(0.1)           



if __name__ == "__main__":
#pool = multiprocessing.Pool(processes=1)
	for i in xrange(200):
		msg = "From client %d, hello world!" %(i)
		test_performance(msg)

	print "Sub-process(es) done."
#	msg = 'From client, hello world!'
#	test_performance(msg)
