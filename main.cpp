#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef std::string Str;


static Str makefileTemplate = R"(
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -Iinclude

SRC_DIR = src
BUILD_DIR = build
BIN = $(BUILD_DIR)/{projectName}

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

all: build

build: $(BIN)

$(BIN): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: build
	./$(BIN)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all build clean run)";

static Str headerTemplate = R"(
#ifndef {projectName}_HPP
#define {projectName}_HPP

#define interface  struct
#define implements public

typedef unsigned char      int8;
typedef unsigned short     int16;
typedef unsigned int       int32;
typedef unsigned long long int64;
typedef int32              word;

#define $1 (int8 *)
#define $2 (int16 *)
#define $4 (int32 *)
#define $8 (int64 *)
#define $i (int *)
#define $v (void *)

#endif // {projectName}_HPP)";

static Str sourceTemplate = R"(
#include "{projectName}.hpp"
#include <iostream>

int main(int argc, const char *argv[]) {
  std::cout << "Project {projectName} started successfully." << std::endl;
  return 0;
})";

void safe_create_file(const Str &filePath, const Str &content) {
  std::ofstream file(filePath);
  if (!file.is_open()) {
    std::cerr << "Error creating file: " << filePath << "\n";
    return;
  }
  file << content;
  file.close();
}

void replace_sub_str(Str &dest, const Str &oldSubstring,
                     const Str &newSubstring) {
  size_t startPos = dest.find(oldSubstring);
  while (startPos != std::string::npos) {
    dest.replace(startPos, oldSubstring.length(), newSubstring);
    startPos = dest.find(oldSubstring, startPos + newSubstring.length());
  }
}

void create_directory(const Str &path) {
  if (mkdir(path.c_str(), 0755) != 0) {
    if (errno != EEXIST) {
      std::cerr << "Error creating directory '" << path
                << "': " << strerror(errno) << "\n";
      exit(EXIT_FAILURE);
    }
  }
}

bool validate_project_name(const Str &name) {
  if (name.empty()) {
    std::cerr << "Project name cannot be empty.\n";
    return false;
  }
  if (name.find('/') != Str::npos || name.find('\\') != Str::npos) {
    std::cerr << "Project name cannot contain '/' or '\\'.\n";
    return false;
  }
  if (name == "." || name == "..") {
    std::cerr << "Project name cannot be '.' or '..'.\n";
    return false;
  }
  return true;
}

int main(int argc, const char *argv[]) {
  if (argc < 3) {
    std::cerr << "Usage: bs create <project_name>\n";
    return EXIT_FAILURE;
  }

  Str action = argv[1];
  Str projectName = argv[2];

  if (action != "create") {
    std::cerr << "Unknown action: " << action << "\n";
    return EXIT_FAILURE;
  }

  if (!validate_project_name(projectName)) {
    return EXIT_FAILURE;
  }

  char cwd[512];
  if (getcwd(cwd, sizeof(cwd)) == nullptr) {
    std::cerr << "Failed to get current working directory.\n";
    return EXIT_FAILURE;
  }

  Str projectPath = Str(cwd) + "/" + projectName;

  create_directory(projectPath);

  create_directory(projectPath + "/src");
  create_directory(projectPath + "/include");

  Str makefileContent = makefileTemplate;
  replace_sub_str(makefileContent, "{projectName}", projectName);
  safe_create_file(projectPath + "/Makefile", makefileContent);

  Str headerContent = headerTemplate;
  replace_sub_str(headerContent, "{projectName}", projectName);
  safe_create_file(projectPath + "/include/" + projectName + ".hpp",
                   headerContent);

  Str sourceContent = sourceTemplate;
  replace_sub_str(sourceContent, "{projectName}", projectName);
  safe_create_file(projectPath + "/src/" + projectName + ".cpp", sourceContent);

  std::cout << "Project '" << projectName
            << "' created successfully at: " << projectPath << "\n";

  return 0;
}
