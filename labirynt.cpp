#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <atomic>

// const int width = 81;
// const int height = 41;

const int width = 42;
const int height = 15;

int maze[height][width];
std::mutex mazeMutex[height][width];

unsigned char image[height][width][3];
std::atomic_int threadId{2};
std::atomic_int crossRoadCount{0};
std::atomic_int childrenCount{0};

void loadMaze(const char *fileName)
{
  char c;
  int i = 0;
  int y = 0;
  int x = 0;
  int j;
  FILE *f = fopen(fileName, "rt");

  while ((c = fgetc(f)) != EOF)
  {
    if (c == '\n')
    {
      x = 0;
      y++;
    }
    else
    {
      maze[y][x] = c == '0' ? 0 : 1;
      x++;
    }
  }

  fclose(f);
}

FILE *initPPM()
{
  const char *fileName = "new1.ppm";
  const char *comment = "# ";
  FILE *f = fopen(fileName, "wb");
  fprintf(f, "P6\n %s\n %d\n %d\n %d\n", comment, width, height, 255);

  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++)
    {
      image[y][x][0] = 255;
      image[y][x][1] = 255;
      image[y][x][2] = 255;
    }
  }

  return f;
}

int genThreadId()
{
  ++threadId;
  return threadId;
}

bool checkCorridor(int y, int x)
{
  const std::lock_guard<std::mutex> lock(mazeMutex[y][x]);
  return maze[y][x] == 0;
}

bool writeToMaze(int y, int x, int threadId)
{
  const std::lock_guard<std::mutex> lock(mazeMutex[y][x]);
  if (maze[y][x] != 0)
  {
    return false;
  }
  maze[y][x] = threadId;
  return true;
}

void mazeRun(int entrance_y, int entrance_x)
{
  int y = entrance_y;
  int x = entrance_x;
  int id = genThreadId();
  bool movable = true;
  std::vector<std::thread> children;

  while (movable)
  {
    movable = writeToMaze(y, x, id);
    if (!movable)
    {
      break;
    }

    std::vector<std::pair<int, int>> directions;
    // const int dx[] = {-1, 1, 0, 0};
    // const int dy[] = {0, 0, -1, 1};
    const int dx[] = {0, 0, -1, 1};
    const int dy[] = {-1, 1, 0, 0};

    for (int i = 0; i < 4; i++)
    {
      int newY = y + dy[i];
      int newX = x + dx[i];

      if (newY >= 0 && newY < height && newX >= 0 && newX < width && checkCorridor(newY, newX))
      {
        directions.emplace_back(std::make_pair(newY, newX));
      }
    }

    if (directions.size() == 1)
    {
      y = directions.at(0).first;
      x = directions.at(0).second;
    }
    else if (directions.size() == 0)
    {
      movable = false;
    }
    else
    {
      crossRoadCount++;
      y = directions.at(0).first;
      x = directions.at(0).second;
      for (int i = 1; i < directions.size(); i++)
      {
        int child_y = directions.at(i).first;
        int child_x = directions.at(i).second;
        childrenCount++;
        children.emplace_back(std::thread(mazeRun, child_y, child_x));
      }
    }
  }

  for (auto &child : children)
  {
    child.join();
  }
}

void finalizePPM(FILE *ppm)
{
  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++)
    {
      if (maze[y][x] == 1)
      {
        continue;
      }
      int red = 50 * maze[y][x];
      int green = 25 * maze[y][x];
      int blue = 1 * maze[y][x];
      image[y][x][0] = (char)red;
      image[y][x][1] = (char)green;
      image[y][x][2] = (char)blue;
    }
  }
  fwrite(image, 1, 3 * height * width, ppm);
}

void printMaze()
{
  for (int y = 0; y < height; ++y)
  {
    for (int x = 0; x < width; ++x)
    {
      printf("%d ", maze[y][x]);
    }
    printf("\n");
  }
}

void corridorStats()
{
  std::unordered_map<int, int> occurrences;
  for (int y = 0; y < height; ++y)
  {
    for (int x = 0; x < width; ++x)
    {
      int value = maze[y][x];
      if (value != 1)
      {
        occurrences[value]++;
      }
    }
  }

  int maxValue = occurrences.begin()->first;
  int maxCount = occurrences.begin()->second;
  int totalLength = 0;
  int totalCorridors = 0;
  for (const auto &entry : occurrences)
  {
    if (entry.second > maxCount)
    {
      maxValue = entry.first;
      maxCount = entry.second;
    }
    totalLength += entry.first * entry.second;
    totalCorridors += entry.second;
  }
  double averageLength = static_cast<double>(totalLength) / totalCorridors;

  std::cout << "Longest corridor: " << maxValue << ", it's length: " << maxCount << std::endl;
  std::cout << "Average corridor length: " << averageLength << std::endl;
}

void stats()
{
  corridorStats();
  std::cout << "Visited crossroads: " << crossRoadCount << std::endl;
  std::cout << "Children count: " << childrenCount << std::endl;
}

int main()
{
  loadMaze("medium.txt");
  FILE *ppm = initPPM();

  std::thread mazeRunner(mazeRun, 1, 1);
  mazeRunner.join();

  finalizePPM(ppm);
  fclose(ppm);

  stats();

  return 0;
}
