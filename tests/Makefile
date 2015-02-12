include Makefile.builtem

include ../src/Makefile.include

TARGETS = libmicrohttpd_fileserver
C_FLAGS += $(call include_dir,../src)
LD_FLAGS += $(call use_library,$(if $(filter win-%,$(PLATFORM)),$(MICRORESTD_LIBRARIES_WIN),$(MICRORESTD_LIBRARIES_POSIX)))

.PHONY: all compile
all: compile

compile: $(call obj,$(MICRORESTD_OBJECTS:%=../src/%) $(MICRORESTD_PUGIXML_OBJECTS:%=../src/%)) $(call exe,$(TARGETS))

$(call exe,libmicrohttpd_fileserver): $(call obj,libmicrohttpd_fileserver) $(call obj,$(MICRORESTD_OBJECTS:%=../src/%))
$(call exe,$(TARGETS)):
	$(call link_exe,$@,$^,$(call win_subsystem,console))

.PHONY: clean
clean:
	$(call rm,.build $(call all_exe,$(TARGETS)))
