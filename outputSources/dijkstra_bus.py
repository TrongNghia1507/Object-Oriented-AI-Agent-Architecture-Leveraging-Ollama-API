nodes = {"A": {"B": 5, "C": 2}, "B": {"A": 5, "C": 1, "D": 3}, "C": {"A": 2, "B": 1, "D": 4, "E": 8}, "D": {"B": 3, "C": 4, "E": 2}, "E": {"C": 8, "D": 2}}

import heapq

def dijkstra(graph, start, end):
    distances = {node: float("inf") for node in graph}
    distances[start] = 0
    priority_queue = [(0, start)]
    previous_nodes = {node: None for node in graph}

    while priority_queue:
        current_distance, current_node = heapq.heappop(priority_queue)
        if current_node == end:
            path = []
            while current_node:
                path.append(current_node)
                current_node = previous_nodes[current_node]
            return path[::-1], distances[end]
        if current_distance > distances[current_node]:
            continue
        for neighbor, weight in graph[current_node].items():
            distance = current_distance + weight
            if distance < distances[neighbor]:
                distances[neighbor] = distance
                previous_nodes[neighbor] = current_node
                heapq.heappush(priority_queue, (distance, neighbor))
    return None, float("inf")

start_node = "A"
end_node = "E"
path, distance = dijkstra(nodes, start_node, end_node)
print(f"Shortest path from {start_node} to {end_node}: {" -> ".join(path)} with total distance: {distance}")