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
			custom_dense_hole_grid) target=20 ;; \
			custom_narrow_gaps_with_three_holes) target=15 ;; \
			custom_near_collinear_wavy_shell) target=10 ;; \
			custom_pinch_corridor_with_two_holes) target=12 ;; \
			custom_comb_bays_with_inner_hole) target=18 ;; \
		esac; \
		./$(EXECUTABLE) "$$input" "$$target" > "$$out"; \
		echo "Generated $$out (target_vertices=$$target)"; \
	done

generate_outputs: run

clean:
	rm -rf $(OBJ_DIR) $(EXECUTABLE) $(OUT_DIR)
