"""fluxml Python routines.

This module contains a set of functions shared among the Python scripts
coming with the fluxml distribution.

Author: Michael Weitzel <info@13cflux.net>
Author: Salah Azzouzi   <info@13cflux.net>
"""

import sys
import os
import re

fml_ns = 'http://www.13cflux.net/fluxml'
fwd_ns = 'http://www.13cflux.net/fwdsim'
mml_ns = 'http://www.w3.org/1998/Math/MathML'

def log2(n):
	"""computes the base 2 logarithm of argument n"""
	l = 0
	while n > 1:
		n = n >> 1
		l = l + 1
	return l

def isPwr2(n):
	"""returns true iff argument n is a power of two"""
	return (1<<log2(n)) == n

def cumulate(v,d=1):
	"""for d==1, performs the fast isotopopmer-to-cumomer transformation
	on input vector v. For d==-1, performs the inverse transform, i.e.
	transforms the given cumomer vector back into isotopomer space. It is
	assumed that isPwr2(len(v))==True and that d is either 1 or -1."""
	n = len(v)
	ldm = log2(n)
	while ldm >= 1:
		m = (1<<ldm)
		mh = (m>>1)
		for r in range(0,n,m):
			for j in range(0,mh):
				s = n-r-j-1
				t = s-mh
				v[t] = v[t] + d * v[s]
		ldm = ldm-1
	return v

def gcd(u,v):
	"""returns the greatest common divisor of u and v, i.e. GCD(u,v)"""
	k = 0
	# Ergebnis ist immer >=0
	if u < 0:
		u = -u
	if v < 0:
		v = -v
	# 0 hat beliebige Teiler; der groesste Teiler einer von 0
	# verschiedenen Zahl ist die Zahl selbst. GCD(0,0) ist undefiniert
	# bzw. die 0:
	if u == 0 or v == 0:
		return u | v
	# finde 2er-Potenz
	while ~(u|v)&1:
		k = k + 1
		u = u >> 1
		v = v >> 1

	if u & 1:
		t = -v
	else:
		t = u >> 1
	while ~t & 1:
		t = t >> 1
	if t > 0:
		u = t
	else:
		v = -t
	t = u - v
	while t != 0:
		t = t >> 1
		while ~t & 1:
			t = t >> 1
		if t > 0:
			u = t
		else:
			v = -t
		t = u - v
	return u << k

def bin_coeff(n,k):
	"""returns the binomial coefficient 'n over k'"""
	if n < k:
		return 0
	if k == 0 or n == k:
		return 1
	d = gcd(n,k)
	return (bin_coeff(n-1,k-1) // (k//d)) * (n//d)

def mask_to_range(mask):
	"""converts a boolean mask into a range string,
	e.g. [True,True,True,False,True] -> '1-3,5'"""
	rangespec = []
	i = 0
	while i < len(mask):
		if not mask[i]:
			i = i+1
			continue
		j = i+1
		while j < len(mask) and mask[j]:
			j = j+1
		j = j-1

		if i != j:
			rangespec.append('%i-%i' % (i+1,j+1))
		else:
			rangespec.append('%i' % (i+1))
		i = j+1

	return ','.join(rangespec)

def range_to_mask(rangespec,atoms = 0):
	"""converts a range string, e.g. '1-3,5' into a boolean mask,
	   e.g. [True,True,True,False,True]. Does not perform error checking"""
	speclst = rangespec.split(',')
	prspec = set()
	for spec in speclst:
		lohi = spec.split('-')
		lohi = [int(k) for k in lohi]
		#(lo,hi) = (lohi[0],lohi[1] if len(lohi)>1 else lohi[0])
		if len(lohi) > 1: (lo,hi) = (lohi[0],lohi[1])
		else: (lo,hi) = lohi[0]
		for k in range(lo,hi+1):
			prspec.add(k)

	if atoms == 0: atoms = max(prspec)
	mask = [False for k in range(atoms)]
	for t in prspec: mask[t] = True
	return mask

def which(fn):
	"""the functionality of the which shell command"""
	if 'PATH' not in os.environ or os.environ['PATH'] == '':
		p = os.defpath
	else:
		p = os.environ['PATH']
	pathlist = p.split(os.pathsep)
	for path in pathlist:
		f = os.path.join(path, fn)
		if os.access(f, os.X_OK):
			return f
	return None

def info(msg):
	"""prints an info message"""
	if os.isatty(sys.stderr.fileno()):
		print >>sys.stderr, ('\033[39m' + msg + '\033[0m')
	else:
		print >>sys.stderr, ('I: ' + msg)


def notice(msg):
	"""prints a notice"""
	if os.isatty(sys.stderr.fileno()):
		print >>sys.stderr, ('\033[32m' + msg + '\033[0m')
	else:
		print >>sys.stderr, ('N: ' + msg)

def warning(msg):
	"""prints a warning message"""
	if os.isatty(sys.stderr.fileno()):
		print >>sys.stderr, ('\033[33mWarning: ' + msg + '\033[0m')
	else:
		print >>sys.stderr, ('W: ' + msg)

def error(msg):
	"""prints an error message"""
	if os.isatty(sys.stderr.fileno()):
		print >>sys.stderr, ('\033[31mError: ' + msg + '\033[0m')
	else:
		print >>sys.stderr, ('E: ' + msg)

# -------------------------------------------------------------------------

class MGroup:
	"""A base class for representing a measurement configuration"""
	gid = ''
	spec = ''
	autoscale = True
	values = []
	stddev = []

	def __init__(self, gid, spec):
		self.gid = gid
		self.spec = spec
		self.values = []
		self.stddev = []

	def set(self, idx, value, stddev = None):
		self.values[idx] = value
		if stddev != None: self.stddev[idx] = stddev

	def get(self, idx = None):
		if idx == None:
			return self.values
		return self.values[idx]

	def getStdDev(self, idx = None):
		if idx == None:
			return self.stddev
		return self.stddev[idx]


class MGroupMS(MGroup):
	"""A mass spectrometry measurement configuration"""
	metabolite = ''
	atomrange = ''
	weights = []

	def __init__(self, gid, spec):
		MGroup.__init__(self, gid, spec)
		reMS = re.compile(r'([A-Za-z_]([A-Za-z_]|\d)*)(\[((\d+|\d+-\d+)(,(\d+|\d+-\d+))*)\])?#M(\d+(,\d+)*)')
		match = reMS.match(self.spec)
		if not match: raise TypeError
		m = match.groups()
		self.metabolite = m[0]
		self.atomrange = m[3]
		self.weights = []
		for w in m[7].split(','):
			self.weights.append(int(w))
		self.values = [ 0. for x in range(len(self)) ]
		self.stddev = [ None for x in range(len(self)) ]

	def __len__(self):
		return len(self.weights)

	def __getitem__(self, key):
		if key < 0 or key >= len(self):
			raise IndexError
		return { 'id':self.gid,'weight':self.weights[key] }
	
	def getSpec(self, idx = None):
		if idx != None:
			if idx < 0 or idx >= len(self):
				raise IndexError
			rangespec = ''
			if self.atomrange:
				rangespec = '[%s]' % self.atomrange
			return '%s%s#M%i' % (self.metabolite,
				rangespec, self.weights[idx])
		else: return self.spec
	
	def get(self, key = None):
		if key == None or type(key) == type(int()):
			return MGroup.get(self, key)
		if type(key) != type({}) or not ('weight' in key):
			raise IndexError
		w = int(key['weight'])
		for i in range(len(self.weights)):
			if self.weights[i] == w:
				return self.values[i]
		raise IndexError
	
	def set(self, key, value, stddev = None):
		if type(key) == type(int()):
			MGroup.set(self, key, value, stddev)
			return
		if type(key) != type({}) or not ('weight' in key):
			raise IndexError
		w = int(key['weight'])
		for i in range(len(self.weights)):
			if self.weights[i] == w:
				self.values[i] = value
				if stddev != None: self.stddev[i] = stddev
				return
		raise IndexError

class MGroupMSMS(MGroup):
	"""A MS/MS measurement configuration"""
	metabolite = ''
	atomrange1 = ''
	atomrange2 = ''
	weights1 = []
	weights2 = []

	def __init__(self, gid, spec):
		MGroup.__init__(self, gid, spec)
		reMSMS = re.compile(r'([A-Za-z_]([A-Za-z_]|\d)*)(\[((\d+|\d+-\d+)(,(\d+|\d+-\d+))*):((\d+|\d+-\d+)(,(\d+|\d+-\d+))*)\])?#M(\(\d+,\d+\)(,\(\d+,\d+\))*)')
		match = reMSMS.match(self.spec)
		if not match: raise TypeError
		m = match.groups()
		self.metabolite = m[0]
		self.atomrange1 = m[3]
		self.atomrange2 = m[7]
		self.weights1 = []
		self.weights2 = []
		wp = m[11][1:]
		for w in wp[:len(wp)-1].split('),('):
			(w1,w2) = w.split(',')
			self.weights1.append(int(w1))
			self.weights2.append(int(w2))
		self.values = [ 0. for x in range(len(self)) ]
		self.stddev = [ None for x in range(len(self)) ]

	def __len__(self):
		return len(self.weights1)

	def __getitem__(self, key):
		if key < 0 or key >= len(self):
			raise IndexError
		return { 'id':self.gid,'weights1':self.weights1[key],
			'weights2':self.weights2[key] }

	def getSpec(self, idx = None):
		if idx != None:
			if idx < 0 or idx >= len(self):
				raise IndexError
			rangespec = ''
			if self.atomrange1:
				rangespec = '[%s:%s]' % (self.atomrange1,self.atomrange2)
			return '%s%s#M(%i,%i)' % (self.metabolite, rangespec,
				self.weights1[idx], self.weights2[idx])
		else: return self.spec

	def get(self, key = None):
		if key == None or type(key) == type(int()):
			return MGroup.get(self, key)
		if type(key) != type({}) or not ('weight' in key):
			raise IndexError
		reW = re.compile(r'(\d+)\s*,\s*(\d+)')
		match = reW.match(key['weight'])
		if not match: raise IndexError
		m = match.groups()
		(w1,w2) = (int(m[0]),int(m[1]))
		for i in range(len(self.weights1)):
			if self.weights1[i] == w1 and self.weights2[i] == w2:
				return self.values[i]
		raise IndexError
	
	def set(self, key, value, stddev = None):
		if type(key) == type(int()):
			MGroup.set(self, key, value, stddev)
			return
		if type(key) != type({}) or not ('weight' in key):
			raise IndexError
		reW = re.compile(r'(\d+)\s*,\s*(\d+)')
		match = reW.match(key['weight'])
		if not match: raise IndexError
		m = match.groups()
		(w1,w2) = (int(m[0]),int(m[1]))
		for i in range(len(self.weights1)):
			if self.weights1[i] == w1 and self.weights2[i] == w2:
				self.values[i] = value
				if stddev != None: self.stddev[i] = stddev
				return
		raise IndexError


class MGroup1HNMR(MGroup):
	"""A 1H-NMR measurement configuration"""
	metabolite = ''
	positions = []

	def __init__(self, gid, spec):
		MGroup.__init__(self, gid, spec)
		re1HNMR = re.compile(r'([A-Za-z_][A-Za-z0-9_]*)#P(\d+(,P?\d+)*)')
		match = re1HNMR.match(self.spec)
		if not match: raise TypeError
		m = match.groups()
		self.metabolite = m[0]
		self.positions = []
		for p in m[1].split(','):
			if p[0] == 'P':
				self.positions.append(int(p[1:]))
			else:
				self.positions.append(int(p))
		self.values = [ 0. for x in range(len(self)) ]
		self.stddev = [ None for x in range(len(self)) ]

	def __len__(self):
		return len(self.positions)
	
	def __getitem__(self, key):
		if key < 0 or key >= len(self):
			raise IndexError
		return { 'id':self.gid,'pos':self.positions[key] }

	def getSpec(self, idx = None):
		if idx != None:
			if idx < 0 or idx >= len(self):
				raise IndexError
			return '%s#P%i' % (self.metabolite,self.positions[idx])
		else: return self.spec
	
	def get(self, key = None):
		if key == None or type(key) == type(int()):
			return MGroup.get(self, key)
		if type(key) != type({}) or not ('pos' in key):
			raise IndexError
		p = int(key['pos'])
		for i in range(len(self.positions)):
			if self.positions[i] == p:
				return self.values[i]
		raise IndexError
	
	def set(self, key, value, stddev = None):
		if type(key) == type(int()):
			MGroup.set(self, key, value, stddev)
			return
		if type(key) != type({}) or not ('pos' in key):
			raise IndexError
		p = int(key['pos'])
		for i in range(len(self.positions)):
			if self.positions[i] == p:
				self.values[i] = value
				if stddev != None: self.stddev[i] = stddev
				return
		raise IndexError

class MGroup13CNMR(MGroup):
	"""A 13C NMR measurement configuration"""
	metabolite = ''
	types = []
	positions = []

	def __init__(self, gid, spec):
		MGroup.__init__(self, gid, spec)
		re13CNMR = re.compile(r'^([A-Za-z_][A-Za-z0-9_]*)#(((S|DL|DR|DD|T)\d+(,\d+)*)(,((S|DL|DR|DD|T)\d+(,\d+)*))*)$')
		match = re13CNMR.match(self.spec)
		if not match: raise TypeError
		m = match.groups()
		self.metabolite = m[0]
		reT = re.compile(r'(S|DL|DR|DD|T)?(\d+)')
		t = None
		self.positions = []
		self.types = []
		for p in m[1].split(','):
			match = reT.match(p)
			if not match: raise TypeError
			m = match.groups()
			if m[0]: t = m[0]
			self.types.append(t)
			self.positions.append(int(m[1]))
		self.values = [ 0. for x in range(len(self)) ]
		self.stddev = [ None for x in range(len(self)) ]

	def __len__(self):
		return len(self.positions)
		
	def __getitem__(self, key):
		if key < 0 or key >= len(self):
			raise IndexError
		return { 'id':self.gid,'type':self.types[key],'pos':self.positions[key] }
	
	def getSpec(self, idx = None):
		if idx != None:
			if idx < 0 or idx >= len(self):
				raise IndexError
			return '%s#%s%i' % (self.metabolite, self.types[idx],
				self.positions[idx])
		else: return self.spec
	
	def get(self, key = None):
		if key == None or type(key) == type(int()):
			return MGroup.get(self, key)
		if type(key) != type({}) or not ('pos' in key and 'type' in key):
			raise IndexError
		p = int(key['pos'])
		t = key['type']
		for i in range(len(self.positions)):
			if self.positions[i] == p and self.types[i] == t:
				return self.values[i]
		raise IndexError
	
	def set(self, key, value, stddev = None):
		if type(key) == type(int()):
			MGroup.set(self, key, value, stddev)
			return
		if type(key) != type({}) or not ('pos' in key and 'type' in key):
			raise IndexError
		p = int(key['pos'])
		t = key['type']
		for i in range(len(self.positions)):
			if self.positions[i] == p and self.types[i] == t:
				self.values[i] = value
				if stddev != None: self.stddev[i] = stddev
				return
		raise IndexError

class MGroupGeneric(MGroup):
	"""A generic measurement configuration"""
	rows = []

	def __init__(self, gid, spec):
		MGroup.__init__(self, gid, spec)
		self.spec = self.spec.rstrip(';')
		self.rows = self.spec.split(';')
		for k in range(len(self.rows)):
			self.rows[k] = self.rows[k].strip()
		self.values = [ 0. for x in range(len(self)) ]
		self.stddev = [ None for x in range(len(self)) ]
	
	def __len__(self):
		return len(self.rows)

	def __getitem__(self, key):
		if key < 0 or key >= len(self):
			raise IndexError
		return { 'id':self.gid,'rowspec':self.rows[key] }

	def getSpec(self, idx = None):
		if idx != None:
			if idx < 0 or idx >= len(self):
				raise IndexError
			return self.rows[idx]
		else: return self.spec

	def get(self, key = None):
		if key == None or type(key) == type(int()):
			return MGroup.get(self, key)
		if type(key) != type({}) or not ('row' in key):
			raise IndexError
		r = int(key['row'])
		return self.values[r-1]
	
	def set(self, key, value, stddev = None):
		if type(key) == type(int()):
			MGroup.set(self, key, value, stddev)
			return
		if type(key) != type({}) or not ('row' in key):
			raise IndexError
		r = int(key['row'])
		self.values[r-1] = value
		if stddev != None: self.stddev[r-1] = stddev

class MGroupFlux(MGroup):
	"""Flux measurement configuration"""
	netflux = None

	def __init__(self, gid, spec, netflux):
		MGroup.__init__(self, gid, spec)
		self.values = [ 0 ]
		self.stddev = [ None ]
		assert type(netflux) == type(True)
		self.netflux = netflux
	
	def __len__(self):
		return 1

	def __getitem__(self, key):
		if key < 0 or key >= len(self):
			raise IndexError
		return { 'id':self.gid,'fluxes':self.spec }

	def getSpec(self, idx = None):
		if idx != None:
			if idx < 0 or idx >= len(self):
				raise IndexError
		#return ('net:' if self.netflux else 'xch:') + self.spec
		if self.netflux: return 'net:' + self.spec
		else: return 'xch:' + self.spec

	def get(self, key = None):
		if key == None:
			return MGroup.get(self, key)
		return MGroup.get(self,0)

	def set(self, key, value, stddev = None):
		MGroup.set(self, 0, value, stddev)

def parseMGroup(gid, spec):
	"""High level method for parsing a FluxML measurement specification"""
	if spec.find(';') >= 0:
		return MGroupGeneric(gid, spec)
	
	try: return MGroupMS(gid, spec)
	except TypeError: pass
	try: return MGroupMSMS(gid, spec)
	except TypeError: pass
	try: return MGroup1HNMR(gid, spec)
	except TypeError: pass
	try: return MGroup13CNMR(gid, spec)
	except TypeError: pass
	# fallback
	return MGroupGeneric(gid, spec)

