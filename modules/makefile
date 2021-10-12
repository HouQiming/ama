all: $(addsuffix .js, $(basename $(basename $(wildcard *.ama.js)))) \
  $(addsuffix .js, $(basename $(basename $(wildcard **/*.ama.js))))

%.js: %.ama.js
	ama $<
