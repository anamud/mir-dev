#!/usr/bin/python

import networkx as nx

def longest_path(G):
    dist = {} # stores [node, distance] pair
    for node in nx.topological_sort(G):
        # pairs of dist,node for all incoming edges
        pairs = [(dist[v][0]+1,v) for v in G.pred[node]] 
        if pairs:
            dist[node] = max(pairs)
        else:
            dist[node] = (0, node)
    node,(length,_)  = max(dist.items(), key=lambda x:x[1])
    path = []
    while length > 0:
        path.append(node)
        length,node = dist[node]
    return list(reversed(path))

if __name__=='__main__':
    G = nx.DiGraph()
    G.add_path([1,2,3,4])
    G.add_path([1,20,30,31,32,4])
#    G.add_path([20,2,200,31])
    print(longest_path(G))
