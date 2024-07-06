NAME := taskmaster
SRC_DIR := src
INCLUDE_DIR := include
BUILD_DIR := build

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)
CXX_DEFS := NAME=\"$(NAME)\"

CXX := g++
CXX_FLAGS := -Wextra -Werror -Wall -std=c++17 -O2 -g3
CXX_LIBS :=

CXX_HEADERS := -I$(INCLUDE_DIR)

CXX_DEPS_FLAGS := -MP -MMD
CXX_DEFS_FLAGS := $(foreach def,$(CXX_DEFS),-D $(def))

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXX_FLAGS) $(CXX_HEADERS) $(OBJS) $(CXX_LIBS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXX_FLAGS) $(CXX_DEPS_FLAGS) $(CXX_DEFS_FLAGS) $(CXX_HEADERS) -c $< -o $@

include $(wildcard $(DEPS))

clean:
	@rm -rf $(BUILD_DIR)

fclean: clean
	@rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
