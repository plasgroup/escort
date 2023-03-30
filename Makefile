COMPILER  := g++
CPPFLAGS  := -g3 -pthread -mclwb -std=c++2a -O3 -shared -fPIC #-DNDEBUG
#CPPFLAGS  += -DOLD_VERSION
# CPPFLAGS += -DDEBUG_MODE
LDFLAGS   := -ljemalloc -L./jemalloc/lib -lhwloc

SRCDIR      := src
NVMDIR      := $(SRCDIR)/nvm
JEMALLOCDIR := $(PWD)/$(SRCDIR)/jemalloc
OBJDIR      := obj
TARGETDIR   := bin
TARGET      := libescort.so
SRCS        := $(wildcard $(SRCDIR)/*.cpp) $(wildcard $(NVMDIR)/*.cpp)
OBJS        := $(addprefix $(OBJDIR)/,$(patsubst %.cpp,%.o,$(SRCS)))

.PHONY: all clean
all: $(TARGETDIR)/$(TARGET)

$(TARGETDIR)/$(TARGET): $(OBJS) 
	@if [ ! -d $(TARGETDIR) ]; then \
		mkdir -p $(TARGETDIR); 	\
	fi
	$(COMPILER) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: %.cpp
	@if [ ! -d `dirname $@` ]; then	\
		mkdir -p `dirname $@`;	\
	fi
	$(COMPILER) $(CPPFLAGS) -o $@ -c $< $(LDFLAGS) -MMD
clean:
	rm -rf $(OBJDIR)/* $(TARGETDIR)/*
	rm -rf *~ $(SRCDIR)/*~ $(NVMDIR)/*~
