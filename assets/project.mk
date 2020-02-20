SOUNDS_SRC+=$(wildcard assets/sounds/*.ogg)
SOUNDS_SRC+=$(wildcard assets/music/*.ogg)
TARGETS+=$(SOUNDS_SRC:assets/%.ogg=res/%.ogg)

res/%.ogg: assets/%.ogg
	@mkdir -p $(dir $@)
	@echo "Transcode $<"
	@ffmpeg -loglevel 1 -y -i "$<" -ar 22050 -ac 2 "$@" </dev/null

ROOMS_SRC+=$(wildcard assets/rooms/*/mesh.blend)
TARGETS+=$(ROOMS_SRC:assets/%.blend=res/%.mesh)
TARGETS+=$(ROOMS_SRC:assets/%.blend=res/%.render)

SPRITES_SRC+=$(wildcard assets/sprites/*.blend)
TARGETS+=$(SPRITES_SRC:assets/%.blend=res/%.render)
TARGETS+=$(SPRITES_SRC:assets/%.blend=res/%.diffuse.png)
TARGETS+=$(SPRITES_SRC:assets/%.blend=res/%.lightmap.png)

TARGETS+=res/font.png
res/font.png: assets/font.png

res/%.mesh: assets/%.blend ./scripts/export_from_blender.py ./scripts/preprocess_blender.py
	@mkdir -p $(dir $@)
	./scripts/export_from_blender "$<" "$@"

res/%.render res/%.diffuse.png res/%.lightmap.png: res/%.mesh $(BIN_HOST)/meshcooker.exe
	@mkdir -p $(dir $@)
	@cp assets/$*.png res/$*.diffuse.png
	$(BIN_HOST)/meshcooker.exe "$<" "res/$*.render" "res/$*.lightmap.png"

res/%: assets/%
	@mkdir -p $(dir $@)
	@cp "$<" "$@"

