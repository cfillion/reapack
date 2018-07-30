.PHONY: all
all:
	$(error Run 'tup' to build or 'make test' instead)

.PHONY: prepare
prepare:
	@ruby prepare.rb src/*.{{h,c}pp,mm}

.PHONY: test
test:
	-x64/bin/test
	-x86/bin/test
