COMPILER  := g++
CPPFLAGS  := -g3 -pthread -mclwb -std=c++2a -shared -fPIC -DESCORT_PROF
CPPFLAGS  += -DMEASURE -DALLOCATOR_RALLOC #-DOLD_VERSION
CPPFLAGS_OPT += -O3 -DNDEBUG
CPPFLAGS_DBG += -O0 -DDEBUG
LDFLAGS   := -ljemallocescort -L./jemalloc/lib -lhwloc

SRCDIR      := src
NVMDIR      := $(SRCDIR)/nvm
JEMALLOCDIR := $(PWD)/$(SRCDIR)/jemalloc
OBJDIR      := obj
TARGETDIR   := bin
TARGET      := libescort.so libescortdbg.so
SRCS        := $(wildcard $(SRCDIR)/*.cpp) $(wildcard $(NVMDIR)/*.cpp)
OBJS        := $(addprefix $(OBJDIR)/,$(patsubst %.cpp,%.o,$(SRCS)))
OBJS_DBG    := $(addprefix $(OBJDIR)/,$(patsubst %.cpp,%_dbg.o,$(SRCS)))

.PHONY: all clean
all: $(patsubst %,$(TARGETDIR)/%,$(TARGET))

$(TARGETDIR)/libescort.so: $(OBJS) 
	@if [ ! -d $(TARGETDIR) ]; then \
		mkdir -p $(TARGETDIR); 	\
	fi
	$(COMPILER) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: %.cpp
	@if [ ! -d `dirname $@` ]; then	\
		mkdir -p `dirname $@`;	\
	fi
	$(COMPILER) $(CPPFLAGS) $(CPPFLAGS_OPT) -o $@ -c $< $(LDFLAGS) -MMD

$(TARGETDIR)/libescortdbg.so: $(OBJS_DBG) 
	@if [ ! -d $(TARGETDIR) ]; then \
		mkdir -p $(TARGETDIR); 	\
	fi
	$(COMPILER) $(CPPFLAGS) $(CPPFLAGS_DBG) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%_dbg.o: %.cpp
	@if [ ! -d `dirname $@` ]; then	\
		mkdir -p `dirname $@`;	\
	fi
	$(COMPILER) $(CPPFLAGS) $(CPPFLAGS_DBG) -o $@ -c $< $(LDFLAGS) -MMD
clean:
	rm -rf $(OBJDIR)/* $(TARGETDIR)/*
	rm -rf *~ $(SRCDIR)/*~ $(NVMDIR)/*~


