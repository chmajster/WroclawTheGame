"""Exact portal routing: bounded local searches and a lazily expanded sector graph.
Only original OSM topology can connect sectors. No proximity welding or invented links.
"""
import heapq
import math
from road_routes import adjacency


class HierarchicalRouter:
    def __init__(self, graph, owners, mode='car'):
        self.links = adjacency(graph, mode)
        if set(self.links) != set(owners):
            raise ValueError('Every road node needs exactly one sector owner')
        self.owners = owners
        self.portals = {s: set() for s in owners.values()}
        self.crossings = {}
        for node, edges in self.links.items():
            for target, length, _ in edges:
                if owners[node] != owners[target]:
                    self.portals[owners[node]].add(node)
                    self.portals[owners[target]].add(target)
                    self.crossings.setdefault(node, []).append((target, length))

    def local_paths(self, start, goal):
        owner = self.owners[start]
        targets = self.portals[owner] - {start}
        if self.owners[goal] == owner and goal != start:
            targets = targets | {goal}
        queue = [(0, start)]; distance = {start: 0}; previous = {}; found = []
        while queue and targets:
            cost, node = heapq.heappop(queue)
            if cost != distance[node]: continue
            if node in targets:
                path = [node]
                while path[-1] != start: path.append(previous[path[-1]])
                found.append((node, cost, list(reversed(path))))
                targets.remove(node)
            for target, length, _ in self.links[node]:
                if self.owners[target] != owner: continue
                candidate = cost + length
                if candidate < distance.get(target, math.inf):
                    distance[target] = candidate; previous[target] = node
                    heapq.heappush(queue, (candidate, target))
        return found

    def route(self, start, goal):
        if start not in self.links or goal not in self.links:
            raise ValueError('Unknown route endpoint')
        queue = [(0, start)]; distance = {start: 0}; previous = {}
        while queue:
            cost, node = heapq.heappop(queue)
            if cost != distance[node]: continue
            if node == goal:
                chunks = []; cursor = goal
                while cursor != start:
                    parent, segment = previous[cursor]; chunks.append(segment[1:]); cursor = parent
                path = [start]
                for chunk in reversed(chunks): path.extend(chunk)
                sectors = []
                for item in path:
                    owner = self.owners[item]
                    if not sectors or owner != sectors[-1]: sectors.append(owner)
                return {'nodes': path, 'sectors': sectors, 'length_cm': cost}
            candidates = self.local_paths(node, goal)
            candidates.extend((target, length, [node, target]) for target, length in self.crossings.get(node, []))
            for target, length, segment in candidates:
                candidate = cost+length
                if candidate < distance.get(target, math.inf):
                    distance[target] = candidate; previous[target] = node, segment
                    heapq.heappush(queue, (candidate, target))
        raise ValueError('No legal connected route')
