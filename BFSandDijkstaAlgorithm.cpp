#include <iostream>
#include <iomanip>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <vector>
#include <utility>
#include <queue>
#include <tuple>
#include <algorithm>
#include <cstdlib>

struct Graph {
    std::unordered_map<char, std::vector<char> > edges;

    std::vector<char> neighbors(char id) {
        return edges[id];
    }
};

Graph ExampleGraph{ {
    {'A', {'B'}},
    {'B', {'A', 'C', 'D'}},
    {'C', {'A'}},
    {'D', {'E', 'A'}},
    {'E', {'B'}}
  } };


struct GridLocation 
{
    int x, y;
};

namespace std 
{
    template <> struct hash<GridLocation> {
        typedef GridLocation argument_type;
        typedef std::size_t result_type;
        std::size_t operator()(const GridLocation& id) const noexcept {
            return std::hash<int>()(id.x ^ (id.y << 4));
        }
    };
}

struct SquareGrid {
    static std::array<GridLocation, 4> DIRS;

    int width, height;
    std::unordered_set<GridLocation> walls;

    SquareGrid(int width_, int height_)
        : width(width_), height(height_) {}

    bool in_bounds(GridLocation id) const {
        return 0 <= id.x && id.x < width
            && 0 <= id.y && id.y < height;
    }

    bool passable(GridLocation id) const {
        return walls.find(id) == walls.end();
    }

    std::vector<GridLocation> neighbors(GridLocation id) const {
        std::vector<GridLocation> results;

        for (GridLocation dir : DIRS) {
            GridLocation next{ id.x + dir.x, id.y + dir.y };
            if (in_bounds(next) && passable(next)) {
                results.push_back(next);
            }
        }

        if ((id.x + id.y) % 2 == 0) {
            std::reverse(results.begin(), results.end());
        }

        return results;
    }
};

std::array<GridLocation, 4> SquareGrid::DIRS = {
    GridLocation{1, 0}, GridLocation{-1, 0},
    GridLocation{0, -1}, GridLocation{0, 1}
};

bool operator == (GridLocation a, GridLocation b) {
    return a.x == b.x && a.y == b.y;
}

bool operator != (GridLocation a, GridLocation b) {
    return !(a == b);
}

bool operator < (GridLocation a, GridLocation b) {
    return std::tie(a.x, a.y) < std::tie(b.x, b.y);
}

std::basic_iostream<char>::basic_ostream& operator<<(std::basic_iostream<char>::basic_ostream& out, const GridLocation& loc) {
    out << '(' << loc.x << ',' << loc.y << ')';
    return out;
}

template<class Graph>
void draw_grid(const Graph& graph,
    std::unordered_map<GridLocation, double>* distances = nullptr,
    std::unordered_map<GridLocation, GridLocation>* point_to = nullptr,
    std::vector<GridLocation>* path = nullptr,
    GridLocation* A = nullptr,
    GridLocation* Goal = nullptr) {
    const int field_width = 3;
    std::cout << std::string(field_width * graph.width, '_') << '\n';
    for (int y = 0; y != graph.height; ++y) {
        for (int x = 0; x != graph.width; ++x) {
            GridLocation id{ x, y };
            if (graph.walls.find(id) != graph.walls.end()) {
                std::cout << std::string(field_width, '#');
            }
            else if (A && id == *A) {
                std::cout << " A ";
            }
            else if (Goal && id == *Goal) {
                std::cout << " Z ";
            }
            else if (path != nullptr && find(path->begin(), path->end(), id) != path->end()) {
                std::cout << " @ ";
            }
            else if (point_to != nullptr && point_to->count(id)) {
                GridLocation next = (*point_to)[id];
                if (next.x == x + 1) { std::cout << " > "; }
                else if (next.x == x - 1) { std::cout << " < "; }
                else if (next.y == y + 1) { std::cout << " v "; }
                else if (next.y == y - 1) { std::cout << " ^ "; }
                else { std::cout << " * "; }
            }
            else if (distances != nullptr && distances->count(id)) {
                std::cout << ' ' << std::left << std::setw(field_width - 1) << (*distances)[id];
            }
            else {
                std::cout << " . ";
            }
        }
        std::cout << '\n';
    }
    std::cout << std::string(field_width * graph.width, '~') << '\n';
}

void add_rect(SquareGrid& grid, int x1, int y1, int x2, int y2) {
    for (int x = x1; x < x2; ++x) {
        for (int y = y1; y < y2; ++y) {
            grid.walls.insert(GridLocation{ x, y });
        }
    }
}

struct GridWithWeights : SquareGrid {
    std::unordered_set<GridLocation> forests;
    GridWithWeights(int w, int h) : SquareGrid(w, h) {}
    double cost(GridLocation from_node, GridLocation to_node) const {
        return forests.find(to_node) != forests.end() ? 5 : 1;
    }
};

GridWithWeights make_diagram4() {
    GridWithWeights grid(10, 10);
    add_rect(grid, 1, 7, 4, 9);
    typedef GridLocation L;
    grid.forests = std::unordered_set<GridLocation>{
      L{3, 4}, L{3, 5}, L{4, 1}, L{4, 2},
      L{4, 3}, L{4, 4}, L{4, 5}, L{4, 6},
      L{4, 7}, L{4, 8}, L{5, 1}, L{5, 2},
      L{5, 3}, L{5, 4}, L{5, 5}, L{5, 6},
      L{5, 7}, L{5, 8}, L{6, 2}, L{6, 3},
      L{6, 4}, L{6, 5}, L{6, 6}, L{6, 7},
      L{7, 3}, L{7, 4}, L{7, 5}
    };
    return grid;
}

template<typename T, typename priority_t>
struct PriorityQueue {
    typedef std::pair<priority_t, T> PQElement;
    std::priority_queue<PQElement, std::vector<PQElement>,
        std::greater<PQElement>> elements;

    inline bool empty() const {
        return elements.empty();
    }

    inline void put(T item, priority_t priority) {
        elements.emplace(priority, item);
    }

    T get() {
        T best_item = elements.top().second;
        elements.pop();
        return best_item;
    }
};

template<typename Location, typename Graph>
std::unordered_map<Location, Location>
breadth_first_search(Graph graph, Location A) {
    std::queue<Location> frontier;
    frontier.push(A);

    std::unordered_map<Location, Location> came_from;
    came_from[A] = A;

    while (!frontier.empty()) {
        Location current = frontier.front();
        frontier.pop();

        for (Location next : graph.neighbors(current)) {
            if (came_from.find(next) == came_from.end()) {
                frontier.push(next);
                came_from[next] = current;
            }
        }
    }
    return came_from;
}

template<typename Location, typename Graph>
void dijkstra_search
(Graph graph,
    Location A,
    Location Goal,
    std::unordered_map<Location, Location>& came_from,
    std::unordered_map<Location, double>& cost_so_far)
{
    PriorityQueue<Location, double> frontier;
    frontier.put(A, 0);

    came_from[A] = A;
    cost_so_far[A] = 0;

    while (!frontier.empty()) {
        Location current = frontier.get();

        if (current == Goal) {
            break;
        }

        for (Location next : graph.neighbors(current)) {
            double new_cost = cost_so_far[current] + graph.cost(current, next);
            if (cost_so_far.find(next) == cost_so_far.end()
                || new_cost < cost_so_far[next]) {
                cost_so_far[next] = new_cost;
                came_from[next] = current;
                frontier.put(next, new_cost);
            }
        }
    }
}

template<typename Location>
std::vector<Location> reconstruct_path(
    Location A, Location Goal,
    std::unordered_map<Location, Location> came_from
) {
    std::vector<Location> path;
    Location current = Goal;
    while (current != A) {
        path.push_back(current);
        current = came_from[current];
    }
    path.push_back(A);
    std::reverse(path.begin(), path.end());
    return path;
}


//UNCOMMENT FOR A*

//inline double heuristic(GridLocation a, GridLocation b) {
//    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
//}
//
//template<typename Location, typename Graph>
//void a_star_search
//(Graph graph,
//    Location A,
//    Location Goal,
//    std::unordered_map<Location, Location>& came_from,
//    std::unordered_map<Location, double>& cost_so_far)
//{
//    PriorityQueue<Location, double> frontier;
//    frontier.put(A, 0);
//
//    came_from[A] = A;
//    cost_so_far[A] = 0;
//
//    while (!frontier.empty()) {
//        Location current = frontier.get();
//
//        if (current == Goal) {
//            break;
//        }
//
//        for (Location next : graph.neighbors(current)) {
//            double new_cost = cost_so_far[current] + graph.cost(current, next);
//            if (cost_so_far.find(next) == cost_so_far.end()
//                || new_cost < cost_so_far[next]) {
//                cost_so_far[next] = new_cost;
//                double priority = new_cost + heuristic(next, Goal);
//                frontier.put(next, priority);
//                came_from[next] = current;
//            }
//        }
//    }
//}

int main()
{
    GridWithWeights grid = make_diagram4();
    GridLocation A{ 1,4 }, Goal{ 8,3 };
    std::unordered_map<GridLocation, GridLocation> came_from;
    std::unordered_map<GridLocation, double> cost_so_far;


    //COMMENT OUT FOR A*

    //Dijkstra Algorithm
    dijkstra_search(grid, A, Goal, came_from, cost_so_far); 
    draw_grid(grid, nullptr, &came_from, nullptr, &A, &Goal);
    std::cout << '\n';
    std::vector<GridLocation> path = reconstruct_path(A, Goal, came_from);
    draw_grid(grid, nullptr, nullptr, &path, &A, &Goal);
    std::cout << '\n';
    draw_grid(grid, &cost_so_far, nullptr, nullptr, &A, &Goal);

    //UNCOMMENT FOR A*

    ////A* Algorithm
    //a_star_search(grid, A, Goal, came_from, cost_so_far);
    //draw_grid(grid, nullptr, &came_from, nullptr, &A, &Goal);
    //std::cout << '\n';
    //std::vector<GridLocation> path = reconstruct_path(A, Goal, came_from);
    //draw_grid(grid, nullptr, nullptr, &path, &A, &Goal);
    //std::cout << '\n';
    //draw_grid(grid, &cost_so_far, nullptr, nullptr, &A, &Goal);
}
