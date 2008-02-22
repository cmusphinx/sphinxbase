# Copyright (c) 2007 Carnegie Mellon University
#
# You may copy and modify this freely under the same terms as
# Sphinx-III

"""
Word lattices for speech recognition.

Includes routines for loading lattices in Sphinx3 and HTK format,
searching them, and calculating word posterior probabilities.
"""

__author__ = "David Huggins-Daines <dhuggins@cs.cmu.edu>"
__version__ = "$Revision: 7544 $"

import gzip
import re
import math
import numpy

LOGZERO = -100000

def logadd(x,y):
    """
    For M{x=log(a)} and M{y=log(b)}, return M{z=log(a+b)}.

    @param x: M{log(a)}
    @type x: float
    @param y: M{log(b)}
    @type y: float
    @return: M{log(a+b)}
    @rtype: float
    """
    if x < y:
        return logadd(y,x)
    if y == LOGZERO:
        return x
    else:
        return x + math.log(1 + math.exp(y-x))

def is_filler(sym):
    """
    Returns true if C{sym} is a filler word.
    @param sym: Word string to test
    @type sym: string
    @return: True if C{sym} is a filler word (but not <s> or </s>)
    @rtype: boolean
    """
    if sym == '<s>' or sym == '</s>': return False
    return ((sym[0] == '<' and sym[-1] == '>') or
            (sym[0] == '+' and sym[-1] == '+'))

class Dag(list):
    """
    Directed acyclic graph representation of a phone/word lattice.
    
    This data structure is represented as a list, with one entry per
    frame of audio.  Each entry in the list contains a dictionary
    mapping word names to lattice nodes.
    """
    __slots__ = 'start', 'end', 'header', 'frate'

    class Node(object):
        """
        Node in a DAG representation of a phone/word lattice.

        @ivar sym: Word corresponding to this node.  All arcs out of
                   this node represent hypothesized instances of this
                   word starting at frame C{entry}.
        @type sym: string
        @ivar entry: Entry frame for this node.
        @type entry: int
        @ivar exits: List of arcs out of this node.
        @type exits: list of (int, object)
        @ivar score: Viterbi (or other) score for this node, used in
                     bestpath calculation.
        @type score: int
        @ivar prev: Backtrace pointer for this node, used in bestpath
                    calculation.  Alternately used to store list of
                    predecessors in forward-backward calculation.
        @type prev: object
        """
        __slots__ = 'sym', 'entry', 'exits', 'score', 'prev'
        def __init__(self, sym, entry):
            self.sym = sym
            self.entry = entry
            self.exits = []
            self.score = LOGZERO
            self.prev = None
        def __str__(self):
            return "(%s, %d, %s)" % (self.sym, self.entry, self.exits)

    def __init__(self, sphinx_file=None, htk_file=None, frate=100):
        """
        Construct a DAG, optionally loading contents from a file.

        @param frate: Number of frames per second.  This is important
                      when loading HTK word graphs since times in them
                      are specified in decimal.  The default is
                      probably okay.
        @type frate: int
        @param sphinx_file: Sphinx-III format word lattice file to
                            load (optionally).
        @type sphinx_file: string
        @param htk_file: HTK SLF format word lattice file to
                         load (optionally).
        @type htk_file: string
        """
        self.frate = frate
        if sphinx_file != None:
            self.sphinx2dag(sphinx_file)
        elif htk_file != None:
            self.htk2dag(htk_file)

    fieldre = re.compile(r'(\S+)=(?:"((?:[^\\"]+|\\.)*)"|(\S+))')
    def htk2dag(self, htkfile):
        """Read an HTK-format lattice file to populate a DAG."""
        fh = gzip.open(htkfile)
        del self[0:len(self)]
        self.header = {}
        state='header'
        for spam in fh:
            if spam.startswith('#'):
                continue
            fields = dict(map(lambda (x,y,z): (x, y or z), self.fieldre.findall(spam.rstrip())))
            if 'N' in fields:
                nnodes = int(fields['N'])
                nodes = [None] * nnodes
                nlinks = int(fields['L'])
                links = [None] * nlinks
                state = 'items'
            if state == 'header':
                self.header.update(fields)
            else:
                if 'I' in fields:
                    frame = int(float(fields['t']) * self.frate)
                    node = self.Node(fields['W'], frame)
                    nodes[int(fields['I'])] = node
                    while len(self) <= frame:
                        self.append({})
                    self[frame][fields['W']] = node
                elif 'J' in fields:
                    # Link up existing nodes
                    fromnode = int(fields['S'])
                    tonode = int(fields['E'])
                    tofr = nodes[fromnode].entry
                    ascr = float(fields['a'])
                    lscr = float(fields['n'])
                    # FIXME: Not sure if this is a good idea
                    if not (tofr,ascr) in nodes[int(fromnode)].exits:
                        nodes[int(fromnode)].exits.append((nodes[int(tonode)].entry, ascr))
        # FIXME: Not sure if the first and last nodes are always the start and end?
        self.start = nodes[0]
        self.end = nodes[-1]

    headre = re.compile(r'# (-\S+) (\S+)')
    def sphinx2dag(self, s3file):
        """Read a Sphinx-III format lattice file to populate a DAG."""
        fh = gzip.open(s3file)
        del self[0:len(self)]
        self.header = {}
        state = 'header'
        logbase = math.log(1.0001)
        for spam in fh:
            spam = spam.rstrip()
            m = self.headre.match(spam)
            if m:
                arg, val = m.groups()
                self.header[arg] = val
                if arg == '-logbase':
                    logbase = math.log(float(val))
            if spam.startswith('#'):
                continue
            else:
                fields = spam.split()
                if fields[0] == 'Frames':
                    for i in range(0,int(fields[1]) + 1):
                        self.append({})
                elif fields[0] == 'Nodes':
                    state='nodes'
                    nnodes = int(fields[1])
                    nodes = [None] * nnodes
                elif fields[0] == 'Initial':
                    state = 'crud'
                    self.start = nodes[int(fields[1])]
                elif fields[0] == 'Final':
                    self.end = nodes[int(fields[1])]
                elif fields[0] == 'Edges':
                    state='edges'
                elif fields[0] == 'End':
                    state='done'
                else:
                    if state == 'nodes':
                        nodeid, word, sf, fef, lef = fields
                        node = self.Node(word, int(sf))
                        nodes[int(nodeid)] = node
                        self[int(sf)][word] = node
                    elif state == 'edges':
                        fromnode, tonode, ascr = fields
                        ascr = float(ascr) * logbase
                        tofr = nodes[int(tonode)].entry
                        # FIXME: Not sure if this is a good idea
                        if not (tofr,ascr) in nodes[int(fromnode)].exits:
                            nodes[int(fromnode)].exits.append((tofr, ascr))

    def edges(self, node, lm=None):
        """
        Return a generator for the set of edges exiting node.
        @param node: Node whose edges to iterate over.
        @type node: Dag.Node
        @param lm: Language model to use for scoring edges.
        @type lm: sphinx.arpalm.ArpaLM (or equivalent)
        @return: Tuple of (successor, exit-frame, acoustic-score, language-score)
        @rtype: (Dag.Node, int, int, int)
        """
        for frame, score in node.exits:
            for next in self[frame].itervalues():
                if lm:
                    # Build history for LM score if it exists
                    if node.prev:
                        syms = [next.sym, node.sym]
                        prev = node.prev
                        for i in range(2,lm.n):
                            if prev == None:
                                break
                            syms.append(prev.sym)
                            prev = prev.prev
                        yield next, frame, score, lm.score(*syms)[0]
                    else:
                        yield next, frame, score, lm.score(next.sym, node.sym)[0]
                else:
                    yield next, frame, score, 0

    def n_nodes(self):
        """
        Return the number of nodes in the DAG
        @return: Number of nodes in the DAG
        @rtype: int
        """
        return sum(map(len, self))

    def n_edges(self):
        """
        Return the number of edges in the DAG
        @return: Number of edges in the DAG
        @rtype: int
        """
        return (len(tuple(self.edges(self.start)))
                + sum(map(lambda x:
                          sum(map(lambda y:
                                  len(tuple(self.edges(y))), x.itervalues())), self)))

    def nodes(self):
        """
        Return a generator over all the nodes in the DAG, in time order
        @return: Generator over all nodes in the DAG, in time order
        @rtype: generator(Dag.Node)
        """
        for frame in self:
            for node in frame.values():
                yield node

    def reverse_nodes(self):
        """
        Return a generator over all the nodes in the DAG, in reverse time order
        @return: Generator over all nodes in the DAG, in reverse time order
        @rtype: generator(Dag.Node)
        """
        for frame in self[::-1]: # reversed() isn't in Python 2.3
            for node in frame.values():
                yield node

    def all_edges(self):
        """
        Return a generator over all the edges in the DAG, in time order
        @return: Generator over all edges in the DAG, in time order
        @rtype: generator((int,object,Dag.Node))
        """
        for sf in self:
            for node in sf.itervalues():
                for ef, ascr in node.exits:
                    yield ef, ascr, node

    def bestpath(self, lm=None, start=None, end=None):
        """
        Find best path through lattice using Dijkstra's algorithm.

        @param lm: Language model to use in search
        @type lm: sphinx.arpalm.ArpaLM (or equivalent)
        @param start: Node to start search from
        @type start: Dag.Node
        @param end: Node to end search at
        @type end: Dag.Node
        @return: Final node in search (same as C{end})
        @rtype: Dag.Node
        """
        Q = list(self.nodes())
        for u in Q:
            u.score = LOGZERO
            u.prev = None
        if start == None:
            start = self.start
        if end == None:
            end = self.end
        start.score = 0
        while Q:
            bestscore = LOGZERO
            bestidx = 0
            for i,u in enumerate(Q):
                if u.score > bestscore:
                    bestidx = i
                    bestscore = u.score
            u = Q[bestidx]
            del Q[bestidx]
            if u == end:
                return u
            for v, frame, ascr, lscr in self.edges(u, lm):
                if isinstance(ascr, tuple):
                    ascr = ascr[0]
                score = ascr + lscr
                if u.score + score > v.score:
                    v.score = u.score + score
                    v.prev = u

    def bypass_fillers(self):
        """Bypass filler nodes in the lattice."""
        for u in self.nodes():
            for v, frame, ascr, lscr in self.edges(u):
                if is_filler(v.sym):
                    for vv, frame, ascr, lscr in self.edges(v):
                        if not is_filler(vv.sym):
                            u.exits.append((vv.entry, 0))

    def minimum_error(self, hyp, start=None):
        """
        Find the minimum word error rate path through lattice,
        returning the number of errors and an alignment.
        @return: Tuple of (error-count, alignment of (hyp, ref) pairs)
        @rtype: (int, list(string, string))
        """
        # Get the set of nodes in proper order
        nodes = tuple(self.nodes())
        # Initialize the alignment matrix
        align_matrix = numpy.ones((len(hyp),len(nodes)), 'i') * 999999999
        # And the backpointer matrix
        bp_matrix = numpy.zeros((len(hyp),len(nodes)), 'O')
        # Remove filler nodes from the reference
        hyp = filter(lambda x: not is_filler(x), hyp)
        # Bypass filler nodes in the lattice
        self.bypass_fillers()
        # Figure out the minimum distance to each node from the start
        # of the lattice, and the set of predecessors for each node
        for u in nodes:
            u.score = 999999999
            u.prev = []
        if start == None:
            start = self.start
        start.score = 1
        for i,u in enumerate(nodes):
            if is_filler(u.sym):
                continue
            for v, frame, ascr, lscr in self.edges(u):
                dist = u.score + 1
                if dist < v.score:
                    v.score = dist
                if not i in v.prev:
                    v.prev.append(i)
        def find_pred(ii, jj):
            bestscore = 999999999
            bestp = -1
            if len(nodes[jj].prev) == 0:
                return bestp, bestscore
            for k in nodes[jj].prev:
                if align_matrix[ii,k] < bestscore:
                    bestp = k
                    bestscore = align_matrix[ii,k]
            return bestp, bestscore
        # Now fill in the alignment matrix
        for i, w in enumerate(hyp):
            for j, u in enumerate(nodes):
                # Insertion = cost(w, prev(u)) + 1
                if len(u.prev) == 0: # start node
                    bestp = -1
                    inscost = i + 2 # Distance from start of ref
                else:
                    # Find best predecessor in the same reference position
                    bestp, bestscore = find_pred(i, j)
                    inscost = align_matrix[i,bestp] + 1
                # Deletion  = cost(prev(w), u) + 1
                if i == 0: # start symbol
                    delcost = u.score + 1 # Distance from start of hyp
                else:
                    delcost = align_matrix[i-1,j] + 1
                # Substitution = cost(prev(w), prev(u)) + (w != u)
                if i == 0 and bestp == -1: # Start node, start of ref
                    subcost = int(w != u.sym)
                elif i == 0: # Start of ref
                    subcost = nodes[bestp].score + int(w != u.sym)
                elif bestp == -1: # Start node
                    subcost = i - 1 + int(w != u.sym)
                else:
                    # Find best predecessor in the previous reference position
                    bestp, bestscore = find_pred(i-1, j)
                    subcost = align_matrix[i-1,bestp] + int(w != u.sym)
                align_matrix[i,j] = min(subcost, inscost, delcost)
                # Now find the argmin
                if align_matrix[i,j] == subcost:
                    bp_matrix[i,j] = (i-1, bestp)
                elif align_matrix[i,j] == inscost:
                    bp_matrix[i,j] = (i, bestp)
                else:
                    bp_matrix[i,j] = (i-1, j)
        # Find last node's index
        last = 0
        for i, u in enumerate(nodes):
            if u == self.end:
                last = i
        # Backtrace to get an alignment
        i = len(hyp)-1
        j = last
        bt = []
        while True:
            ip,jp = bp_matrix[i,j]
            if ip == i: # Insertion
                bt.append(('INS', nodes[j].sym))
            elif jp == j: # Deletion
                bt.append((hyp[i], 'DEL'))
            else:
                bt.append((hyp[i], nodes[j].sym))
            # If we consume both ref and hyp, we are done
            if ip == -1 and jp == -1:
                break
            # If we hit the beginning of the ref, fill with insertions
            if ip == -1:
                while True:
                    bt.append(('INS', nodes[jp].sym))
                    bestp, bestscore = find_pred(i,jp)
                    if bestp == -1:
                        break
                    jp = bestp
                break
            # If we hit the beginning of the hyp, fill with deletions
            if jp == -1:
                while ip >= 0:
                    bt.append((hyp[ip], 'DEL'))
                    ip = ip - 1
                break
            # Follow the pointer
            i,j = ip,jp
        bt.reverse()
        return align_matrix[-1,last], bt

    def backtrace(self, end=None):
        """
        Return a backtrace from an optional end node after bestpath.

        @param end: End node to backtrace from (default is final node in DAG)
        @type end: Dag.Node
        @return: Best path through lattice from start to end.
        @rtype: list of Dag.Node
        """
        if end == None:
            end = self.end
        backtrace = []
        while end:
            backtrace.append(end)
            end = end.prev
        backtrace.reverse()
        return backtrace

    def find_preds(self):
        """
        Find predecessor nodes for each node in the lattice and store them
        in its 'prev' field.
        """
        for u in self.nodes():
            u.prev = []
        for w in self.nodes():
            for f, s in w.exits:
                for u in self[f].itervalues():
                    if w not in u.prev:
                        u.prev.append(w)

    def traverse_depth(self, start=None):
        """Depth-first traversal of DAG nodes"""
        if start == None:
            start = self.start
        # Initialize the agenda (set of root nodes)
        roots = [start]
        # Keep a table of already seen nodes
        seen = {start:1}
        # Repeatedly pop the first one off of the agenda and push
        # all of its successors
        while roots:
            r = roots.pop()
            for ef, ascr in r.exits:
                for v in self[ef].itervalues():
                    if v not in seen:
                        roots.append(v)
                        seen[v] = 1
            yield r

    def traverse_breadth(self, start=None):
        """Breadth-first traversal of DAG nodes"""
        if start == None:
            start = self.start
        # Initialize the agenda (set of active nodes)
        roots = [start]
        # Keep a table of already seen nodes
        seen = {start:1}
        # Repeatedly pop the first one off of the agenda and shift
        # all of its successors
        while roots:
            r = roots.pop()
            for ef, ascr in r.exits:
                for v in self[ef].itervalues():
                    if v not in seen:
                        roots.insert(0, v)
                        seen[v] = 1
            yield r

    def reverse_breadth(self, end=None):
        """Breadth-first reverse traversal of DAG nodes"""
        if end == None:
            end = self.end
        if end.prev == None:
            self.find_preds()
        # Initialize the agenda (set of active nodes)
        roots = [end]
        # Keep a table of already seen nodes
        seen = {end:1}
        # Repeatedly pop the first one off of the agenda and shift
        # all of its successors
        while roots:
            r = roots.pop()
            for v in r.prev:
                if v not in seen:
                    roots.insert(0, v)
                seen[v] = 1
            yield r

    def remove_unreachable(self):
        """Remove unreachable nodes and dangling edges."""
        if self.end.prev == None:
            self.find_preds()
        for w in self.reverse_breadth():
            w.score = 42
        for frame in self:
            for sym in frame.keys():
                if frame[sym].score != 42:
                    del frame[sym]
        for frame in self:
            for node in frame.itervalues():
                newexits = []
                for ef, ascr in node.exits:
                    if len(self[ef]) > 0:
                        newexits.append((ef, ascr))
                node.exits = newexits

    def forward(self, lm=None):
        """
        Compute forward variable for all arcs in the lattice.

        @param lm: Language model to use in computation
        @type lm: sphinx.arpalm.ArpaLM (or equivalent)
        """
        self.find_preds()
        self.remove_unreachable()
        # For each node in self (they sort forward by time, which is
        # actually the only thing that guarantees that a nodes'
        # predecessors will be touched before it)
        for w in self.nodes():
            # For each outgoing arc from w
            for i,x in enumerate(w.exits):
                wf, wascr = x
                # This is alpha_t(w)
                alpha = LOGZERO
                # If w has no predecessors the previous alpha is 1.0
                if len(w.prev) == 0:
                    alpha = wascr / lm.lw
                # For each predecessor node to w
                for v in w.prev:
                    # Get unscaled language model score P(w|v) (bigrams only for now...)
                    if lm:
                        lscr = lm.prob(v.sym, w.sym)[0]
                    else:
                        lscr = 0
                    # Find the arc from v to w to get its alpha
                    for vf, vs in v.exits:
                        vascr, valpha, vbeta = vs
                        if vf == w.entry:
                            # Accumulate alpha for this arc
                            alpha = logadd(alpha, valpha + lscr + wascr / lm.lw)
                # Update the acoustic score to hold alpha and beta
                w.exits[i] = (wf, (wascr, alpha, LOGZERO))

    def backward(self, lm=None):
        """
        Compute backward variable for all arcs in the lattice.

        @param lm: Language model to use in computation
        @type lm: sphinxbase.ngram_model.NGramModel (or equivalent)
        """
        # For each node in self (in reverse):
        for w in self.reverse_nodes():
            # For each predecessor to w
            for v in w.prev:
                # Beta for arcs into </s> = 1.0
                if w == self.end:
                    beta = 0
                else:
                    beta = LOGZERO
                    # Get unscaled language model probability P(w|v) (bigrams only for now...)
                    if lm:
                        lscr = lm.prob(v.sym, w.sym)[0]
                    else:
                        lscr = 0
                    # For each outgoing arc from w
                    for wf, ws in w.exits:
                        wascr, walpha, wbeta = ws
                        # Accumulate beta for arc from v to w
                        beta = logadd(beta, wbeta + lscr + wascr / lm.lw)
                # Find the arc from v to w to update its beta
                for i, arc in enumerate(v.exits):
                    vf, vs = arc
                    if vf == w.entry:
                        vascr, valpha, vbeta = vs
                        v.exits[i] = (vf, (vascr, valpha, logadd(vbeta, beta)))

    def posterior(self, lm=None):
        """
        Compute arc posterior probabilities.

        @param lm: Language model to use in computation
        @type lm: sphinxbase.ngram_model.NGramModel (or equivalent)
        """
        # Run forward and backward if not already done
        frame, ascr = self.start.exits[0]
        if not isinstance(ascr, tuple):
            self.forward(lm)
        frame, ascr = self.start.exits[0]
        if ascr[2] == LOGZERO:
            self.backward(lm)
        # Sum over alpha for arcs entering the end node to get normalizer
        norm = LOGZERO
        for v in self.end.prev:
            for ef, ascr in v.exits:
                if ef == self.end.entry:
                    ascr, alpha, beta = ascr
                    norm = logadd(norm, alpha)
        # Iterate over all arcs and normalize
        for w in self.nodes():
            for i, x in enumerate(w.exits):
                ef, ascr = x
                ascr, alpha, beta = ascr
                w.exits[i] = (ef, (ascr, alpha, beta, alpha + beta - norm))
