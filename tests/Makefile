include Makefile.builtem

include ../src/Makefile.include

TARGETS = compile_test libmicrohttpd_fileserver xml_builder_test

MICRORESTD = $(call obj,$(MICRORESTD_OBJECTS:%=../src/%))
MICRORESTD_PUGIXML = $(call obj,$(MICRORESTD_PUGIXML_OBJECTS:%=../src/%))
C_FLAGS += $(call include_dir,../src)
LD_FLAGS += $(call use_library,$(if $(filter win-%,$(PLATFORM)),$(MICRORESTD_LIBRARIES_WIN),$(MICRORESTD_LIBRARIES_POSIX)))

.PHONY: all compile
all: compile

compile: $(call exe,$(TARGETS))

$(call exe,compile_test): $(call obj,compile_test) $(MICRORESTD) $(MICRORESTD_PUGIXML)
$(call exe,libmicrohttpd_fileserver): $(call obj,libmicrohttpd_fileserver) $(MICRORESTD)
$(call exe,xml_builder_test): $(call obj,xml_builder_test) $(MICRORESTD) $(MICRORESTD_PUGIXML)
$(call exe,$(TARGETS)):
	$(call link_exe,$@,$^,$(call win_subsystem,console))

.PHONY: clean
clean:
	$(call rm,.build $(call all_exe,$(TARGETS)))
