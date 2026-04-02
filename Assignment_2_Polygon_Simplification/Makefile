CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3
SRC_DIR = .
OBJ_DIR = obj
TEST_DIR = test_cases
OUT_DIR = generated_outputs

# The list of objects now explicitly includes the new visualization component
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SOURCES))
EXECUTABLE = simplify

all: $(EXECUTABLE)

.PHONY: all clean generate_outputs

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUT_DIR):
	@mkdir -p $(OUT_DIR)

generate_outputs: $(EXECUTABLE) $(OUT_DIR)
	./$(EXECUTABLE) $(TEST_DIR)/input_rectangle_with_two_holes.csv 11 > $(OUT_DIR)/my_output_rectangle_with_two_holes.txt
	./$(EXECUTABLE) $(TEST_DIR)/input_cushion_with_hexagonal_hole.csv 13 > $(OUT_DIR)/my_output_cushion_with_hexagonal_hole.txt
	./$(EXECUTABLE) $(TEST_DIR)/input_blob_with_two_holes.csv 17 > $(OUT_DIR)/my_output_blob_with_two_holes.txt
	./$(EXECUTABLE) $(TEST_DIR)/input_wavy_with_three_holes.csv 21 > $(OUT_DIR)/my_output_wavy_with_three_holes.txt
	./$(EXECUTABLE) $(TEST_DIR)/input_lake_with_two_islands.csv 17 > $(OUT_DIR)/my_output_lake_with_two_islands.txt
	./$(EXECUTABLE) $(TEST_DIR)/input_original_01.csv 99 > $(OUT_DIR)/my_output_01.txt
	./$(EXECUTABLE) $(TEST_DIR)/input_original_02.csv 99 > $(OUT_DIR)/my_output_02.txt
	./$(EXECUTABLE) $(TEST_DIR)/input_original_03.csv 99 > $(OUT_DIR)/my_output_03.txt
	./$(EXECUTABLE) $(TEST_DIR)/input_original_04.csv 99 > $(OUT_DIR)/my_output_04.txt
	./$(EXECUTABLE) $(TEST_DIR)/input_original_05.csv 99 > $(OUT_DIR)/my_output_05.txt
	./$(EXECUTABLE) $(TEST_DIR)/input_original_06.csv 99 > $(OUT_DIR)/my_output_06.txt
	./$(EXECUTABLE) $(TEST_DIR)/input_original_07.csv 99 > $(OUT_DIR)/my_output_07.txt
	./$(EXECUTABLE) $(TEST_DIR)/input_original_08.csv 99 > $(OUT_DIR)/my_output_08.txt
	./$(EXECUTABLE) $(TEST_DIR)/input_original_09.csv 99 > $(OUT_DIR)/my_output_09.txt
	./$(EXECUTABLE) $(TEST_DIR)/input_original_10.csv 99 > $(OUT_DIR)/my_output_10.txt

clean:
	rm -rf $(OBJ_DIR) $(EXECUTABLE) $(OUT_DIR)