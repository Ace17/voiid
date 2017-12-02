SOUNDS_SRC+=$(wildcard res-src/sounds/*.ogg)
SOUNDS_SRC+=$(wildcard res-src/music/*.ogg)
TARGETS+=$(SOUNDS_SRC:res-src/%.ogg=res/%.ogg)

res/%.ogg: res-src/%.ogg
	@mkdir -p $(dir $@)
	@echo "Transcode $<"
	@ffmpeg -loglevel 1 -y -i "$<" -ar 22050 -ac 2 "$@" </dev/null

ROOMS_SRC+=$(wildcard res-src/rooms/*.blend)
TARGETS+=$(ROOMS_SRC:res-src/%.blend=res/%.3ds)

res/%.3ds: res-src/%.blend
	@mkdir -p $(dir $@)
	@echo "Convert to 3ds $<"
	@./scripts/convert_to_3ds "$<" "$@"

