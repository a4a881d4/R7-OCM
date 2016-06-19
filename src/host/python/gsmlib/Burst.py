from fractions import Fraction
import numpy as np

class item:
	def getLen(self):
		if hasattr(self,"field"):
			l = 0
			for x in self.field:
				l += x.getLen()
			return l
		else:
			return self.__class__.length

	@staticmethod
	def gmsk_mapper( inp, start_point ):
		inpb = np.array(inp)*2 - 1
		o = start_point
		out = [o]
		previous_symbol = inpb[0]
		for current_symbol in inpb[1:]:
			encoded_symbol = current_symbol * previous_symbol
			o = complex(0,1)*encoded_symbol*o
			out.append(o)
			previous_symbol = current_symbol
		return np.array(out)
	@staticmethod
	def c2bits(cbits):
		r = []
		for x in cbits:
			r.append(int(x))
		return r

class TB(item):
	length = 3
	bits = [0,0,0]

class ATB(item):
	length = 8
	bits = [0]*length

class NGP(item):
	length = Fraction(33,4)

class AGP(item):
	length = Fraction(273,4)

class Burst:
	length = Fraction(625,4)
	small_overlap = 5
	large_overlap = length/2
	mmap = None

	def __init__(self):
		if hasattr(self.__class__,"__field__"):
			self.field = [ x() for x in self.__class__.__field__]
		else:
			self.field = []
		self.fn = 0
		self.sn = 0
		self.ch = None
	
	def set(self,fn,sn):
		self.fn = fn
		self.sn = sn
		self.pos = Burst.length*(fn*8+sn)

	def getLen(self):
		if hasattr(self,"field"):
			l = 0
			for x in self.field:
				l += x.getLen()
			return l
		else:
			return self.__class__.length

	def dump(self):
		name = [n.__name__ for n in self.__class__.__field__]
		print name

	def attach(self,CH):
		self.ch = CH

	def deattach(self):
		self.ch = None

	def channelEst( self, frame, training, osr ):
		inx = np.floor(np.arange(len(training))*osr)
		last = int(inx[-1]+1)
		out = np.zeros(len(frame)-last,dtype=complex)
		for k in range(len(out)):
			slc = frame[k:]
			s = slc[inx.astype(int)]
			r = np.dot(s,training)
			out[k] = r
		return out
	
	@staticmethod
	def short2Complex(data):
		nframes = len(data)/2
		frame = np.array([complex(data[2*i],data[2*i+1]) for i in range(nframes)])
		return frame
	
	def mapRfData(self):
		if mmap==None:
			raise NoInstall
			return
		s = int(self.pos-Burst.small_overlap)
		l = int(Burst.length+Burst.small_overlap)
		self.recv = Burst.short2Complex(mmap(s,l))

	def mapLData(self):
		if mmap==None:
			raise NoInstall
			return
		s = int(self.pos-Burst.large_overlap)
		l = int(Burst.length+Burst.large_overlap)
		return Burst.short2Complex(mmap(s,l))
		