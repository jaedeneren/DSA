CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3
SRC_DIR = .
OBJ_DIR = obj
INPUT_DIR = input_test_cases
OUT_DIR = generated_outputs

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))

ifeq ($(OS),Windows_NT)
EXECUTABLE = simplify.exe
else
EXECUTABLE = simplify
endif

# Default target vertices for inputs without a custom override.
TARGET_VERTICES_DEFAULT = 99

all: $(EXECUTABLE)

.PHONY: all run generate_outputs clean

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUT_DIR):
	@mkdir -p $(OUT_DIR)

# Build and process every input_*.csv into generated_outputs/my_output_*.txt.
run: $(EXECUTABLE) $(OUT_DIR)
	@set -e; \
	for input in $(INPUT_DIR)/input_*.csv; do \
		[ -f "$$input" ] || continue; \
		base=$${input##*/}; \
		name=$${base#input_}; \
		name=$${name%.csv}; \
		out="$(OUT_DIR)/my_output_$$name.txt"; \
		target=$(TARGET_VERTICES_DEFAULT); \
		case "$$name" in \
			rectangle_with_two_holes) target=7 ;; \
			cushion_with_hexagonal_hole) target=13 ;; \
			blob_with_two_holes) target=17 ;; \
			wavy_with_three_holes) target=21 ;; \
			lake_with_two_islands) target=17 ;; \
			custom_high_hole) target=350 ;; \
			custom_high_vertex) target=500 ;; \
			custom_narrow_gaps) target=50 ;; \
			custom_near_degeneracies) target=100 ;; \
			custom_combined) target=1500 ;; \
		esac; \
		./$(EXECUTABLE) "$$input" "$$target" > "$$out"; \
		echo "Generated $$out (target_vertices=$$target)"; \
	done

generate_outputs: run

clean:
	rm -rf $(OBJ_DIR) $(EXECUTABLE) $(OUT_DIR)
