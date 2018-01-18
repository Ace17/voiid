SOUNDS_SRC+=$(wildcard res-src/sounds/*.ogg)
SOUNDS_SRC+=$(wildcard res-src/music/*.ogg)
TARGETS+=$(SOUNDS_SRC:res-src/%.ogg=res/%.ogg)

res/%.ogg: res-src/%.ogg
	@mkdir -p $(dir $@)
	@echo "Transcode $<"
	@ffmpeg -loglevel 1 -y -i "$<" -ar 22050 -ac 2 "$@" </dev/null

ROOMS_SRC+=$(wildcard res-src/rooms/*/mesh.blend)
TARGETS+=$(ROOMS_SRC:res-src/%.blend=res/%.3ds)
TARGETS+=$(ROOMS_SRC:res-src/%.blend=res/%.3ds.render)

JSON_SRCS+=$(wildcard res-src/rooms/*/mesh.json)
TARGETS+=$(JSON_SRCS:res-src/%.json=res/%.json)

PNG_SRCS+=$(wildcard res-src/rooms/*/mesh.png)
TARGETS+=$(PNG_SRCS:res-src/%.png=res/%.png)

SPRITES_SRC+=$(wildcard res-src/sprites/*.blend)
TARGETS+=$(SPRITES_SRC:res-src/%.blend=res/%.3ds)

res/%.3ds: res-src/%.blend
	@mkdir -p $(dir $@)
	@echo "Convert to 3ds (physics) $<"
	@./scripts/convert_to_3ds "$<" "$@"

res/%.3ds.render: res-src/%.blend
	@mkdir -p $(dir $@)
	@echo "Convert to 3ds (render) $<"
	@./scripts/convert_to_3ds_for_rendering "$<" "$@" "res/$*.png"

res/%: res-src/%
	@mkdir -p $(dir $@)
	@cp "$<" "$@"

