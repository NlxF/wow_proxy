#!_*_coding:utf-8_*_
#!/usr/bin/python

import socket
import json
import crc8
import time
import multiprocessing
import random
from contextlib import closing

HOST = '10.49.199.41' # '127.0.0.1'
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

	cmd = [[{'op': '1'}, {'name': "{0}{1}".format(account, i)}, {'pwd': '123456'}] for i in range(1)] #, [{'op': '2'}, {'name': 'username'}]]
	# cmd = [[{'op': '12'}, {'account': 'ads'}, {'name': '20'}]]
	# cmd = [[{'op': '13'}, {'role': 'CHRACE;UPLEVEL'}, {'label': u'变种族;升级'}, {'value': '10'}]]
        
	dic = assembly_request(cmd)
	news_str = json.dumps(dic)
	
	c = crc8.crc8()
	crc_str = c.crc8_data(news_str)
	print('length of send str is:%d' % len(crc_str))
	# hex_str = ":".join("{:02x}".format(ord(c)) for c in crc_str)
	# print(hex_str)
	try:
		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM) 
		s.connect((HOST, PORT))
	except Exception as e:
		print(e)
	else:
		loop = 1
		while loop>0:
			try:
				print('send data:%s to lserver' % crc_str)
				send_num = len(crc_str)
				data = bytearray((send_num).to_bytes(2, 'big')) + crc_str[:-1].encode() 
				data.append(ord(crc_str[-1]))
				print(data)
				s.sendall( data )
				print('send data finish')
				s.settimeout(4)
				buf=s.recv(1024)
				print("client recv data: %s" % buf)
				print("client recv data: %s" %(c.reverse_crc8_data(str(buf[2:]))))
			except socket.timeout as e:
				print("sock time out")
			except socket.error as e:
				print("socket error except occure")
			except Exception as e:
				print(e)
			loop -= 1
		print("socket close")
		s.close()
		#time.sleep(0.1)           



if __name__ == "__main__":
	pool_size = multiprocessing.cpu_count() * 2
	pool = multiprocessing.Pool(processes=pool_size)
	for i in range(1):
		name = '{0}{1}'.format('username', i)
		pool.apply_async(test_performance, (name, ))
	pool.close()
	pool.join()
	print("Sub-process(es) done.")
	
