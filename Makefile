CC = cc

NAME = bolao_copa

UNAME_S = $(shell uname -s)
CFLAGS   = -O3 -fPIC -Wall -Wextra

ANSWERS_CSV ?= $(PWD)/answers.csv

LIBPAPAGO_URL = https://github.com/tashima42/libpapago
LIBPAPAGO_DIR = vendor/libpapago
LIBPAPAGO_SRC = $(LIBPAPAGO_DIR)/papago.c

CFLAGS += -I$(LIBPAPAGO_DIR)

ifeq ($(UNAME_S),Darwin)
	CFLAGS += $(shell pkg-config --cflags libwebsockets) \
	          $(shell pkg-config --cflags libmicrohttpd) \
	          $(shell pkg-config --cflags openssl)
endif

ifeq ($(UNAME_S),Linux)
	CFLAGS += $(shell pkg-config --cflags libwebsockets) \
	          $(shell pkg-config --cflags libmicrohttpd) \
	          $(shell pkg-config --cflags openssl)
endif

LDFLAGS = -lwebsockets -lmicrohttpd -lssl -lcrypto -lz -lm -lpthread

ifeq ($(UNAME_S),Darwin)
	LDFLAGS += $(shell pkg-config --libs libwebsockets) \
	           $(shell pkg-config --libs libmicrohttpd) \
	           $(shell pkg-config --libs openssl)
endif

IMAGE_NAME ?= bolao_copa:dev

.PHONY: docker
docker:
	podman build -t $(IMAGE_NAME) .

.PHONY: run
run:
	@[ -f $(ANSWERS_CSV) ] || echo "name,match1,match2,match3,match4,match5,match6,match7,match8,match9,match10,match11,match12,match13,match14,match15,match16" > $(ANSWERS_CSV)
	podman run --rm -p 8282:8282 -v $(ANSWERS_CSV):/data/answers.csv $(IMAGE_NAME)


deps:
	@if [ ! -d $(LIBPAPAGO_DIR) ]; then \
		git clone --depth=1 $(LIBPAPAGO_URL) $(LIBPAPAGO_DIR); \
	fi

.PHONY: build
build: deps
	$(CC) -o $(NAME) $(LIBPAPAGO_SRC) bolao_copa.c $(CFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(NAME)

.PHONY: clean-all
clean-all: clean
	rm -rf $(LIBPAPAGO_DIR)

