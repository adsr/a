JSX_FILES = $(shell find src/ -type f -name '*.jsx')

all: bundle.js

bundle.js: $(JSX_FILES)
	cat $^ >bundle.jsx
	babel --presets react,es2015 bundle.jsx >bundle.js
	rm -f bundle.jsx

clean:
	rm -f bundle.js*
