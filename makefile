ROOTDIR := .
OUTDIR := $(ROOTDIR)
SRCDIRS := $(ROOTDIR)
SRCEXTS := .c
DEBUG_DIR := $(OUTDIR)/debug/
RELEASE_DIR := $(OUTDIR)/release/
DEBUG = 0

ifeq ($(DEBUG),1)
OBJDIR := $(DEBUG_DIR)
CPPFLAGS := -g  
else
OBJDIR := $(RELEASE_DIR)
CPPFLAGS := -O2 
endif

PROGRAMDIR := $(ROOTDIR)/bin
PROGRAM := $(PROGRAMDIR)/main
CPPFLAGS := $(CPPFLAGS)
LDFLAGS := -lpthread

CC  = gcc
RM = rm -f

SHELL = /bin/bash
FULLSOURCES = $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $(d)/*,$(SRCEXTS))))
SOURCES = $(notdir $(FULLSOURCES))
OBJS = $(foreach x,$(SRCEXTS), $(patsubst %$(x),%.o,$(filter %$(x),$(SOURCES))))
FULLOBJS = $(addprefix $(OBJDIR),$(OBJS))
DEPS = $(patsubst %.o,%.d,$(OBJS))

vpath %.c $(SRCDIRS)
vpath %.o $(OBJDIR)

.PHONY : all objs clean rebuild

all : config $(PROGRAM)

# Rules for producing the objects.
#---------------------------------------------------
objs : $(OBJS)

%.o : %.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $(OBJDIR)$(notdir $@)

# Rules for producing the executable.
#----------------------------------------------
$(PROGRAM) : $(OBJS)

ifeq ($(strip $(SRCEXTS)), .c) # C file
	$(CC) -o $(PROGRAM) $(FULLOBJS) $(INLIBS) $(LIBDIRS) $(LDFLAGS)
#else # C++ file
#	$(CXX) -o $(PROGRAM) $(FULLOBJS) $(INLIBS) $(LIBDIRS) $(LDFLAGS)
endif

test:
	@echo -e "objs: $(OBJS)"
	@echo -e "all objs: $(FULLOBJS)"

ifeq ($(DEBUG),1)
config: dir debugdir
else
config: dir releasedir
endif

debugdir:
	mkdir -p $(DEBUG_DIR)

releasedir:
	mkdir -p $(RELEASE_DIR)

dir:
	@echo -e "making dir..."
	mkdir -p $(OUTDIR)
	mkdir -p $(PROGRAMDIR)

rebuild: clean all

clean:
	@$(RM) $(FULLOBJS) 
	@$(RM) $(DEPS)
	@$(RM) $(PROGRAM)


