#!/usr/bin/env python


import sys
import os
import pylab
import numpy
import subprocess
import matplotlib.pyplot as plt


def stats(data):
	sum = 0.0
	for value in data:
		sum =sum+ value
	return (sum/len(data))

def meanstdv(x): 
	from math import sqrt 
	n, mean, std = len(x), 0, 0 
	for a in x:
		mean = mean + a 
	mean = mean / float(n)
	for a in x:
			 std = std + (a - mean)**2
	std = sqrt(std / float(n-1))
	return mean

print sys.argv[1]
code= int(sys.argv[2])
filename=sys.argv[1]


not_increased ="1"

figFormat = "ps"

fig = plt.figure()

#./waf --run "scratch/adhoc 4254 3 5
for k in range(1,4):
	m_results_inc=list() 
	m_results_Ninc= list() 
	m_results_sub= list()
	m_results_sent=list()
	m_relay_sent=list()
	m_sent_source=list()
	m_receieve_relay=list()
	relay_received_activity=list()
	m_receieve_source=list()
	m_time_list=list()
	m_throughput=list()
	s_r=list()
	r_d=list()
	for j in range(1, 100):
        
		results_inc1 =list()
		results_Ninc1 = list()
		results_sub1 = list()
		results_sent1 =list()
		relay_sent1 =list()
		sent_source1 =list()
		receieve_relay1 =list()
		receieve_source1 =list()
		time_list1 = list()
		throughput1 = list()
		for n in range(1, 20):
			f=open(filename,'w')
			distance=(k*250-500)
			cmd = ["./waf","--run", "test --numPackets=1800 --EnableCode=1  --Symbols=30 --seed={} --EnableRencode={} --RelayActivity={} --distance={}".format(n,code,100-j,distance)]
			#print " ".join(cmd)
			print cmd
			print j
			p = subprocess.Popen(cmd, stdout=f, stderr=subprocess.PIPE)
			#print p.communicate()
			p.wait()
			if p.returncode != 0:
				(stdout,stderr) = p.communicate()
				print stderr
				#sys.exit(1)
				continue
			
			f.close()
			f = open(filename, "r")
			#f.flush()
			#f.seek(0)

			increased=0
			not_increased=0

			while True:
				line=f.readline()
				if not line:
					break
				
				line=line.split(":")
				if line[0]=="increased":
					increased = line[1].strip()
				if line[0]=="not increased":
					not_increased = line[1].strip()
				if line[0]=="sent":
					sent=line[1].strip()
				if line[0]=="sent_code":
					sent_code=line[1].strip()
				if line[0]=="recevied_source":
					rsource=line[1].strip()
				if line[0]=="received":
					received=line[1].strip()
				if line[0]=="from_source":
					from_source=line[1].strip()
				if line[0]=="from_relay":
					from_relay=line[1].strip()
				if line[0]=="time":
					time=line[1].strip()
				if line[0]=="received_relay":
					relay_received= line[1].strip()
			f.close()
			print sent 
			print sent_code
			overal=int(sent)+int(sent_code)

			relay_sent1.insert(n,int(sent_code));
			throughput1.insert(n,(float(received)-float(not_increased))/float(overal))
			results_inc1.insert(n,int(increased));
			results_Ninc1.insert(n,int(not_increased)); 
			results_sent1.insert(n,int(overal)); 
			sent_source1.insert(n,float(sent)); 
			results_sub1.insert(n,(int(increased)-int(not_increased)));
			receieve_relay1.insert(n,int(from_relay)); 
			receieve_source1.insert(n,int(from_source)); 
			time_list1.insert(n,float(time));
			relay_received_activity.insert(n,int(relay_received))

		print results_inc1
		print meanstdv(results_inc1)
			
		overal=int(sent)+int(sent_code)
		print "sent code {}" .format(meanstdv(relay_sent1))
		print "source sending activity {} " .format(meanstdv(sent_source1))
		print "relay reciving activity {} " .format(meanstdv(relay_received_activity))

		print "overal sent {}".format(meanstdv(results_sent1))
		print "received from source {}".format(meanstdv(receieve_source1))

		print "source relay loss {} " .format((meanstdv(sent_source1)-meanstdv(relay_received_activity))/meanstdv(sent_source1))
		s_r.insert(j,(meanstdv(sent_source1)-meanstdv(relay_received_activity))/meanstdv(sent_source1))
		print "relay destionation loss {}".format((meanstdv(relay_sent1)-meanstdv(receieve_relay1))/meanstdv(relay_sent1))
		r_d.insert(j,(meanstdv(relay_sent1)-meanstdv(receieve_relay1))/meanstdv(relay_sent1))
		print "source destionation loss {}".format((meanstdv(sent_source1)-meanstdv(receieve_source1))/meanstdv(sent_source1))
		
		print "increased rank {}" .format(meanstdv(results_inc1))
		print "don increacsed {}" .format(meanstdv(results_Ninc1))
		print "received by destinatiin {}" .format(received)
		m_relay_sent.insert(j,meanstdv(relay_sent1));
		print "throupout {}" .format(float(received)/float(overal))
		m_throughput.insert(j,meanstdv(throughput1))
		m_results_inc.insert(j,meanstdv(results_inc1));
		m_results_Ninc.insert(j,meanstdv(results_Ninc1)); 
		m_results_sent.insert(j,meanstdv(results_sent1)); 
		m_sent_source.insert(j,meanstdv(sent_source1)); 
		m_results_sub.insert(j,meanstdv(results_sub1));
		m_receieve_relay.insert(j,meanstdv(receieve_relay1)); 
		m_receieve_source.insert(j,meanstdv(receieve_source1)); 
		m_time_list.insert(j,meanstdv(time_list1));
		try:
			result=float(increased)/float(not_increased);
			
		except ZeroDivisionError:
			print "Don't divide by zero :)"
			continue

		
	print len(m_results_sent)
	#x = numpy.array(range(10))
	#y = numpy.array(range(10))
	t = numpy.arange(0.01, 1, 0.01)

	print len(t)
	print m_results_inc	
	print "hi"
	print t
	ax1 = fig.add_subplot(331)
	ax1.plot(t,m_results_inc ,label='innovative packets')
	ax1.plot(t,m_results_Ninc,label='non-innovative packets')
	ax1.plot(t,m_results_sub,label='innovative - non-innovative')

	#handles, labels = ax1.get_legend_handles_labels()
	#ax0.legend(handles, labels,loc='lower left', shadow=True)
	plt.xlabel('probability of sending the packet by relay')
	plt.ylabel('Number of packets')
	ax1.legend(loc='upper left', shadow=True)
	ax2 = fig.add_subplot(332)
	ax2.plot(t,m_results_sent)
	ax1.grid(True)
	ax2.grid(True)
	plt.xlabel('probability of sending the packet by relay')
	plt.ylabel('number of sent packets by source and relay ')
	ax2.legend(loc='upper left', shadow=True)
	ax3=fig.add_subplot(333)
	ax3.plot(t,m_throughput)
	plt.xlabel('probability of sending the packet by relay')
	plt.title('Throghput')
	ax4=fig.add_subplot(334)
	ax4.plot(t,m_sent_source)
	plt.xlabel('probability of sending the packet by relay')
	plt.title('source sending activity')
	ax5=fig.add_subplot(335)
	ax5.plot(t,m_relay_sent)
	plt.xlabel('probability of sending the packet by relay')
	plt.title('relay sending activity')
	ax6=fig.add_subplot(336)
	x = numpy.array(m_sent_source)
	y = numpy.array(m_receieve_source)
	ax6.plot(t,numpy.divide(numpy.subtract(x,y),x))
	plt.xlabel('probability of sending the packet by relay')
	plt.title('number of lost packets sent by source to destinatiion')
	ax7=fig.add_subplot(337)
	ax7.plot(t,s_r)
	plt.xlabel('source relay loss')
	plt.xlabel('probability of sending the packet by relay')
	plt.title('number of lost packets sent by source to relay')
	ax8=fig.add_subplot(338)
	ax8.plot(t,r_d)
	plt.xlabel('relay destination loss')
	print numpy.subtract(x,y)
	plt.xlabel('probability of sending the packet by relay')
	plt.title('number of lost packets sent by relay to destinatiion')
	ax3.grid(True)
	ax4.grid(True)
	ax5.grid(True)
	ax6.grid(True)

	fig2 = plt.figure()
	ax7=fig2.add_subplot(111)

	ax7.plot(t,m_time_list)

	plt.xlabel('probability of sending the packet by relay')
	plt.title('Delay')

fig.savefig('ax1_plot.{}'.format(figFormat), bbox_inches='tight', pad_inches=0.01)



pylab.show()

