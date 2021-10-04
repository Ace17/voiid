include build/common_head.mak

CROSS_COMPILE?=
ifneq (,$(CROSS_COMPILE))
CXX:=$(CROSS_COMPILE)g++
endif

EXT?=.exe
BIN_HOST?=bin_host
TARGETS+=$(BIN_HOST)

all: true_all

PKGS:=\
	sdl2\

PKG_CFLAGS:=$(shell pkg-config $(PKGS) --cflags)
PKG_LDFLAGS:=$(shell pkg-config $(PKGS) --libs || echo "ERROR")

ifeq (ERROR,$(PKG_LDFLAGS))
  $(error At least one library was not found in the build environment)
endif

DBGFLAGS?=-g

CXXFLAGS+=-Wall -Wextra
CXXFLAGS+=-Isrc
CXXFLAGS+=-I.
CXXFLAGS+=-Iengine/include
CXXFLAGS+=-std=c++17
CXXFLAGS+=$(PKG_CFLAGS)
LDFLAGS+=$(PKG_LDFLAGS)

CXXFLAGS+=-O3

CXXFLAGS+=$(DBGFLAGS)
LDFLAGS+=$(DBGFLAGS)

#------------------------------------------------------------------------------

SRCS_ENGINE:=\
	src/engine/app.cpp\
	src/engine/main.cpp\
	src/engine/stats.cpp\
	src/audio/audio.cpp\
	src/audio/sound_ogg.cpp\
	src/misc/base64.cpp\
	src/misc/decompress.cpp\
	src/misc/file.cpp\
	src/misc/json.cpp\
	src/misc/string.cpp\
	src/misc/time.cpp\
	src/render/renderer.cpp\
	src/render/rendermesh.cpp\
	src/render/picture.cpp\
	src/render/postprocess.cpp\
	src/render/png.cpp\
	src/render/mesh_import.cpp\

SRCS_ENGINE+=\
	src/platform/audio_sdl.cpp\
	src/platform/input_sdl.cpp\
	src/platform/display_ogl.cpp\
	src/platform/glad.cpp\

SRCS_MESHCOOKER:=\
	src/engine/main_meshcooker.cpp\
	src/misc/decompress.cpp\
	src/misc/file.cpp\
	src/render/mesh_import.cpp\

#-----------------------------------
$(BIN_HOST):
	@mkdir -p "$@"

$(BIN_HOST)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	@echo [HOST] compile "$@"
	g++ -Isrc -Iengine/src -c "$^" -o "$@"

$(BIN_HOST)/meshcooker.exe: $(SRCS_MESHCOOKER:%=$(BIN_HOST)/%.o)
	@mkdir -p $(dir $@)
	g++ $^ -o '$@'


#------------------------------------------------------------------------------

SRCS_GAME:=\
	src/entities/all.cpp\
	src/entities/amulet.cpp\
	src/entities/bonus.cpp\
	src/entities/door.cpp\
	src/entities/editor.cpp\
	src/entities/explosion.cpp\
	src/entities/hero.cpp\
	src/entities/lamp.cpp\
	src/entities/move.cpp\
	src/entities/moving_platform.cpp\
	src/entities/finish.cpp\
	src/entities/switch.cpp\
	src/gameplay/convex.cpp\
	src/gameplay/entity_factory.cpp\
	src/gameplay/game.cpp\
	src/gameplay/physics.cpp\
	src/gameplay/resources.cpp\
	src/gameplay/room_loader.cpp\
	src/gameplay/state_playing.cpp\
	src/gameplay/state_splash.cpp\
	src/gameplay/state_ending.cpp\

#------------------------------------------------------------------------------

SRCS:=\
	$(SRCS_GAME)\
	$(SRCS_ENGINE)\

$(BIN)/rel/game$(EXT): $(SRCS:%=$(BIN)/%.o)
	@mkdir -p $(dir $@)
	$(CXX) $^ -o '$@' $(LDFLAGS)

TARGETS+=$(BIN)/rel/game$(EXT)

#------------------------------------------------------------------------------
include assets/project.mk

#------------------------------------------------------------------------------

SRCS_TESTS:=\
	$(SRCS_GAME)\
	$(filter-out src/engine/main.cpp, $(SRCS_ENGINE))\
	src/tests/tests.cpp\
	src/tests/tests_main.cpp\
	src/tests/audio.cpp\
	src/tests/base64.cpp\
	src/tests/decompress.cpp\
	src/tests/json.cpp\
	src/tests/util.cpp\
	src/tests/png.cpp\
	src/tests/entities.cpp\
	src/tests/physics.cpp\
	src/tests/trace.cpp\

$(BIN)/tests$(EXT): $(SRCS_TESTS:%=$(BIN)/%.o)
	@mkdir -p $(dir $@)
	$(CXX) $^ -o '$@' $(LDFLAGS)

TARGETS+=$(BIN)/tests$(EXT)

include build/common.mak
