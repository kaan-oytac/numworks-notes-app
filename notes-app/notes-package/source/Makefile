APP_NAME = notes
APP_ICON = src/icon.png

OUTPUT_DIR = output

SOURCES = $(addprefix src/,\
  doc.cpp \
  main.cpp \
  mathlayout.cpp \
  notes.cpp \
  ui.cpp \
)

include build/external_app.mak

# make WITH_DATA=1 APP_NAME=notes-data OUTPUT_DIR=output-data ...
# builds the variant that accepts a notes file as external data.
ifeq ($(WITH_DATA),1)
  CXXFLAGS += -DNOTES_WITH_DATA
endif

ifeq ($(PLATFORM),device)
  LDLIBS = -lm -lc -lgcc
endif
