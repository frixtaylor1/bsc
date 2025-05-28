#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef std::string Str;

#define safe_create_file(filePath, content)                                    \
  std::ofstream file(filePath);                                                \
  if (!file.is_open()) {                                                       \
    std::cerr << "Error creating file: " << filePath << "\n";                  \
    return;                                                                    \
  }                                                                            \
  file << content;                                                             \
  file.close();

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

static Str entryPointFileHeader = R"(
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

static Str entryPointFileSource = R"(
#include "../include/{projectName}.hpp"
#include <iostream>

int main(int argc, const char *argv[]) {
  std::cout << "Hello world from {projectName}!." << std::endl;
  return 0;
})";

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
    std::cerr << "No se pudo crear el directorio (puede que ya exista): "
              << path << '\n';
  }
}

void build_root_directory(const Str &path) { create_directory(path); }

void build_source_directory(const Str &path) {
  create_directory(path + "/src");
}

void build_include_directory(const Str &path) {
  create_directory(path + "/include");
}

void build_make_file(const Str &dir, const Str &projectName) {
  Str content = makefileTemplate;
  replace_sub_str(content, "{projectName}", projectName);
  safe_create_file(dir + "/Makefile", content);
}

void build_entry_point_header(const Str &dir, const Str &projectName) {
  Str content = entryPointFileHeader;
  replace_sub_str(content, "{projectName}", projectName);
  safe_create_file(dir + "/include/" + projectName + ".hpp", content);
}

void build_entry_point_source(const Str &dir, const Str &projectName) {
  Str content = entryPointFileSource;
  replace_sub_str(content, "{projectName}", projectName);
  safe_create_file(dir + "/src/" + projectName + ".cpp", content);
}

int main(int argc, const char *argv[]) {
  if (argc < 3) {
    std::cerr << "Uso: bs create <nombre_proyecto>\n";
    return EXIT_FAILURE;
  }

  Str action = argv[1];
  Str projectName = argv[2];

  if (action != "create") {
    std::cerr << "Acción desconocida: " << action << '\n';
    return EXIT_FAILURE;
  }

  char cwd[512];
  if (getcwd(cwd, sizeof(cwd)) == nullptr) {
    std::cerr << "No se pudo obtener el directorio actual\n";
    return EXIT_FAILURE;
  }

  Str projectPath = Str(cwd) + "/" + projectName;

  build_root_directory(projectPath);
  build_source_directory(projectPath);
  build_include_directory(projectPath);
  build_make_file(projectPath, projectName);
  build_entry_point_header(projectPath, projectName);
  build_entry_point_source(projectPath, projectName);

  std::cout << "Project \"" << projectName
            << "\" created: [path: " << projectPath << "]\n";

  return 0;
}
