#!/usr/bin/python2
# A simulator for Frond behaviours

from __future__ import nested_scopes
from random import random, randint
from math import sqrt

twopi = (3.14159265358979323846+3.14159265358979323846)

class Frond:
    def __init__(self):
        self.__leds = [ 0 for x in range(0,16) ]
        self._input = 0
        self.randstate = randint(0, 65535)
        if self.randstate == 0:
            self.randstate = 0xcf5
        
    def rand(self):
        r = self.randstate
        if r & 1:
            r >>= 1
            r = r ^ 0x8016
        else:
            r >>= 1

        self.randstate = r
        return r

    def setled(self, led, val):
        val = int(val)
        if val < 0:
            val = 0
        if val > 16:
            val = 16

        self.__leds[led] = val

    def showleds(self):
        if 1:
            dens = " '.,~/-+=^$!&*%@#"
            ret = "".join([ dens[x] for x in self.__leds ])
        else:
            ret = [ "%2d " % v for v in self.__leds ]
        sum = reduce(lambda a, b: a+b, self.__leds)
        return "".join(ret) + "      = %3d (%2.2g)" % (sum, sum / (16.0*15))
        
    def state(self):
        print "state = ", self.__leds[0]
        return self.__leds[0]

    def input(self, val):               # override to inject new state
        pass

    def next(self):                     # override to generate next state
        pass


class FadeFrond(Frond):
    def __init__(self):
        Frond.__init__(self)

        self.__state = [ 0 for x in range(0,16) ]

    def next(self):
        self.__state = [ max(0, x - 1) for x in self.__state ]
        self.__state[self.rand() & 0xf] = 15
        for i in range(0, 16):
            self.setled(i, self.__state[i])

class FlameFrond(Frond):
    darkt = 20
    lightt = 64
    
    def __init__(self):
        Frond.__init__(self)

        self.__state = [ 0 for x in range(0,16) ]
        self.dark = 1
        
    def next(self):
        s = []
        w = [ 1./4, 1./2, 1./16 ]
        wl = len(w)
        wo = int(wl / 2)
        #print "totw=%g" % reduce(lambda a, b: a+b, w)
        sum = 0
        for i in range(0,16):
            wsum = 0
            for idx in range(- wo, wo + 1):
                ii = i + idx * 2
                if ii & ~15 == 0:
                    wsum += self.__state[(i + idx * 1) & 15] * w[wo + idx]
                #print "i=%d idx=%d wo=%d wo+idx=%d i+idx=%d wsum=%d" % (i, idx, wo, wo + idx, i+idx, wsum)
            s.append(wsum)
            sum += wsum / 16
        self.__state = s
        if (sum < self.darkt):
            self.dark = 1

        if (sum > self.lightt):
            self.dark = 0

        if self.dark or self.rand() & 0xff < 16:
            loops = int(abs(self.lightt - sum) / 4 + 1) & 15
            #print "sum=%d dark=%d, loops=%d" % (sum, self.dark, loops)
        
            r = self.rand() & 0xf
            for i in range(0, loops):
                ii = (r+i) & 15
                self.__state[ii] = min(255, self.__state[ii] + 250)
            
        for i in range(0, 16):
            self.setled(i, self.__state[i] / 16)
    
# f = FlameFrond()

# for i in range(0,10000):
#     f.next()
#     print "%3d: %s" % (i, f.showleds())


# Frond topology
#
# Frond network has a probabilistic nature; some fronds will reliably
# see others; others will have intermittent connections and others
# will be completely disconnected.
#
# Reliability of data transmission depends on number of simultaneous
# signals visible at a receiver.  The more there are, the higher the
# likelyhood of error.  I'm assuming that even under heavy contention,
# the IR receiver will generate output signals on a semi-regular
# basis, even if they don't correlate to any particular input signal
# (or if the carrier is barely recogizable).
#
# The model is therefore a topology map which contains the signal
# strength weight between each frond.  A small number (ideally 1) of
# simultaneously sending strong signals will get the most reliable
# signal transmitted.  A large number of weak signals will generate
# noise (or perhaps nothing at all).
#
# Self-stimulation can readily occur, so there is a feedback term.

class FrondTopo:
    def __init__(self):
        self.frondmap={}

    def key(self, a, b):
        if a < b:
            return `a`+`b`
        else:
            return `b`+`a`

    def set(self, a, b, w):
        if not a in self.frondmap.keys():
            self.frondmap[a] = {}
        self.frondmap[a][b] = w

        if 0:                           # bidir links
            if not b in self.frondmap.keys():
                self.frondmap[b] = {}
            self.frondmap[b][a] = w

    def get(self, a, b):
        if not a in self.frondmap.keys() or \
           not b in self.frondmap[a].keys():
            return 0
        return self.frondmap[a][b]
        
    def connect(self, f1, f2, w):
        w = max(0, min(1, w))           # clamp to 0..1
        self.set(f1, f2, w)
        #print "connect ", f1, "->", f2, " = ", w

    def weight(self, sender, to):
        return self.get(sender, to)
    
    def signal(self, senders, to):
        """Work out how a signal propagates from a set of input fronds
        to a particular target.  senders is an array of (sender, value)
        tuples, and to is the receiver (which may also be in the sender
        set).  Returns the received value, which is randomized
        depending on the signal reception conditions"""
        w = 0

        # for now, use weighted average with no randomness
        for (f, v) in senders:
            w += self.weight(f, to) * v
        return w

topo = FrondTopo()

def dot(a, b):
    return a[0] * b[0] + a[1] * b[1]
def dist(a, b):
    dx = a[0]-b[0]
    dy = a[1]-b[1]
    return sqrt(dx*dx+dy*dy)

class FrondClump:
    def __init__(self):
        self.fronds = []                # array of (x, y, angle, frond) tuples

    def placeFrond(self,gen):
        "Place a single frond in the clump"
        self.fronds.append((random()-.5, random()-.5, random()*twopi, gen()))

    def calcVisibility(self, topo):
        "Go and compute all the frond visibility and put it into the topology"

        for (ax, ay, aa, af) in self.fronds:
            for (bx, by, ba, bf) in self.fronds:
                if af == bf:
                    topo.connect(af, af, .1) # feedback term
                else:
                    # omnidirectonal for now
                    d = dist((ax, ay), (bx, by)) + 1
                    w = 1/(d*d)                 # inverse square
                    topo.connect(af, bf, w)
                
    def makeClump(self,topo,gen):
        "Make a strongly connected clump of fronds"

        for i in range(0,8):
            self.placeFrond(gen)

        self.calcVisibility(topo)

    def getFronds(self):
        return [ f for (x, y, a, f) in self.fronds ]
    
clump = FrondClump()
clump.makeClump(topo, lambda : FadeFrond())

class FrondSim:
    def __init__(self, topo, fronds):
        self.topo = topo
        self.fronds = fronds

    def runsim(self):
        for f in self.fronds:
            f.next()

        signals = [ (f, f.state() / 15.) for f in self.fronds ]

        for f in self.fronds:
            s = int(self.topo.signal(signals, f) * 16)
            print s, " -> ", f
            f.input(s)
        
sim = FrondSim(topo, clump.getFronds())
for i in range(0, 10):
    sim.runsim()
