import heapq

graph = {
    "A": {"B": 5.0, "C": 7.0},
    "B": {"C": 3.0},
    "C": {"D": 4.0},
    "D": {"E": 6.0},
    "E": {}
}

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

path, dist = dijkstra(graph, "A", "E")
print(f"Shortest path from A to E: {" -> ".join(path)} with distance {dist}")