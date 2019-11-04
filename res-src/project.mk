SOUNDS_SRC+=$(wildcard res-src/sounds/*.ogg)
SOUNDS_SRC+=$(wildcard res-src/music/*.ogg)
TARGETS+=$(SOUNDS_SRC:res-src/%.ogg=res/%.ogg)

res/%.ogg: res-src/%.ogg
	@mkdir -p $(dir $@)
	@echo "Transcode $<"
	@ffmpeg -loglevel 1 -y -i "$<" -ar 22050 -ac 2 "$@" </dev/null

ROOMS_SRC+=$(wildcard res-src/rooms/*/mesh.blend)
TARGETS+=$(ROOMS_SRC:res-src/%.blend=res/%.mesh)
TARGETS+=$(ROOMS_SRC:res-src/%.blend=res/%.render)

SPRITES_SRC+=$(wildcard res-src/sprites/*.blend)
TARGETS+=$(SPRITES_SRC:res-src/%.blend=res/%.render)
TARGETS+=$(SPRITES_SRC:res-src/%.blend=res/%.png)

TARGETS+=res/font.png
res/font.png: res-src/font.png

res/%.sa.blend: res-src/%.blend ./scripts/preprocess_blender.py
	@mkdir -p $(dir $@)
	@echo "Preprocess blender file: $<"
	@./scripts/preprocess_blender "$<" "$@"

res/%.mesh: res/%.sa.blend ./scripts/export_from_blender.py
	@mkdir -p $(dir $@)
	@echo "Exporting from blender: $<"
	@./scripts/export_from_blender "$<" "$@"

res/%.render: res/%.mesh $(BIN)/meshcooker.exe
	@mkdir -p $(dir $@)
	@cp res-src/$*.png res/$*.diffuse.png
	$(BIN)/meshcooker.exe "$<" "$@" "res/$*.lightmap.png"

res/%: res-src/%
	@mkdir -p $(dir $@)
	@cp "$<" "$@"

