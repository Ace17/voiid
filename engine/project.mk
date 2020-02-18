SRCS_ENGINE:=\
	$(BIN)/$(ENGINE_ROOT)/src/render/fragment.glsl.cpp\
	$(BIN)/$(ENGINE_ROOT)/src/render/vertex.glsl.cpp\
	$(ENGINE_ROOT)/src/app.cpp\
	$(ENGINE_ROOT)/src/main.cpp\
	$(ENGINE_ROOT)/src/audio/audio.cpp\
	$(ENGINE_ROOT)/src/audio/audio_sdl.cpp\
	$(ENGINE_ROOT)/src/audio/sound_ogg.cpp\
	$(ENGINE_ROOT)/src/misc/base64.cpp\
	$(ENGINE_ROOT)/src/misc/decompress.cpp\
	$(ENGINE_ROOT)/src/misc/file.cpp\
	$(ENGINE_ROOT)/src/misc/json.cpp\
	$(ENGINE_ROOT)/src/render/display_ogl.cpp\
	$(ENGINE_ROOT)/src/render/glad.cpp\
	$(ENGINE_ROOT)/src/render/rendermesh.cpp\
	$(ENGINE_ROOT)/src/render/png.cpp\
	$(ENGINE_ROOT)/src/render/3ds.cpp\

$(BIN)/$(ENGINE_ROOT)/src/render/vertex.glsl.cpp: NAME=VertexShaderCode
$(BIN)/$(ENGINE_ROOT)/src/render/fragment.glsl.cpp: NAME=FragmentShaderCode

$(BIN)/%.glsl.cpp: %.glsl
	@mkdir -p $(dir $@)
	scripts/embed.sh "$<" "$@" "$(NAME)"

$(BIN)/$(ENGINE_ROOT)/src/%: CXXFLAGS+=-I$(ENGINE_ROOT)/src

SRCS_MESHCOOKER:=\
	$(ENGINE_ROOT)/src/main_meshcooker.cpp\
	$(ENGINE_ROOT)/src/misc/file.cpp\
	$(ENGINE_ROOT)/src/render/3ds.cpp\

#-----------------------------------
$(BIN_HOST):
	@mkdir -p "$@"

$(BIN_HOST)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	@echo [HOST] compile "$@"
	g++ -Iengine/include -Iengine/src -c "$^" -o "$@"

$(BIN_HOST)/meshcooker.exe: $(SRCS_MESHCOOKER:%=$(BIN_HOST)/%.o)
	@mkdir -p $(dir $@)
	g++ $^ -o '$@'

