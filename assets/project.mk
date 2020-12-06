#-----------------------------------
# Sound & Music

SOUNDS_SRC+=$(wildcard assets/sounds/*.ogg)
SOUNDS_SRC+=$(wildcard assets/music/*.ogg)
TARGETS+=$(SOUNDS_SRC:assets/%.ogg=res/%.ogg)

res/%.ogg: assets/%.ogg
	@mkdir -p $(dir $@)
	@echo "Transcode $<"
	@ffmpeg -loglevel 1 -y -i "$<" -ar 22050 -ac 2 "$@" </dev/null

#-----------------------------------
# Fonts

TARGETS+=res/font.png res/white.png

#-----------------------------------
# ROOMS

ROOMS+=$(wildcard assets/rooms/*)
TARGETS+=$(ROOMS:assets/%=res/%/room.settings)
TARGETS+=$(ROOMS:assets/%=res/%/room.mesh)
TARGETS+=$(ROOMS:assets/%=res/%/room.render)

res/%/room.mesh: assets/%/room.blend ./scripts/export_from_blender.py
	@mkdir -p $(dir $@)
	./scripts/export_from_blender "$<" "$@"

#-----------------------------------
# Meshes

SPRITES_SRC+=$(wildcard assets/sprites/*.blend)
TARGETS+=$(SPRITES_SRC:assets/%.blend=res/%.render)

res/%.mesh: assets/%.blend ./scripts/export_from_blender.py
	@mkdir -p $(dir $@)
	./scripts/export_from_blender "$<" "$@"

res/%.render: res/%.mesh $(BIN_HOST)/meshcooker.exe
	@mkdir -p $(dir $@)
	$(BIN_HOST)/meshcooker.exe "$<" "$(dir assets/$*)" "res/$*.render"

#-----------------------------------
# Shaders

SHADERS_SRC+=$(wildcard assets/shaders/*.vert)
SHADERS_SRC+=$(wildcard assets/shaders/*.frag)
TARGETS+=$(SHADERS_SRC:assets/%=res/%)

res/%.frag: assets/%.frag
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(BIN)/$*)
	glslangValidator -G -o "$(BIN)/$*.spv" "$<"
	@cp "$<" "$@"

res/%.vert: assets/%.vert
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(BIN)/$*)
	glslangValidator -G -o "$(BIN)/$*.spv" "$<"
	@cp "$<" "$@"

#-----------------------------------
res/%: assets/%
	@mkdir -p $(dir $@)
	@cp "$<" "$@"

