include build/common_head.mak

CROSS_COMPILE?=
ifneq (,$(CROSS_COMPILE))
CXX:=$(CROSS_COMPILE)g++
endif

EXT?=.exe

all: true_all

PKGS:=\
	gl\
	sdl2\
	SDL2_image\
	SDL2_mixer\

PKG_CFLAGS:=$(shell pkg-config $(PKGS) --cflags)
PKG_LDFLAGS:=$(shell pkg-config $(PKGS) --libs || echo "ERROR")

ifeq (ERROR,$(PKG_LDFLAGS))
  $(error At least one library was not found in the build environment)
endif

CXXFLAGS+=-Iengine/extra

CXXFLAGS+=-Wall -Wextra
CXXFLAGS+=-Isrc
CXXFLAGS+=-I.
CXXFLAGS+=-Iengine/include
CXXFLAGS+=-std=c++14
CXXFLAGS+=$(PKG_CFLAGS)
LDFLAGS+=$(PKG_LDFLAGS)

CXXFLAGS+=-O3

#CXXFLAGS+=-g3
#LDFLAGS+=-g

#------------------------------------------------------------------------------

SRCS_GAME:=\
	src/entities/bonus.cpp\
	src/entities/explosion.cpp\
	src/entities/rockman.cpp\
	src/entities/switch.cpp\
	src/entity_factory.cpp\
	src/game.cpp\
	src/level_graph.cpp\
	src/physics.cpp\
	src/resources.cpp\
	src/smarttiles.cpp\

#------------------------------------------------------------------------------

SRCS:=\
	$(SRCS_GAME)\
	engine/extra/miniz.c\
	$(BIN)/fragment.glsl.cpp\
	$(BIN)/vertex.glsl.cpp\
	engine/src/app.cpp\
	engine/src/base64.cpp\
	engine/src/decompress.cpp\
	engine/src/display.cpp\
	engine/src/json.cpp\
	engine/src/main.cpp\
	engine/src/model.cpp\
	engine/src/sound.cpp\

$(BIN)/rel/game$(EXT): $(SRCS:%.cpp=$(BIN)/%_cpp.o)
	@mkdir -p $(dir $@)
	$(CXX) $^ -o '$@' $(LDFLAGS)

TARGETS+=$(BIN)/rel/game$(EXT)

#------------------------------------------------------------------------------

SRCS_TESTS:=\
	$(SRCS_GAME)\
	engine/extra/miniz.c\
	engine/src/base64.cpp\
	engine/src/decompress.cpp\
	engine/src/json.cpp\
	engine/tests/base64.cpp\
	engine/tests/decompress.cpp\
	engine/tests/json.cpp\
	engine/tests/tests.cpp\
	engine/tests/tests_main.cpp\
	engine/tests/tokenizer.cpp\
	engine/tests/util.cpp\
	tests/game/entities.cpp\
	tests/game/physics.cpp\

$(BIN)/tests$(EXT): $(SRCS_TESTS:%.cpp=$(BIN)/%_cpp.o)
	@mkdir -p $(dir $@)
	$(CXX) $^ -o '$@' $(LDFLAGS)

TARGETS+=$(BIN)/tests$(EXT)

$(BIN)/vertex.glsl.cpp: engine/src/vertex.glsl
	@mkdir -p $(dir $@)
	scripts/embed.sh "$<" "$@" "VertexShaderCode"

$(BIN)/fragment.glsl.cpp: engine/src/fragment.glsl
	@mkdir -p $(dir $@)
	scripts/embed.sh "$<" "$@" "FragmentShaderCode"

include build/common.mak
