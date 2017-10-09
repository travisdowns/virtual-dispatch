BENCH_ROOT    = ../benchmark
BENCH_INCLUDE = $(BENCH_ROOT)/include/

LIBS=$(BENCH_ROOT)/src/libbenchmark.a -lpthread
D_LIBS=$(LIBS)
R_LIBS=$(LIBS)

CFLAGS   += -Wjump-misses-init
CPPFLAGS += -fmessage-length=0 -Wall -Wextra -march=native -I$(BENCH_INCLUDE) -Wno-unused-parameter -funroll-loops
CXXFLAGS += -std=gnu++11

# output executables
TARGETS = bench_main

# source files
CSRCS   = $(wildcard *.c)
CXXSRCS = $(wildcard *.cpp)

# headers
HEADERS = $(wildcard *.h)

# object names (actual files end up in debug/rel dirs per the vars below)
COBJS   = $(CSRCS:.c=.o)
CXXOBJS = $(CXXSRCS:.cpp=.o)

# debug build vars
D_TARGETS = $(addprefix debug/, $(TARGETS))
D_COBJS   = $(addprefix debug/, $(COBJS))
D_CXXOBJS  = $(addprefix debug/, $(CXXOBJS))
D_OBJS    = $(D_COBJS) $(D_CXXOBJS)
D_NOMAIN  = $(filter-out $(D_TARGETS:%=%.o), $(D_OBJS))
D_CPPFLAGS = -g -O0 -DDEBUG

# release build vars
R_TARGETS = $(addprefix release/, $(TARGETS))
R_COBJS   = $(addprefix release/, $(COBJS))
R_CXXOBJS  = $(addprefix release/, $(CXXOBJS))
R_OBJS    = $(R_COBJS) $(R_CXXOBJS)
R_NOMAIN  = $(filter-out $(R_TARGETS:%=%.o), $(R_OBJS))
R_CPPFLAGS = -g -O3 -DNDEBUG

.PHONY : all
all: debug release

.PHONY : init
init:
	@mkdir -p debug release

debug:   init $(D_TARGETS)

release: init $(R_TARGETS)

.PHONY : clean clean-debug clean-release
clean: clean-debug clean-release

clean-debug:
	rm -rf debug

clean-release:
	rm -rf release

$(D_TARGETS) : % : %.o $(D_NOMAIN) $(D_LIBS)
	$(CXX) -o $@ $< $(D_NOMAIN) $(D_LIBS)

$(R_TARGETS) : % : %.o $(R_NOMAIN) $(R_LIBS) 
	$(CXX) -o $@ $< $(R_NOMAIN) $(R_LIBS)

debug/%.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(D_CFLAGS) $(D_CPPFLAGS) -o $@ $<

release/%.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(R_CFLAGS) $(R_CPPFLAGS) -o $@ $<

debug/%.o: %.cpp $(HEADERS)
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $(D_CXXFLAGS) $(D_CPPFLAGS) -o $@ $<

release/%.o: %.cpp $(HEADERS)
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $(R_CXXFLAGS) $(R_CPPFLAGS) -o $@ $<
