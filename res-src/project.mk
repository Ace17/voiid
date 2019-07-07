SOUNDS_SRC+=$(wildcard res-src/sounds/*.ogg)
SOUNDS_SRC+=$(wildcard res-src/music/*.ogg)
TARGETS+=$(SOUNDS_SRC:res-src/%.ogg=res/%.ogg)

res/%.ogg: res-src/%.ogg
	@mkdir -p $(dir $@)
	@echo "Transcode $<"
	@ffmpeg -loglevel 1 -y -i "$<" -ar 22050 -ac 2 "$@" </dev/null

ROOMS_SRC+=$(wildcard res-src/rooms/*/mesh.blend)
TARGETS+=$(ROOMS_SRC:res-src/%.blend=res/%.3ds)
TARGETS+=$(ROOMS_SRC:res-src/%.blend=res/%.render)

SPRITES_SRC+=$(wildcard res-src/sprites/*.blend)
TARGETS+=$(SPRITES_SRC:res-src/%.blend=res/%.3ds)
TARGETS+=$(SPRITES_SRC:res-src/%.blend=res/%.render)
TARGETS+=$(SPRITES_SRC:res-src/%.blend=res/%.png)

TARGETS+=res/font.png
res/font.png: res-src/font.png

res/%.sa.blend: res-src/%.blend ./scripts/preprocess_blender.py
	@mkdir -p $(dir $@)
	@echo "Preprocess blender file $<"
	@./scripts/preprocess_blender "$<" "$@"

res/%.3ds: res/%.sa.blend ./scripts/convert_to_3ds.py
	@mkdir -p $(dir $@)
	@echo "Convert to 3ds (physics) $<"
	@./scripts/convert_to_3ds "$<" "$@"

res/%.render: res/%.sa.blend ./scripts/import_rendermesh_from_blender.py
	@mkdir -p $(dir $@)
	@echo "Convert to 3ds (render) $<"
	@cp res-src/$*.png res/$*.diffuse.png
	@./scripts/import_rendermesh_from_blender "$<" "$@" "res/$*.lightmap.png"

res/%: res-src/%
	@mkdir -p $(dir $@)
	@cp "$<" "$@"

