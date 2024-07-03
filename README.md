# MazeFinder-GeneticAlgorithm

MazeFinder-GeneticAlgorithm is a C++ application that uses a genetic algorithm to solve mazes of size 20x20. This project demonstrates the use of evolutionary techniques to find optimal paths through mazes, leveraging multi-threading for improved performance.

## Features

- **Genetic Algorithm:** Implements a genetic algorithm to evolve sequences of moves (genomes) that navigate through mazes.
- **Thread Pool:** Utilizes a custom thread pool for parallel evaluation of genome fitness, achieving significant speed improvements.
- **Dynamic Mutation and Crossover:** Includes mechanisms for genome mutation and crossover to explore and exploit potential solutions.
- **Maze Representation:** Uses a 2D vector to represent maze structures, enabling customizable maze configurations.

## Getting Started

To compile and run the MazeFinder-GeneticAlgorithm on your local machine:

1. **Clone the repository:**
```bash
git clone https://github.com/brettadams0/MazeFinder-GeneticAlgorithm.git
```
Navigate to the project directory:
```bash
cd MazeFinder-GeneticAlgorithm
```
Compile the code:

Ensure you have a C++ compiler that supports C++11 and the pthread library.
```bash
  g++ -std=c++11 -pthread main.cpp -o main
```
Run the executable:
```bash
  ./main
```
## Usage

Upon running the program, it will generate an initial population of genomes.
Each generation evaluates genome fitness based on its ability to navigate through the maze.
Top-performing genomes are selected for crossover and mutation to form the next generation.
The process repeats for a specified number of generations, optimizing the pathfinding solution.

## Contributing

Contributions are welcome! Please fork the repository and create a pull request with your improvements. For major changes, please open an issue first to discuss what you would like to change.
